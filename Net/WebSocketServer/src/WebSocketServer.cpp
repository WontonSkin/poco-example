//
// WebSocketServer.cpp
//
// This sample demonstrates the WebSocket class.
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//

#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerConnection.h"
#include "Poco/Net/HTTPServerSession.h"
#include "Poco/Net/HTTPServerRequestImpl.h"
#include "Poco/Net/HTTPServerResponseImpl.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/NetException.h"
#include "Poco/Util/JSONConfiguration.h"
#include "Poco/Format.h"
#include "Poco/NumberParser.h"
#include "Poco/Logger.h"
#include "Poco/Process.h"
#include "Poco/NamedEvent.h"
#include "Poco/Mutex.h"
#include "Poco/NumberFormatter.h"
#include "Poco/Timestamp.h"
#include "Poco/Delegate.h"
#include <memory>
#include <iostream>
#include <map>

using namespace std;
using namespace Poco::Net;
using Poco::UInt16;
using Poco::NumberParser;
using Poco::Logger;
using Poco::Event;
using Poco::NamedEvent;
using Poco::Process;
using Poco::ProcessImpl;
using Poco::Exception;
using Poco::FastMutex;
using Poco::Timestamp;
using Poco::ThreadPool;
using Poco::AutoPtr;
using Poco::Util::JSONConfiguration;



//ClientManager
class WSClientManager
{
public:
	WSClientManager():m_userNum(0)
	{
	}
	
	// add
	void add(std::string name, WebSocket* s)
	{
		FastMutex::ScopedLock lock(m_mutex);
		
		auto search = m_user.find(name);
	    if (search != m_user.end()) {
	        std::cout << "Found " << search->first << '\n';
			std::cout << "some wrong happen.\n";
			return;
	    }
		
		m_user[name] = s;
		return;
	}
	
	// del
	void del(std::string name)
	{
		FastMutex::ScopedLock lock(m_mutex);

		auto search = m_user.find(name);
	    if (search == m_user.end()) {
	        std::cout << name << " not Found " << ", some wrong? \n";
			return;
	    }

		std::cout << "delete " << name << " success." << std::endl;
		m_user.erase(name);
		
		return;
	}
	
	// sendAll
	int sendToAllUser(char* buf, int len)
	{
		FastMutex::ScopedLock lock(m_mutex);

		for (auto& i : m_user) {
			i.second->sendFrame(buf, len);
		}
		
		return 0;
	}
	
	// get name
	std::string getName()
	{
		FastMutex::ScopedLock lock(m_mutex);
		m_userNum++;
		std::string name("user");
		name = name + std::to_string(m_userNum);

		//std::cout << "getName: " << name << std::endl;
		return name;
	}
private:
	FastMutex m_mutex;
	std::map<std::string, WebSocket*> m_user;
	int m_userNum;
};


WSClientManager* gpCliMng = new WSClientManager;



class PageRequestHandler: public HTTPRequestHandler
	/// Return a HTML document with some JavaScript creating
	/// a WebSocket connection.
{
public:
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		response.setChunkedTransferEncoding(true);
		response.setContentType("text/html");
		std::ostream& ostr = response.send();
		ostr << "<html>";
		ostr << "<head>";
		ostr << "<title>WebSocketServer</title>";
		ostr << "<script type=\"text/javascript\">";
		ostr << "function WebSocketTest()";
		ostr << "{";
		ostr << "  if (\"WebSocket\" in window)";
		ostr << "  {";
		ostr << "    var ws = new WebSocket(\"ws://" << request.serverAddress().toString() << "/ws\");";
		ostr << "    ws.onopen = function()";
		ostr << "      {";
		ostr << "        ws.send(\"Hello, world!\");";
		ostr << "      };";
		ostr << "    ws.onmessage = function(evt)";
		ostr << "      { ";
		ostr << "        var msg = evt.data;";
		ostr << "        alert(\"Message received: \" + msg);";
		ostr << "        ws.close();";
		ostr << "      };";
		ostr << "    ws.onclose = function()";
		ostr << "      { ";
		ostr << "        alert(\"WebSocket closed.\");";
		ostr << "      };";
		ostr << "  }";
		ostr << "  else";
		ostr << "  {";
		ostr << "     alert(\"This browser does not support WebSockets.\");";
		ostr << "  }";
		ostr << "}";
		ostr << "</script>";
		ostr << "</head>";
		ostr << "<body>";
		ostr << "  <h1>WebSocket Server</h1>";
		ostr << "  <p><a href=\"javascript:WebSocketTest()\">Run WebSocket Script</a></p>";
		ostr << "</body>";
		ostr << "</html>";
	}
};


