// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef UDP_MUDUO_NET_TCPCONNECTION_H
#define UDP_MUDUO_NET_TCPCONNECTION_H


#include <base/StringPiece.h>
#include <base/Types.h>
#include "UdpCallbacks.h"
#include <net/Callbacks.h>
#include <net/Buffer.h>
#include <net/InetAddress.h>
#include <net/TimerId.h>

#include <memory>

#include "any.h"
#include <kcpp/kcpp.h>

namespace muduo
{
	namespace net
	{

		class Channel;
		class EventLoop;
		class Socket;

		class UdpConnection : noncopyable,
			public std::enable_shared_from_this<UdpConnection>
		{
		public:
			/// Constructs a UdpConnection with a connected sockfd
			///
			/// User should not create this object.
			UdpConnection(const kcpp::RoleTypeE role,
				EventLoop* loop,
				const string& name,
				Socket* connectedSocket,
				int ConnectionId,
				const InetAddress& localAddr,
				const InetAddress& peerAddr);

			~UdpConnection();

			enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

			EventLoop* getLoop() const { return loop_; }
			const string& name() const { return name_; }
			const InetAddress& localAddress() const { return localAddr_; }
			const InetAddress& peerAddress() const { return peerAddr_; }

			bool connected() const { return state_ == kConnected; }
			bool disconnected() const { return state_ == kDisconnected; }

			bool canSend() const
			{ 
				if (kcpSession_)
					return kcpSession_->CheckCanSend();
				return true;
			}

			// void send(string&& message); // C++11

			void send(const void* data, int len,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable)
			{ send(StringPiece(static_cast<const char*>(data), len), transmitMode); }

			void send(const StringPiece& message,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable);
			//{ send(message.data(), message.size()); }

			// FIXME efficiency!!!
			void send(Buffer* buf,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable);
			//{ send(buf->peek(), static_cast<int>(buf->readableBytes())); }

			void shutdown(); // NOT thread safe, no simultaneous calling
							 // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
			void forceClose();
			//void forceCloseWithDelay( double seconds );

			// reading or not
			void startRead();
			void stopRead();
			bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

			void setConnectionCallback( const UdpConnectionCallback& cb )
			{ connectionCallback_ = cb; }

			void setMessageCallback( const UdpMessageCallback& cb )
			{ messageCallback_ = cb; }

			void setWriteCompleteCallback( const UdpWriteCompleteCallback& cb )
			{ writeCompleteCallback_ = cb; }

			/// Internal use only.
			void setCloseCallback( const UdpCloseCallback& cb )
			{ closeCallback_ = cb; }

			// called when TcpServer accepts a new connection
			void connectEstablished();   // should be called only once
												 // called when TcpServer has removed me from its map
			void connectDestroyed();  // should be called only once

			void setContext(const realtinet::any& context)
			{ context_ = context; }

			const realtinet::any& getContext() const
			{ return context_; }

			realtinet::any* getMutableContext()
			{ return &context_; }

			int GetConnId() const
			{ return connId_; }

		private:

			void kcpSend(const void* message, int len,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable);

			void onKcpsessConnection(std::deque<std::string>* pendingSendDataDeque);

			void KcpSessionUpdate();
			void DoSend(const void* message, int len);
			kcpp::UserInputData DoRecv();

			void handleRead( Timestamp receiveTime);
			void handleClose();
			void handleError();
			// void sendInLoop(string&& message);
			void sendInLoop(const StringPiece& message, kcpp::TransmitModeE transmitMode);
			void sendInLoop(const void* message, size_t len, kcpp::TransmitModeE transmitMode);
			// void shutdownAndForceCloseInLoop(double seconds);
			void forceCloseInLoop();
			void setState( StateE s ) { state_ = s; }
			const char* stateToString() const;
			void startReadInLoop();
			void stopReadInLoop();

			EventLoop* loop_;
			const string name_;
			StateE state_;  // FIXME: use atomic variable
			bool reading_;
			// we don't expose those classes to client.
			std::unique_ptr<Socket> socket_;
			std::unique_ptr<Channel> channel_;
			const InetAddress localAddr_;
			const InetAddress peerAddr_;
			UdpConnectionCallback connectionCallback_;
			UdpMessageCallback messageCallback_;
			UdpWriteCompleteCallback writeCompleteCallback_;
			UdpCloseCallback closeCallback_;

			// new
			int connId_;
			static const size_t kPacketBufSize = 1500;
			char packetBuf_[kPacketBufSize];
			realtinet::any context_;

			Buffer inputBuffer_;
			kcpp::Buf kcpsessRcvBuf_;

			// kcp
			std::shared_ptr<kcpp::KcpSession> kcpSession_;
			TimerId curKcpsessUpTimerId_;
			//bool isCliKcpsessConned_;
		};

		typedef std::shared_ptr<UdpConnection> UdpConnectionPtr;

	}
}
#endif  // MUDUO_NET_TCPCONNECTION_H
