using System;

namespace NETTrace.Core.Communication {
    public sealed class PipeMessageEventArgs : EventArgs {
        public PipeMessage Message { get; set; }

        public PipeMessageEventArgs(PipeMessage message) {
            Message = message;
        }
    }
}
