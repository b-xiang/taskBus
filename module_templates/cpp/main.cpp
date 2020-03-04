#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

using namespace std;
#define MAX_DTALEN 65536
int main(int argc, char * argv[])
{
	//In windows, stdio must be set to BINARY mode, to
	//prevent linebreak \\n\\r replace.
#ifdef WIN32
	setmode(fileno(stdout),O_BINARY);
	setmode(fileno(stdin),O_BINARY);
#endif
	bool bInfo = false, finished = false;
	int instance = 0;
	int sub_input = 0, sub_output = 0;
	char mask = 0;
	string function;
	//1. parse cmdline
	for (int i=1;i<argc;++i)
	{
		string arg_key = argv[i], arg_value = argv[i];
		int idx = arg_key.find('=');
		if (idx>=0 && idx<arg_key.size())
		{
			arg_key = arg_key.substr(0,idx);
			arg_value = arg_value.substr(idx+1);
		}
		if (arg_key=="--function")
			function = arg_value;
		else if (arg_key=="--information")
			bInfo = true;
		else if (arg_key=="--instance")
			instance = atoi(arg_value.c_str());
		else if (arg_key=="--data_in")
			sub_input = atoi(arg_value.c_str());
		else if (arg_key=="--data_out")
			sub_output = atoi(arg_value.c_str());
		else if (arg_key=="--mask")
			mask = atoi(arg_value.c_str());
		fprintf(stderr,"%s:%s\n",arg_key.c_str(),arg_value.c_str());
		fflush(stderr);
	}
	//2. function case
	if (bInfo)
	{
		//In this example, json file will be published with exe file.
		//We will return directly.  Or, you can output json here to stdout,
		//If you do not want to publish your json file.
		return 0;
	}
	else if (instance<=0 || function.length()==0)
		return -1;
	else
	{
		char header[4], data[MAX_DTALEN+1];
		memset(data,0,MAX_DTALEN+1);
		int n_sub = 0, n_path = 0, n_len = 0;
		while(false==finished)
		{
			fread(header,1,4,stdin);	//2.1 read header
			if (header[0]!=0x3C || header[1]!=0x5A || header[2]!=0x7E || header[3]!=0x69)
			{
				fprintf(stderr,"Bad header\n");
				break;
			}
			fread(&n_sub,sizeof(int),1,stdin);
			fread(&n_path,sizeof(int),1,stdin);
			fread(&n_len,sizeof(int),1,stdin);
			if (n_len<0 || n_len >MAX_DTALEN)
			{
				fprintf(stderr,"Bad length %d\n",n_len);
				break;
			}
			fread(data,sizeof(char),n_len,stdin);

			if (n_sub<=0)
			{
				if (strstr(data, "function=quit;")!=nullptr)
				{
					finished = true;
					continue;
				}
			}
			else if (n_sub != sub_input)
				continue;

			if (function=="example_bitxor")
			{
				for (int i=0;i<n_len;++i)
					data[i] ^= mask;
			}
			else if (function=="example_reverse")
			{
				for (int i=0;i<n_len/2;++i)
				{
					char t = data[i];
					data[i] = data[n_len-1-i];
					data[n_len-1-i] = t;
				}
			}
			else
			{
				fprintf(stderr,"Unknown function %s\n",function.c_str());
				break;
			}
			fwrite(header,1,4,stdout);
			fwrite(&sub_output,sizeof(int),1,stdout);
			fwrite(&n_path,sizeof(int),1,stdout);
			fwrite(&n_len,sizeof(int),1,stdout);
			fwrite(data,sizeof(char),n_len,stdout);
			fflush(stdout);
		}
	}
	//3.exit
	return 0;
}
