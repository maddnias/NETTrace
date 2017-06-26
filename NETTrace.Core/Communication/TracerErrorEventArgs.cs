using System;

namespace NETTrace.Core.Communication
{
    public sealed class TracerErrorEventArgs : EventArgs
    {
        public PipeErrorCode ErrorCode { get; }
        public string Message { get; }

        public TracerErrorEventArgs(PipeErrorCode errorCode, string message) {
            ErrorCode = errorCode;
            Message = message;
        }
    }
}
