
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/TCPServer.h"
#include "Poco/Timespan.h"
#include "Poco/NamedEvent.h"
#include "Poco/Exception.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/ThreadPool.h"
#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/Mutex.h"
#include "Poco/Random.h"
#include "Poco/AutoPtr.h"

#include <iostream>
#include <memory>
#include <sys/types.h>
#include <signal.h>


using Poco::Net::Socket;
using Poco::Net::StreamSocket;
using Poco::Net::SocketAddress;
using Poco::Net::TCPServer;
using Poco::Event;
using Poco::Exception;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::ThreadPool;
using Poco::Thread;
using Poco::Runnable;
using Poco::FastMutex;
using Poco::AutoPtr;

typedef struct
{
	char buffer[256];
	int len;
}stMessage;

class WorkNotification: public Notification
{
public:
	typedef AutoPtr<WorkNotification> Ptr;
	
	WorkNotification(std::shared_ptr<stMessage>& data):
		m_data(data)
	{
	}
	
	std::shared_ptr<stMessage> data() const
	{
		return m_data;
	}

private:
	std::shared_ptr<stMessage> m_data;
};


class OutWorker: public Runnable
{
public:
	OutWorker(const std::string& name, NotificationQueue& queue):
		_name(name),
		_queue(queue)
	{
	}
	
	void run()
	{
		Poco::Random rnd;
		for (;;)
		{
			Notification::Ptr pNf(_queue.waitDequeueNotification());
			if (pNf)
			{
				WorkNotification::Ptr pWorkNf = pNf.cast<WorkNotification>();
				if (pWorkNf)
				{
					{
						std::shared_ptr<stMessage> p = pWorkNf->data();
						std::cout << p->buffer << std::endl;
					}
					Thread::sleep(rnd.next(200));
				}
			}
			else break;
		}
	}
	
private:
	std::string        _name;
	NotificationQueue& _queue;
};

class Client: public Poco::Runnable
{
public:
	Client(const Poco::Net::SocketAddress& address, NotificationQueue& queue): m_address(address), 
				m_queue(queue), m_stop(false)
	{
	}

    /*Client(const Client& client)  //拷贝构造
	{
	}

	Client& operator = (const Client& client)//赋值构造
	{
	}*/

	~Client()
	{
		m_stop = true;
	}

	int init()
	{
		Poco::Timespan timeout(1000000);
			
		try {
			m_socket.connect(m_address, timeout);
		} catch (Exception& e) {
			std::cout << "connect err: " << e.displayText() <<std::endl;
			//std::cout << "connect err: " << e.what() <<std::endl;
			return -1;
		} catch (...) {
			std::cout << "connect err." << std::endl;
			return -1;
		}

		std::cout << "Connection from " + m_socket.peerAddress().toString() << " success." << std::endl;
		
		return 0;
	}

	void stop()
	{
		m_stop = true;
	}
	
	void run()
	{
		
		Poco::Timespan span(250000);
		while (!m_stop) {
			if (m_socket.poll(span, Socket::SELECT_READ)) {
				try {
					std::shared_ptr<stMessage> p = std::make_shared<stMessage>();

					{
						FastMutex::ScopedLock lock(m_mutex);
						int n = m_socket.receiveBytes(p->buffer, sizeof(p->buffer));
						if (n <= 0) {
							std::cout << "errno: " << n << "<= 0." << std::endl;
							break;
						}
						p->len = n;
					}
					
					//std::cout << "recv:" << p->buffer << std::endl;
					m_queue.enqueueNotification(new WorkNotification(p));
					
				} catch (Poco::Exception& exc) {
					std::cout << "Exception: " << exc.displayText() << std::endl;
					break;
				} catch (...) {
					std::cout << "some err." << std::endl;
					break;
				}
			}
		}

		if (!m_stop) {
			std::cout << "exit." << std::endl;
			kill(getpid(), SIGINT);
			std::exit(-1);
		}
	}

	void sendToSrv(std::shared_ptr<stMessage>& p)
	{
		FastMutex::ScopedLock lock(m_mutex);
		m_socket.sendBytes(p->buffer, p->len);
		return;
	}
	
private:
	FastMutex   m_mutex;
	SocketAddress m_address;
	StreamSocket m_socket;
	NotificationQueue& m_queue;
	bool m_stop;
};

class InWorker: public Runnable
{
public:
	InWorker(const std::string& name, Client* pClientApp):
		m_name(name),
		m_pClientApp(pClientApp),
		m_stop(false)
	{
	}

	~InWorker()
	{
		m_stop = true;
	}

	void stop()
	{
		m_stop = true;
	}
	
	void run()
	{
		
		Poco::Timespan span(250000);
		while (!m_stop) {
			{
				std::shared_ptr<stMessage> p = std::make_shared<stMessage>();
				std::cin >> p->buffer;
				p->len = strlen(p->buffer);
				if (m_pClientApp != NULL) m_pClientApp->sendToSrv(p);
			}
		}
	}
	
private:
	std::string m_name;
	Client* m_pClientApp;
	bool m_stop;
};

int main(int argc, char** argv)
{
	NotificationQueue queue;

	//create outputWork thread
	OutWorker outputWork("outputWork", queue);
	ThreadPool::defaultPool().start(outputWork);

	//clientApp connect
	Client clientApp(SocketAddress("127.0.0.1", 7788), queue);
	if (clientApp.init() != 0) {
		return -1;
	}
	//create clientApp thread
	ThreadPool::defaultPool().start(clientApp);

	//create inputWork thread
	InWorker inputWork("inputWork", &clientApp);
	ThreadPool::defaultPool().start(inputWork);

	//std::cout << "TCP client. Press Ctrl-C to quit." << std::endl;
	Event terminator;
	terminator.wait();

	//stop inputWork thread
	inputWork.stop();
	//stop ClientApp thread
	clientApp.stop();
	//stop outputWork thread
	queue.wakeUpAll();

	ThreadPool::defaultPool().joinAll();

	return 0;
}



