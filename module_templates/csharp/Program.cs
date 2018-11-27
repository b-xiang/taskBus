/**
 * C# template by goldenhawking
 * C#范例，演示接入taskBus的最小步骤
 * @date 2017-11-19
 * @author goldenhawking
 */
using System;
using System.Threading;
using System.Collections.Generic;
namespace RandomSource
{
    class Program
    {
        /*!
         * 
         * 
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
            //Open stdout, strErr
            System.IO.Stream stderr = Console.OpenStandardError();
            System.IO.Stream stdout = System.Console.OpenStandardOutput();
            if (chaos_id == 0)
                return;
            //Open a new thread for data reading
            //创建ThreadTest类的一个实例
            ThreadRead stdinReader = new ThreadRead();
            Thread thread_reader = new Thread(new ThreadStart(stdinReader.ReadStdin));

            bool bfinished = false;
            uint bytesPer50ms = path_rate / 50;
            byte[] buf = new byte[bytesPer50ms+16];

            System.Random rndeng = new System.Random();
            int clocks = 0;
            double pys = 0;
            int pathsel = 0;
            while (!bfinished)
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

    class ThreadRead
    {
        public bool m_bQuit = false;
        public void deal_package(int sub, int path, int len, List<byte> package)
        {
            
        }

        public void ReadStdin()
        {
            const int buffer_size = 65536;
            List<byte> listBuffer =  new List<byte>();
            System.IO.Stream std_in = Console.OpenStandardInput();
            byte [] bf_red = new byte [buffer_size];
            do
            {
                int red = std_in.Read(bf_red,0,buffer_size);
                for (int i=0;i<red;++i)
                    listBuffer.Add(bf_red[i]);   
                if (red == 0)
                {
                    Thread.Sleep(100);
                    continue;
                }                
                while (listBuffer.Count>=4 && m_bQuit==false)
                {
                    //Move header to first byte
                    if (listBuffer[0] != 0x3C || listBuffer[1] != 0x5A || listBuffer[2] != 0x7e || listBuffer[3] != 0x69)
                    {
                        listBuffer.RemoveAt(0);
                        continue;
                    }
                    //read length
                    if (listBuffer.Count < 16)
                        break;
                    int subject = 0, path = 0, length = 0;

                    subject += (listBuffer[4] << 0); subject += (listBuffer[5] << 8);
                    subject += (listBuffer[6] << 16); subject += (listBuffer[7] << 24);

                    path += (listBuffer[8] << 0); path += (listBuffer[9] << 8);
                    path += (listBuffer[10] << 16); path += (listBuffer[11] << 24);

                    length += (listBuffer[12] << 0); length += (listBuffer[13] << 8);
                    length += (listBuffer[14] << 16); length += (listBuffer[15] << 24);

                    if (listBuffer.Count < 16 + length)
                        break;
                    //Deal 
                    deal_package(subject, path, length, listBuffer);

                    //Delete
                    listBuffer.RemoveRange(0, 16 + length);
                }
            } while (m_bQuit == false);           
        }        
    }
}
