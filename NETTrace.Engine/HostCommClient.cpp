#include "stdafx.h"
#include "HostCommClient.h"
#include "Constants.h"

using namespace boost::asio;
using namespace std;

HostCommClient::HostCommClient(io_service &io)
	: mPipe(make_shared<StreamHandle>(io)),
	mInitialized(false),
	mPipeId(0),
	mMsgBuf(maxTransferSize, '0') {
}

HostCommClient::~HostCommClient() {
	this->release();
}

void HostCommClient::run() {
	// Sanity check
	if (!this->mInitialized)
		throw exception("HostCommClient must be initialized before it can run.");

	this->mPipe->async_read_some(buffer(&mMsgBuf[0], mMsgBuf.size()), boost::bind(&HostCommClient::handleRead, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

bool HostCommClient::initialize() {
	const HANDLE &hPipe = this->createPipe();

	if (!hPipe /* == nullptr */)
		return false;

	DWORD mode = PIPE_READMODE_MESSAGE;
	bool pipeFlag = SetNamedPipeHandleState(
		hPipe,
		&mode,
		nullptr,
		nullptr
	);

	if (!pipeFlag) {
		DisconnectNamedPipe(hPipe);
		return false;
	}

	this->mPipe->assign(hPipe);

	string initBuff(255, '0');
	PipeMessage msg;
	msg.serialize(PipeMessage::MessageType::TRACER_INIT);
	sendMessage(msg);

	int read = mPipe->read_some(buffer(&initBuff[0], initBuff.size()));
	if (read == 0) {
		// Shouldn't happen
		this->release();
		throw exception("Failed to read INIT response.");
	}

	// We can reuse our allocated PipeMessage here
	msg.deserialize(initBuff.substr(0, read));
	JsonStringMap data = msg.getDeserializedData();
	auto errCode = static_cast<PipeMessage::PipeErrorCode>(stoi(data["error"]));

	if(errCode != PipeMessage::PipeErrorCode::TRACER_ERR_SUCCESS) {
		this->release();
		return false;
	}

	this->mInitialized = true;
	return true;
}

void HostCommClient::release() const {
	if (this->mPipe) {
		this->mPipe->cancel();
		this->mPipe->close();
	}
}

void HostCommClient::sendMessage(const PipeMessage &msg) const {
	// Sanity check for thread safety according to https://stackoverflow.com/a/14628366/7973038 
	// assuming implementation is still similar for pipes or the same
	assert(msg.getSerializedSize() < maxTransferSize);
	auto buf = buffer(msg.getSerializedData()->c_str(), msg.getSerializedSize());

	//TODO: async_write_some doesn't guarantee full delivery
	this->mPipe->async_write_some(buf,
		boost::bind(&HostCommClient::handleWrite,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void HostCommClient::sendMessageSync(const PipeMessage &msg) const {
	// Make sure message can be sent in one go
	assert(msg.getSerializedSize() < maxTransferSize);

	auto buf = buffer(msg.getSerializedData()->c_str(), msg.getSerializedSize());
	int sent = this->mPipe->write_some(buf);

	// Sanity check
	assert(sent == msg.getSerializedSize());
}

HANDLE HostCommClient::createPipe() {
	HANDLE hPipe;
	wstring hostPipeName = getHostPipeName();

	// Pipe shouldn't be busy, but just in case we wait until it's available
	while (true) {
		hPipe = ::CreateFile(hostPipeName.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED /* required for async operations */,
			nullptr);

		// We got a working handle, break
		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		if (GetLastError() != ERROR_PIPE_BUSY)
			return nullptr;

		if (!WaitNamedPipe(hostPipeName.c_str(), pipeWaitTimeout))
			return nullptr;
	}

	return hPipe;
}

void HostCommClient::handleWrite(boost::system::error_code err, size_t msgLen) {

}

void HostCommClient::handleRead(boost::system::error_code err, size_t msgLen) {
	shared_ptr<PipeMessage> msg = make_shared<PipeMessage>();
	msg->deserialize(this->mMsgBuf.substr(0, msgLen));

	// Notify listeners of a new message
	this->messageReceivedSignal(msg);

	// Keep reading
	this->mPipe->async_read_some(buffer(&mMsgBuf[0], mMsgBuf.size()) /* TODO: dangling ptr? */, 
		boost::bind(&HostCommClient::handleRead, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

wstring HostCommClient::getHostPipeName() {
	wchar_t *pipeNameBuf = nullptr;
	size_t strSize = 0;

	if(_wdupenv_s(&pipeNameBuf, &strSize, hostPipeEnvName) == 0 && pipeNameBuf) {
		return wstring(pipeNameBuf, strSize);
	}

	return nullptr;
}

