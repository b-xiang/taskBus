using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace RandomSource
{
    class Program
    {
        static void Main(string[] args)
        {
            uint instance_id = 0;
            uint chaos_id = 0;
            uint total_path = 8;
            uint path_rate = 1024 * 128;
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
            System.IO.Stream stderr = Console.OpenStandardError();
            System.IO.Stream stdout = System.Console.OpenStandardOutput();
            if (chaos_id == 0)
                return;
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
}
