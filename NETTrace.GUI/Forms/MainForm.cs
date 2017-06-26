using System;
using System.Windows.Forms;
using NETTrace.Core;
using NETTrace.Core.Auxiliary;
using NETTrace.Core.Communication;

namespace NETTrace.GUI.Forms
{
    public partial class MainForm : Form
    {
        private HostCommServer _srv;
        public MainForm()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e) {
            var bootstrapper = new TracerBootstrapper();
            var ctx = bootstrapper.LaunchTracerSuspended("NETTrace.Test.exe");
            ctx.CommServer.OnPipeMessageReceived += CommServer_OnPipeMessageReceived;
            ctx.CommServer.OnTracerError += CommServer_OnTracerError;
            ctx.Bootstrapper.ResumeTracer();
        }

        private void CommServer_OnTracerError(object sender, TracerErrorEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void CommServer_OnPipeMessageReceived(object sender, PipeMessageEventArgs e) {
            switch (e.Message.MessageType) {
                case MessageType.Init:
                    break;
                case MessageType.JitCompilationStarted:
                    treeView1.Invoke((MethodInvoker) delegate {
                        var node = treeView1.Nodes.Add("[Event: JitCompilationStarted]");
                        foreach (var itm in e.Message.MessageData)
                            node.Nodes.Add($"{itm.Key}: {itm.Value}");
                        node.ExpandAll();
                    });
                    break;
                case MessageType.JitCompilationFinished:
                    treeView1.Invoke((MethodInvoker)delegate {
                        var node = treeView1.Nodes.Add("[Event: JitCompilationFinished]");
                        foreach (var itm in e.Message.MessageData)
                            node.Nodes.Add($"{itm.Key}: {itm.Value}");
                        node.ExpandAll();
                    });
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private void Srv_OnPipeMessageReceived(object sender, PipeMessageEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void Srv_OnClientDisconnected(object sender, EventArgs e)
        {
            throw new NotImplementedException();
        }

        private void Srv_OnClientConnected(object sender, EventArgs e)
        {
            throw new NotImplementedException();
        }

        private void _srv_OnClientDisconnected(object sender, EventArgs e)
        {
            MessageBox.Show("Disconnected");
        }

        private void _srv_OnPipeMessageReceived(object sender, PipeMessageEventArgs e)
        {
        //    MessageBox.Show(e.Message.MessageType);
        }

        private void _srv_OnClientConnected(object sender, EventArgs e)
        {

        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }
    }
}
