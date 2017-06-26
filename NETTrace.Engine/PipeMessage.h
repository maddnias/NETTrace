#pragma once

#include <map>
#include <memory>

typedef std::map<std::string, std::string> JsonStringMap;

class PipeMessage {
public:
	enum MessageType {
		TRACER_INIT,
		TRACER_ERROR,
		TRACER_JIT_COMPILATION_STARTED,
		TRACER_JIT_COMPILATION_FINISHED
	};

	enum PipeErrorCode {
		TRACER_ERR_SUCCESS,
		TRACER_ERR_PIPE_BUSY,
		TRACER_ERR_SETTINGS_NOT_FOUND,
		TRACER_ERR_SETTINGS_INVALID
	};

	PipeMessage();
	~PipeMessage();

	std::shared_ptr<std::string> getSerializedData() const;
	size_t getSerializedSize() const;

	MessageType getMessageType() const;
	JsonStringMap getDeserializedData() const;

	void deserialize(const std::string &serializedData);
	void serialize(const MessageType &type, const JsonStringMap &map = {});

private:
	std::shared_ptr<std::string> mSerializedData;
	JsonStringMap mDeserializedData;
	MessageType mMsgType;
};

