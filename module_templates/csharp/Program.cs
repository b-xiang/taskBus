/**
 * C# template by goldenhawking
 * C#范例，演示接入taskBus的最小步骤
 * @date 2017-11-19
 * @author goldenhawking
 */
using System;
using System.Threading;
using System.Collections.Generic;
using System.Linq;

namespace RandomSource
{
    class Program
    {
        /*! C# example demos how to read and write stdin, out in C#
         *  We will output a random sig for stdout, and listen for stdin with nothing to do except for
         *  package building.
         */
        static void Main(string[] args)
        {
            uint instance_id = 0;
            uint chaos_id = 0;
            uint total_path = 8;
            uint path_rate = 1024 * 128;
            //!解释命令行
            //!read cmdline 
            foreach (string strpar in args)
            {
                int nidx = -1;
                if (0 <= (nidx = strpar.IndexOf("--instance=")))
                    instance_id = System.Convert.ToUInt32(strpar.Substring(nidx + 11));
                else if (0 <= (nidx = strpar.IndexOf("--rand=")))
                    chaos_id = System.Convert.ToUInt32(strpar.Substring(nidx + 7));
                else if (0 <= (nidx = strpar.IndexOf("--total_path=")))
                    total_path = System.Convert.ToUInt32(strpar.Substring(nidx + 13));
                else if (0 <= (nidx = strpar.IndexOf("--path_rate=")))
                    path_rate = System.Convert.ToUInt32(strpar.Substring(nidx + 12));
            }
            //Open stdout          
            System.IO.Stream stdout = System.Console.OpenStandardOutput();
            if (chaos_id == 0)
                return;
            //Open a new thread for data reading
            //创建ThreadTest类的一个实例
            ThreadRead stdinReader = new ThreadRead();
            Thread thread_reader = new Thread(new ThreadStart(stdinReader.ReadStdin));
            stdinReader.prmsg("Started!");
            uint bytesPer50ms = path_rate / 50;
            byte[] buf = new byte[bytesPer50ms+16];
            thread_reader.Start();
            System.Random rndeng = new System.Random();
            int clocks = 0;
            double pys = 0;
            int pathsel = 0;
            while (!stdinReader.finished())
            {
                for (int p = 0; p < total_path; ++p)
                {
                    for (int i = 0; i < bytesPer50ms; ++i)
                    {
                        if (clocks % 16 ==0)
                            pathsel = rndeng.Next() & 0x01;
                        if (pathsel == 0)
                            buf[i + 16] = (byte)(System.Math.Cos(pys) * 120.0);
                        else
                            buf[i + 16] = (byte)(System.Math.Cos(pys + 3.1415927) * 120.0);
                        ++clocks;
                        pys += 3.1415927 / 8;
                    }
                    buf[0] = 0x3C; buf[1] = 0x5A; buf[2] = 0x7E; buf[3] = 0x69;
                    buf[4] = (byte)(chaos_id & 0x0ff); 
                    buf[5] = (byte)((chaos_id>>8) & 0x0ff);
                    buf[6] = (byte)((chaos_id>>16) & 0x0ff); 
                    buf[7] = (byte)((chaos_id >> 24) & 0x0ff);
                    buf[8] = (byte)(p & 0x0ff); 
                    buf[9] = (byte)((p >> 8) & 0x0ff);
                    buf[10] = (byte)((p >> 16) & 0x0ff); 
                    buf[11] = (byte)((p >> 24) & 0x0ff);
                    buf[12] = (byte)(bytesPer50ms & 0x0ff); 
                    buf[13] = (byte)((bytesPer50ms >> 8) & 0x0ff);
                    buf[14] = (byte)((bytesPer50ms >> 16) & 0x0ff); 
                    buf[15] = (byte)((bytesPer50ms >> 24) & 0x0ff);
                    stdout.Write(buf, 0, (int)bytesPer50ms+16);
                    System.Threading.Thread.Sleep(48);
                }                
            }
            
        }
    }
    /*
     *  我们在一个独立的线程中进行阻塞读取 stdin
     *  C#对stdin的处理比起其他语言要复杂很多。
     *  We will read data from stdin with this thread.
     *  Reading data from redirected stdin is surprisingly challenging.
     *  @author goldenhawking
     */
    class ThreadRead
    {
        private bool m_bQuit = false;
        public bool finished()
        {            
            return m_bQuit;
        }

        public void prmsg(string strmsg)
        {
            System.IO.Stream stderr = Console.OpenStandardError();
            byte[] byteMsg = System.Text.Encoding.Default.GetBytes(strmsg);
            List<byte> lst = new List<byte>(byteMsg);
            stderr.Write(byteMsg, 0, lst.Count);
            stderr.Flush();
        }

        public void deal_package(int sub, int path, int len, byte[] package)
        {
            if (sub==-1)
            {
                if (len >= 5)
                {
                    if (package[1] == 'q' && package[2] == 'u' && package[3] == 'i' && package[4] == 't')
                    {
                        m_bQuit = true;
                        prmsg("Quit!\n");
                    }
                }
            }
            //prmsg("Sub "+sub.ToString()+ ",Path " + path.ToString()+ ",Length " + len.ToString());
        }
        /** 从C#读取重定向的控制台真的是一个有挑战性的工作。感谢Dave Glick的尝试。
         *  Thks for this blog, that reading stdin from c# is a quite difficult work.
         *@ref https://daveaglick.com/posts/capturing-standard-input-in-csharp
         */
        public byte[] read_from_stdin(int size)
        {
            byte[] buffer = new byte[size];
            int red = 0;
            System.IO.Stream stream = Console.OpenStandardInput();
            int read = -1;
            AutoResetEvent gotInput = new AutoResetEvent(false);
            while (read< size)
            {
                Thread inputThread = new Thread(() =>
                {
                    try
                    {
                        read = stream.Read(buffer, red, buffer.Length - red);
                        red += read;
                        gotInput.Set();
                    }
                    catch (ThreadAbortException)
                    {
                        Thread.ResetAbort();
                    }
                })
                {
                    IsBackground = true
                };
                inputThread.Start();
                // Timeout expired?
                if (!gotInput.WaitOne(100))
                    inputThread.Abort();                    
            }
            return buffer;
        }

        public void ReadStdin()
        {
            do
            {
                byte[] bf_red = read_from_stdin(4);
                if (bf_red[0] != 0x3C || bf_red[1] != 0x5A || bf_red[2] != 0x7E || bf_red[3] != 0x69)
                    continue;
                int subject = 0, path = 0, length = 0;
                bf_red = read_from_stdin(4);
                subject += (bf_red[0] << 0); subject += (bf_red[1] << 8);
                subject += (bf_red[2] << 16); subject += (bf_red[3] << 24);
                bf_red = read_from_stdin(4);
                path += (bf_red[0] << 0); path += (bf_red[1] << 8);
                path += (bf_red[2] << 16); path += (bf_red[3] << 24);
                bf_red = read_from_stdin(4);
                length += (bf_red[0] << 0); length += (bf_red[1] << 8);
                length += (bf_red[2] << 16); length += (bf_red[3] << 24);              
                bf_red = read_from_stdin(length);
                deal_package(subject, path, length, bf_red);
                
            } while (m_bQuit == false);           
        }        
    }
}
