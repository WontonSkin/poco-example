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
#include "Poco/Util/Application.h"
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


using namespace Poco::Net;
using Poco::Util::Application;
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
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		Application& app = Application::instance();
		try
		{
			WebSocket ws(request, response);
			app.logger().information("WebSocket connection established.");
			char buffer[1024];
			int flags;
			int n;
			do
			{
				n = ws.receiveFrame(buffer, sizeof(buffer), flags);
				app.logger().information(Poco::format("Frame received (length=%d, flags=0x%x).", n, unsigned(flags)));
				ws.sendFrame(buffer, n, flags);
			}
			while (n > 0 && (flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE);
			app.logger().information("WebSocket connection closed.");
		}
		catch (WebSocketException& exc)
		{
			app.logger().log(exc);
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
	}
};


class RequestHandlerFactory: public HTTPRequestHandlerFactory
{
public:
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
	{
		Application& app = Application::instance();
		app.logger().information("Request from " 
			+ request.clientAddress().toString()
			+ ": "
			+ request.getMethod()
			+ " "
			+ request.getURI()
			+ " "
			+ request.getVersion());
			
		for (HTTPServerRequest::ConstIterator it = request.begin(); it != request.end(); ++it)
		{
			app.logger().information(it->first + ": " + it->second);
		}
		
		if(request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0)
			return new WebSocketRequestHandler;
		else
			return new PageRequestHandler;
	}
};







//ClientManager
class ClientManager
{
public:
	ClientManager():m_userNum(0)
	{
	}
	
	// add
	void add(std::string name, StreamSocket* s)
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
			i.second->sendBytes(buf, len);
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
	std::map<std::string, StreamSocket*> m_user;
	int m_userNum;
};


ClientManager* gpCliMng = new ClientManager;


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

	void run_bak()
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

