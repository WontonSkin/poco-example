//
// tcpserver.cpp
//
// This sample demonstrates a TCP server.
//
// Copyright (c) 2005-2018, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/Socket.h"
#include "Poco/NumberParser.h"
#include "Poco/Logger.h"
#include "Poco/Process.h"
#include "Poco/NamedEvent.h"
#include "Poco/Mutex.h"
#include <iostream>
#include <map>


using Poco::Net::TCPServer;
using Poco::Net::TCPServerConnectionFilter;
using Poco::Net::TCPServerConnection;
using Poco::Net::TCPServerConnectionFactory;
using Poco::Net::TCPServerConnectionFactoryImpl;
using Poco::Net::StreamSocket;
using Poco::Net::Socket;
using Poco::UInt16;
using Poco::NumberParser;
using Poco::Logger;
using Poco::Event;
using Poco::NamedEvent;
using Poco::Process;
using Poco::ProcessImpl;
using Poco::Exception;
using Poco::FastMutex;


//ClientManager
class ClientManager
{
public:
	ClientManager():userNum(0)
	{
	}
	
	// add
	void add(std::string name, StreamSocket* s)
	{
		FastMutex::ScopedLock lock(m_mutex);
		
		auto search = m_User.find(name);
	    if (search != m_User.end()) {
	        std::cout << "Found " << search->first << '\n';
			std::cout << "some wrong happen.\n";
			return;
	    }
		
		m_User[name] = s;
		return;
	}
	
	// del
	void del(std::string name)
	{
		FastMutex::ScopedLock lock(m_mutex);

		auto search = m_User.find(name);
	    if (search == m_User.end()) {
	        std::cout << name << " not Found " << ", some wrong? \n";
			return;
	    }

		std::cout << "delete " << name << " success." << std::endl;
		m_User.erase(name);
		
		return;
	}
	
	// sendAll
	int sendToAllUser(char* buf, int len)
	{
		FastMutex::ScopedLock lock(m_mutex);

		for (auto& i : m_User) {
			i.second->sendBytes(buf, len);
		}
		
		return 0;
	}
	
	// get name
	std::string getName()
	{
		FastMutex::ScopedLock lock(m_mutex);
		userNum++;
		std::string name("user");
		name = name + std::to_string(userNum);

		//std::cout << "getName: " << name << std::endl;
		return name;
	}
private:
	FastMutex m_mutex;
	std::map<std::string, StreamSocket*> m_User;
	int userNum;
};


ClientManager* gpCliMng = new ClientManager;


class ClientConnection: public TCPServerConnection
{
public:
	ClientConnection(const StreamSocket& s): TCPServerConnection(s)
	{
	}

	void run()
	{
		StreamSocket& ss = socket();

		// generate name
		m_name = gpCliMng->getName();
		// add one user
		gpCliMng->add(m_name, &ss);
		// join in room
		std::string sayAddStr(m_name + " join in room.");
		gpCliMng->sendToAllUser(const_cast<char*>(sayAddStr.c_str()), sayAddStr.size());
		
		//say somethings
		Poco::Timespan span(250000);
		while (true) {
			if (ss.poll(span, Socket::SELECT_READ)) {
				try {
					char buffer[256];
					int n = ss.receiveBytes(buffer, sizeof(buffer));
					if (n <= 0) {
						std::cout << m_name << " errno: " << n << "<= 0." << std::endl;
						break;
					}

					std::string saySomeStr(m_name + " say: " + buffer);
					gpCliMng->sendToAllUser(const_cast<char*>(saySomeStr.c_str()), saySomeStr.size());
				} catch (Poco::Exception& exc) {
					std::cout << m_name  << " Exception: " << exc.displayText() << std::endl;
					break;
				} catch (...) {
					std::cout <<  m_name  << " some err." << std::endl;
					break;
				}
			}
		}

		// left room
		std::string sayLeftStr(m_name + " left room.");
		gpCliMng->del(m_name);
		gpCliMng->sendToAllUser(const_cast<char*>(sayLeftStr.c_str()), sayLeftStr.size());
		
	}

private:
	std::string m_name;
};

typedef TCPServerConnectionFactoryImpl<ClientConnection> TCPFactory;


int main(int argc, char** argv)
{
	try
	{
		Poco::UInt16 port = NumberParser::parse((argc > 1) ? argv[1] : "7788");

		TCPServer srv(new TCPFactory(), port);
		srv.start();

		std::cout << "TCP server listening on port " << port << '.' << std::endl;
		//std::cout << "Press Ctrl-C to quit." << std::endl;

		Event terminator;
		terminator.wait();
	}
	catch (Exception& exc)
	{
		std::cerr << exc.displayText() << std::endl;
		return 1;
	}

	return 0;
}