class WebSocketRequestHandler: public HTTPRequestHandler
	/// Handle a WebSocket connection.
{
public:

	void doSomethings(WebSocket& ws, const HTTPServerParams& param)
	{
		AutoPtr<JSONConfiguration> pConf = new JSONConfiguration;
	
		// generate name
		m_name = gpCliMng->getName();
		// add one user
		gpCliMng->add(m_name, &ws);
		
		// join in room
		std::string sayAddStr(m_name + " join in room.");
		pConf->setString("type", "enter");
		pConf->setString("data", sayAddStr);
		ostringstream ossSayAddStr;
		pConf->save(ossSayAddStr, 0);
		gpCliMng->sendToAllUser(const_cast<char*>(ossSayAddStr.str().c_str()), ossSayAddStr.str().size());
		
		//say somethings
		while (true) {
			if (ws.poll(param.getKeepAliveTimeout(), Socket::SELECT_READ)) {
				try 
				{
					char buffer[1024] = {0};
					int flags;
					int n = ws.receiveFrame(buffer, sizeof(buffer), flags);
					cout << Poco::format("Frame received (length=%d, flags=0x%x).", n, unsigned(flags)) << endl;
					if (n <= 0 || ((flags & WebSocket::FRAME_OP_BITMASK) == WebSocket::FRAME_OP_CLOSE)) {
						std::cout << m_name << " errno: " << n << "<= 0." << std::endl;
						break;
					}

					std::string saySomeStr(m_name + " say: " + buffer);
					pConf->setString("type", "message");
					pConf->setString("data", saySomeStr);
					ostringstream ossSaySomeStr;
					pConf->save(ossSaySomeStr, 0);
					gpCliMng->sendToAllUser(const_cast<char*>(ossSaySomeStr.str().c_str()), ossSaySomeStr.str().size());
				} catch (Poco::Exception& exc){
					std::cout << m_name  << " Exception: " << exc.displayText() << std::endl;
					break;
				} catch (...) {
					std::cout <<  m_name  << " some err." << std::endl;
					break;
				}
			}
		}

		cout << m_name <<" left room.\n";

		// left room
		std::string sayLeftStr(m_name + " left room.");
		pConf->setString("type", "leave");
		pConf->setString("data", sayLeftStr);
		ostringstream ossLeftStr;
		pConf->save(ossLeftStr, 0);
		gpCliMng->del(m_name);
		gpCliMng->sendToAllUser(const_cast<char*>(ossLeftStr.str().c_str()), ossLeftStr.str().size());

		return;
	}

	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
					
		try
		{	
			const HTTPServerParams& param = request.serverParams();
			WebSocket ws(request, response);
			cout<< "WebSocket connection established.\n";
			doSomethings(ws, param);
			cout << "WebSocket connection closed.\n";
		}
		catch (Exception& exc)
		{
			cout << exc.displayText() <<endl;
			switch (exc.code())
			{
			case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
				response.set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);
				// fallthrough
			case WebSocket::WS_ERR_NO_HANDSHAKE:
			case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
			case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
				response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentLength(0);
				response.send();
				break;
			}
		}
		catch (...) {
			cout << "some unknow exception happen.\n";
		}
	}

private:
	string m_name;
};


class RequestHandlerFactory: public HTTPRequestHandlerFactory
{
public:
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
	{
		cout << "Request from " 
			+ request.clientAddress().toString()
			+ ": "
			+ request.getMethod()
			+ " "
			+ request.getURI()
			+ " "
			+ request.getVersion() << endl;
			
		for (HTTPServerRequest::ConstIterator it = request.begin(); it != request.end(); ++it)
		{
			cout << it->first + ": " + it->second << endl;
		}
		
		if(request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0)
			return new WebSocketRequestHandler;
		else
			return new PageRequestHandler;
	}
};


class ClientConnection: public TCPServerConnection
{
public:
	ClientConnection(const StreamSocket& s): TCPServerConnection(s)
	{
		m_pParams = new HTTPServerParams;
		m_pFactory = new RequestHandlerFactory;
		m_stopped = false;
	}

	void sendErrorResponse(HTTPServerSession& session, HTTPResponse::HTTPStatus status)
	{
		HTTPServerResponseImpl response(session);
		response.setVersion(HTTPMessage::HTTP_1_1);
		response.setStatusAndReason(status);
		response.setKeepAlive(false);
		response.send();
		session.setKeepAlive(false);
	}

	void run()
	{
		std::string server = m_pParams->getSoftwareVersion();
		HTTPServerSession session(socket(), m_pParams);
		while (!m_stopped && session.hasMoreRequests()) {
			try {
				Poco::FastMutex::ScopedLock lock(m_mutex);
				if (!m_stopped) {
					HTTPServerResponseImpl response(session);
					HTTPServerRequestImpl request(response, session, m_pParams);
	
					Poco::Timestamp now;
					response.setDate(now);
					response.setVersion(request.getVersion());
					response.setKeepAlive(m_pParams->getKeepAlive() && request.getKeepAlive() && session.canKeepAlive());
					if (!server.empty()) {
						response.set("Server", server);
					}
					try {
						std::unique_ptr<HTTPRequestHandler> pHandler(m_pFactory->createRequestHandler(request));
						if (pHandler.get()) {
							if (request.getExpectContinue() && response.getStatus() == HTTPResponse::HTTP_OK) {
								response.sendContinue();
							}
	
							pHandler->handleRequest(request, response);
							session.setKeepAlive(m_pParams->getKeepAlive() && response.getKeepAlive() && session.canKeepAlive());
						} else {
							sendErrorResponse(session, HTTPResponse::HTTP_NOT_IMPLEMENTED);
						}
					} catch (Poco::Exception&){
						if (!response.sent()) {
							try
							{
								sendErrorResponse(session, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
							}
							catch (...)
							{
							}
						}
						throw;
					}
				}
			}catch (NoMessageException&) {
				break;
			}catch (MessageException&) {
				sendErrorResponse(session, HTTPResponse::HTTP_BAD_REQUEST);
			}catch (Poco::Exception&) {
				if (session.networkException()) {
					session.networkException()->rethrow();
				}
				else throw;
			}
		}
	}

private:
	std::string m_name;
	HTTPServerParams::Ptr          m_pParams;
	HTTPRequestHandlerFactory::Ptr m_pFactory;
	bool m_stopped;
	Poco::FastMutex m_mutex;
};

typedef TCPServerConnectionFactoryImpl<ClientConnection> TCPFactory;


int main(int argc, char** argv)
{
	try
	{
		Poco::UInt16 port = NumberParser::parse((argc > 1) ? argv[1] : "9980");

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

