#include <stdio.h>
#include <map>
#include <windows.h>
#include <cstring>
#include <tchar.h>
#include "../../../tb_interface/cmdlineparser.h"
#include "../../../tb_interface/tb_interface.h"
#include "engine.h"
#include "matrix.h"
#pragma comment( lib, "libeng.lib" )
#pragma comment( lib, "libmx.lib" )
using namespace TASKBUS;
int do_matlab(const int argc,  const  char * const  argv[]);

int main(int argc , char * argv[])
{
	init_client();
	const cmdlineParser args (argc,argv);
	int ret = 0;
	
	if (args.contains("function","example_matlab"))
		ret = do_matlab(argc,argv);
	else
	{
		fprintf_s(stderr,"no function valid in func list.");
		ret = -1;
	}

	return ret;
}


int do_matlab(const  int argc,   const char * const   argv[])
{
	Engine *ep;
	mxArray *T = NULL, *result = NULL;
	//Chdir
	char fileName[MAX_PATH];
	::GetModuleFileNameA(0, fileName, MAX_PATH);
	while (strlen(fileName) > 1)
		if (fileName[strlen(fileName) - 1] != _T('/') && fileName[strlen(fileName) - 1] != _T('\\') && fileName[strlen(fileName) - 1] != _T(':'))
			fileName[strlen(fileName) - 1] = 0;
		else
			break;
	
	std::string pathv = "cd '" ;
	pathv += std::string(fileName);	
	pathv += "';";
	if (!(ep = engOpen(pathv.c_str()))) {
		fprintf(stderr, "\nCan't start MATLAB engine\n");
		return EXIT_FAILURE;
	}
	

	const cmdlineParser args (argc,argv);

	int res = 0;
	std::string script   = args.toString("program","entry");
	

	//获得平台告诉自己的实例名
	const unsigned int instance	  = args.toInt("instance",0);
	const unsigned int isub1 = args.toInt("isub1",0); 
	const unsigned int isub2 = args.toInt("isub2",0);
	const unsigned int isub3 = args.toInt("isub3",0);
	const unsigned int osub1 = args.toInt("osub1",0);
	const unsigned int osub2 = args.toInt("osub2",0); 
	const unsigned int osub3 = args.toInt("osub3",0); 
	try{
		//判断参数合法性
		if (instance==0)	throw "\"quit\":{\"error\":\"instance is 0, quit.\"}";
		//开始服务
		bool bfinished = false;
		while (bfinished==false)
		{
			engEvalString(ep,pathv.c_str());
			subject_package_header header;
			std::vector<unsigned char> packagedta = pull_subject(&header);
			if (is_valid_header(header) == false)
			{
				::Sleep(100);
				continue;
			}
			else if (header.subject_id==isub1 || header.subject_id==isub2 ||header.subject_id==isub3 )
			{
				if (header.subject_id==0)
					continue;
				//传入头
				mxArray * header_subject_id = mxCreateDoubleMatrix(1, 1, mxREAL);
				if (header_subject_id)	mxGetPr(header_subject_id)[0] = header.subject_id;
				mxArray * header_path_id = mxCreateDoubleMatrix(1, 1, mxREAL);
				if (header_path_id)	mxGetPr(header_path_id)[0] = header.path_id;
				mxArray * header_valid_isubids = mxCreateDoubleMatrix(1, 3, mxREAL);
				if (header_path_id)	
				{
					mxGetPr(header_valid_isubids)[0] = isub1;
					mxGetPr(header_valid_isubids)[1] = isub2;
					mxGetPr(header_valid_isubids)[2] = isub3;
				}
				mxArray * header_valid_osubids = mxCreateDoubleMatrix(1, 3, mxREAL);
				if (header_path_id)	
				{
					mxGetPr(header_valid_osubids)[0] = osub1;
					mxGetPr(header_valid_osubids)[1] = osub2;
					mxGetPr(header_valid_osubids)[2] = osub3;
				}
				
				engEvalString(ep, "clear header_subject_id  header_path_id header_valid_isubids header_valid_osubids package_data;");
				engEvalString(ep, "clear out_sub_id  out_path_id out_package;");
				engPutVariable(ep,"header_subject_id",header_subject_id);
				engPutVariable(ep,"header_path_id",header_path_id);
				engPutVariable(ep,"header_valid_isubids",header_valid_isubids);
				engPutVariable(ep,"header_valid_osubids",header_valid_osubids);
				
				const int ELEMENTS = packagedta.size();
				//传入数据
				INT8_T *dynamicData = (INT8_T *) mxCalloc(ELEMENTS, sizeof(INT8_T));
				for ( int index = 0; index < ELEMENTS; index++ ) 
					dynamicData[index] = packagedta[index];
				
				mxArray * inputArr = mxCreateNumericMatrix(0, 0, mxINT8_CLASS, mxREAL);
				/* Point mxArray to dynamicData */
				mxSetData(inputArr, dynamicData);
				mxSetM(inputArr, 1);
				mxSetN(inputArr, ELEMENTS);
				
				engPutVariable(ep,"package_data", inputArr);
				std::string cmd = "[out_sub_id,out_path_id,out_package]=";
				cmd += script;
				cmd += "( header_subject_id, header_path_id,header_valid_isubids,header_valid_osubids,package_data);";
				//执行
				engEvalString(ep, cmd.c_str());
				//取结果
				mxArray * out_sub_id = engGetVariable(ep, "out_sub_id");
				mxArray * out_path_id = engGetVariable(ep, "out_path_id");
				mxArray * out_data = engGetVariable(ep, "out_package");
				
				if (out_sub_id && out_path_id && out_data )
				{
					if (
						mxGetNumberOfElements(out_sub_id)>0 &&
						mxGetNumberOfElements(out_path_id)>0 &&
						mxGetNumberOfElements(out_data)>0
						)
					{
						int sub_out = int(mxGetPr(out_sub_id)[0]+0.0001);
						int path_out = int(mxGetPr(out_path_id)[0]+0.0001);
						size_t Size_out = mxGetNumberOfElements(out_data);
						size_t elSize = mxGetElementSize(out_data);
						size_t memSz = Size_out * elSize;
						if (memSz)
							push_subject(
								sub_out,				
								path_out,
								Size_out,
								(const unsigned char *)mxGetPr(out_data)				
								);
					}

				}
				else
				{
					fprintf (stderr,("Err matlab:"+cmd).c_str());
					fflush(stderr);
				}
				if (out_sub_id)	mxDestroyArray (out_sub_id);
				if (out_path_id)	mxDestroyArray (out_path_id);
				if (out_data)	mxDestroyArray (out_data);
				if (inputArr)	mxDestroyArray(inputArr);
				if (header_subject_id)	mxDestroyArray(header_subject_id);
				if (header_path_id)	mxDestroyArray(header_path_id);
				if (header_valid_isubids)	mxDestroyArray(header_valid_isubids);
				if (header_valid_osubids)	mxDestroyArray(header_valid_osubids);
				
			}			
			else if (is_control_subject(header))
			{
				//收到命令进程退出的广播消息,退出
				if (strstr(control_subject(header,packagedta).c_str(),"function=quit;")>0)
					bfinished = true;
			}		
			else
			{
				fprintf_s(stderr,"Warning:src=%d,dst=%d,sub=%d,path=%d,len=%d",
					header.subject_id,
					header.path_id,
					header.data_length
					);
				fflush (stderr);
			}
		}

	}
	catch (const char * errMessage)
	{
		//向所有部位广播，偶要退出。
		push_subject(control_subect_id(),0,errMessage);
		fprintf_s(stderr,"Error:%s.",errMessage);
		fflush (stderr);
		res = -1;
	}
	catch (std::string errMessage)
	{
		//向所有部位广播，偶要退出。
		push_subject(control_subect_id(),0,errMessage.c_str());
		fprintf_s(stderr,"Error:%s.",errMessage.c_str());
		fflush (stderr);
		res = -1;
	}

	
	engClose(ep);

	return res;
}
