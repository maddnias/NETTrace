using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using NETTrace.Core.Communication;

namespace NETTrace.Core.Auxiliary {
    public sealed class TracerBootstrapper {
        public Win32.ProcessInfo TracedProcessInfo { get; private set; }

        private const string ProfilerClsId = "{8356E05C-ED29-43D2-8EAD-06BEBA7DF8A3}";

        private readonly string[] _tracerEngineProbePaths = {
            "",
            "bin"
        };

        private string GetTracerEnginePath() {
            return _tracerEngineProbePaths.Select(path => Path.Combine(path, "NETTrace.Engine.dll"))
                .FirstOrDefault(File.Exists);
        }

        private void PrepareEnvironment(string targetFile, string pipeName, out Win32.ProcessInfo procInfo,
            out Win32.StartupInfo startupInfo) {
            var tracerPath = GetTracerEnginePath();
            if (tracerPath == null)
                throw new Exception("Tracer engine could not be found.");

            var envBlock = new StringBuilder();

            envBlock.Append("COR_ENABLE_PROFILING=1\0");
            envBlock.Append($"COR_PROFILER={ProfilerClsId}\0");
            envBlock.Append($"COR_PROFILER_PATH={tracerPath}\0");
            // We need to tell the tracer what our pipename is
            envBlock.Append($"NETTrace_pipe_name=\\\\.\\pipe\\{pipeName}\0");
            // We also need to tell the tracer where to find the settings file
            envBlock.Append(
                // ReSharper disable once AssignNullToNotNullAttribute
                $"NETTrace_settings_file=\"{Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "settings.json")}\"\0");

            var creationFlag = Win32.CreateProcess(targetFile, null, IntPtr.Zero, IntPtr.Zero, true, 0x00000004,
                envBlock, null, out startupInfo, out procInfo);

            if (!creationFlag)
                throw new Exception("Could not create process");
        }

        public TracerContext LaunchTracer(string targetAssembly) {
            var ctx = LaunchTracerSuspended(targetAssembly);
            if (!ctx.Bootstrapper.ResumeTracer())
                throw new Exception("Failed to resume traced process");

            return ctx;
        }

        public bool ResumeTracer() {
            var result = Win32.ResumeThread(TracedProcessInfo.hThread);
            return result != -1;
        }

        public TracerContext LaunchTracerSuspended(string targetAssembly) {
            if (!File.Exists(targetAssembly))
                throw new FileNotFoundException();

            var hostCommServer = new HostCommServer();
            hostCommServer.Initialize();

            var procInfo = new Win32.ProcessInfo();

            try {
                Win32.StartupInfo startupInfo;
                PrepareEnvironment(targetAssembly, hostCommServer.HostPipeName, out procInfo, out startupInfo);
            }
            catch(Exception e) {
            }

            TracedProcessInfo = procInfo;

            return new TracerContext {
                Bootstrapper = this,
                CommServer = hostCommServer
            };
        }
    }
}
