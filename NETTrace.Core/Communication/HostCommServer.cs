using System;
using System.Collections.Generic;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NETTrace.Core.Communication {
    /// <summary>
    /// A simple asynchronous named pipe server to communicate with the profiler.
    /// </summary>
    public sealed class HostCommServer : IDisposable {
        private const int MaxMessageSize = 1024 * 1024;
        private const int MaxPipeCount = 100;

        private NamedPipeServerStream _pipeStream;
        private int _pipeId;
        private readonly byte[] _msgBuff;
        private bool _busy, _pipeCreated;

        public string HostPipeName => _pipeCreated ? $"NETTrace_comm{_pipeId}" : null;

        // Events
        public event EventHandler OnClientConnected;

        public event EventHandler OnClientDisconnected;
        public event EventHandler<TracerErrorEventArgs> OnTracerError;
        public event EventHandler<PipeMessageEventArgs> OnPipeMessageReceived;

        public HostCommServer() {
            _pipeId = 0;
            _msgBuff = new byte[MaxMessageSize];
        }

        private bool CreatePipe() {
            var openPipes = System.IO.Directory.GetFiles(@"\\.\pipe\");

            // Generate a pipe id that is unused
            while (openPipes.Contains($"\\\\.\\pipe\\NETTrace_comm{_pipeId}") &&
                   _pipeId < MaxPipeCount /* Something is seriously wrong if there are 100 opened pipes */)
                _pipeId++;

            if (_pipeId >= MaxPipeCount)
                throw new Exception("Could not create communication pipe.");

            try {
                _pipeStream = new NamedPipeServerStream(
                    $"NETTrace_comm{_pipeId}",
                    PipeDirection.InOut,
                    NamedPipeServerStream.MaxAllowedServerInstances,
                    PipeTransmissionMode.Message,
                    PipeOptions.Asynchronous);
            }
            catch {
                // TODO: useful output
                return false;
            }

            _pipeCreated = true;
            return true;
        }

        private async Task WaitForConnectionAsync() {
            await Task.Factory.FromAsync(_pipeStream.BeginWaitForConnection,
                _pipeStream.EndWaitForConnection,
                TaskCreationOptions.None
            ).ContinueWith(t => ListenForMessages());
        }

        private void ListenForMessages() {
            _pipeStream.ReadAsync(_msgBuff, 0, _msgBuff.Length).ContinueWith(t => {
                if (t.Result == 0) {
                    OnClientDisconnected?.Invoke(this, null);
                    return;
                }

                var decodedData = Encoding.Default.GetString(_msgBuff, 0, t.Result);
                var parsedMessage = PipeMessage.Deserialize(decodedData);

                switch (parsedMessage.MessageType) {
                    case MessageType.Init:
                        if (!_busy) {
                            _busy = true;
                            SendMessage(new PipeMessage(MessageType.Init, new Dictionary<string, object> {
                                {"error", PipeErrorCode.Success}
                            }));

                            OnClientConnected?.Invoke(this, null);
                        }
                        else
                            SendMessage(new PipeMessage(MessageType.Init, new Dictionary<string, object> {
                                {"error", PipeErrorCode.PipeBusy}
                            }));
                        break;
                    case MessageType.TracerError:
                        //TODO: error message
                        OnTracerError?.Invoke(this,
                            new TracerErrorEventArgs(
                                (PipeErrorCode) int.Parse((string) parsedMessage.MessageData["error"]), ""));
                        break;
                    default:
                        OnPipeMessageReceived?.Invoke(this, new PipeMessageEventArgs(parsedMessage));
                        break;
                }

                // Keep reading
                ListenForMessages();
            });
        }

        public async void Initialize() {
            if (!CreatePipe())
                throw new Exception();

            await WaitForConnectionAsync();
        }

        public void Dispose() {
            _pipeStream?.Dispose();
        }

        public async void SendMessage(PipeMessage msg) {
            var jsonBuf = Encoding.UTF8.GetBytes(msg.Serialize());
            await _pipeStream.WriteAsync(jsonBuf, 0, jsonBuf.Length);
        }
    }
}
