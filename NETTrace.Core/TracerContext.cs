using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NETTrace.Core.Auxiliary;
using NETTrace.Core.Communication;

namespace NETTrace.Core
{
    public class TracerContext
    {
        public HostCommServer CommServer { get; set; }
        public TracerBootstrapper Bootstrapper { get; set; }
    }
}
