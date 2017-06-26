using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace NETTrace.Core.Communication {
    public sealed class PipeMessage {
        [JsonProperty("type")]
        public MessageType MessageType { get; private set; }

        [JsonProperty("data")]
        public Dictionary<string, object> MessageData { get; private set; }

        internal PipeMessage() {
            MessageData = new Dictionary<string, object>();
        }

        internal PipeMessage(MessageType messageType, Dictionary<string, object> messageData) {
            MessageType = messageType;
            MessageData = messageData;
        }

        public string Serialize() {
            return JsonConvert.SerializeObject(this);
        }

        //TODO: potential too severe bottleneck, might have to use a more efficient JSON decoding library
        internal static PipeMessage Deserialize(string rawJson) {
            var currProp = string.Empty;
            var msg = new PipeMessage();

            // Use JsonTextReader instead of DeserializeObject for improved performance
            using (var strReader = new StringReader(rawJson))
            using (var jsonReader = new JsonTextReader(strReader)) {
                var flag = jsonReader.Read();
                if (!flag || jsonReader.TokenType != JsonToken.StartObject)
                    throw new Exception("Malformed or unknown JSON received.");

                // MessageType property name
                flag = jsonReader.Read();
                Debug.Assert(jsonReader.TokenType == JsonToken.PropertyName);
                Debug.Assert((string) jsonReader.Value == "type");

                // MessageType value
                flag = jsonReader.Read();
                Debug.Assert(jsonReader.TokenType == JsonToken.String);
                msg.MessageType = (MessageType) int.Parse((string)jsonReader.Value);

                // Data property name
                flag = jsonReader.Read();
                // Data value
                flag = jsonReader.Read();

                // If there are no values in Data, return
                if (jsonReader.TokenType != JsonToken.StartObject)
                    return msg;

                while (jsonReader.Read()) {
                    if (jsonReader.Value == null)
                        continue;

                    switch (jsonReader.TokenType) {
                        case JsonToken.PropertyName:
                            currProp = jsonReader.Value.ToString();
                            break;
                        default:
                            msg.MessageData.Add(currProp, jsonReader.Value);
                            break;
                    }
                }
            }

            return msg;
        }
    }
}
