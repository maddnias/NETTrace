#pragma once

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include "PipeMessage.h"

typedef boost::asio::windows::stream_handle StreamHandle;

class HostCommClient {
private:
	std::shared_ptr<StreamHandle> mPipe;
	bool mInitialized;
	int mPipeId;
	std::string mMsgBuf;

	static HANDLE createPipe();
	static void handleWrite(boost::system::error_code err, size_t msgLen);
	void handleRead(boost::system::error_code err, size_t msgLen);
	static std::wstring getHostPipeName();

public:
	boost::signals2::signal<void(std::shared_ptr<PipeMessage>)> messageReceivedSignal;

	HostCommClient(boost::asio::io_service &io);
	virtual ~HostCommClient();

	void run();
	bool initialize();
	void release() const;

	void sendMessage(const PipeMessage &msg) const;
	// Sends a message synchronously
	void sendMessageSync(const PipeMessage &msg) const;
};

