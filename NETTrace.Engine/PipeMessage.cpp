#include "stdafx.h"
#include "PipeMessage.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/json_parser/detail/wide_encoding.hpp>
#include <boost/iostreams/stream.hpp>

using namespace boost::property_tree;
using namespace std;

PipeMessage::PipeMessage()
	: mMsgType(TRACER_INIT) {
}


PipeMessage::~PipeMessage() {
}

shared_ptr<string> PipeMessage::getSerializedData() const {
	assert(mSerializedData->length() != 0);
	return this->mSerializedData;
}

size_t PipeMessage::getSerializedSize() const {
	return this->mSerializedData->length();
}

PipeMessage::MessageType PipeMessage::getMessageType() const {
	return this->mMsgType;
}

JsonStringMap PipeMessage::getDeserializedData() const {
	return this->mDeserializedData;
}

void PipeMessage::deserialize(const string &serializedData) {
	/* 
	Pipe message use a very simple format:

	{
		"type": string,
		"data": {
			"key1": value (string),
			"key2": value (string),
			etc ...
		}
	}
	*/

	// https://stackoverflow.com/a/37712933/7973038
	boost::iostreams::stream<boost::iostreams::array_source> stream(serializedData.c_str(), serializedData.size());
	ptree tree;
	read_json(stream, tree);

	this->mMsgType = static_cast<MessageType>(stoi(tree.get_child("type").data()));

	for (const auto& itm : tree.get_child("data"))
		this->mDeserializedData[itm.first] = itm.second.data();
}

void PipeMessage::serialize(const MessageType &type, const JsonStringMap &map) {
	ptree tree;

	tree.put("type", to_string(type));
	tree.put("data", "");

	for (const auto &entry : map)
		tree.put("data." + entry.first, entry.second);

	ostringstream outBuf;
	write_json(outBuf, tree, false);
	this->mSerializedData = make_shared<string>(outBuf.str().c_str());
}
