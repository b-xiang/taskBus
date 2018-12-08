// Cspthread.cpp : implementation file
//

#include "stdafx.h"
#include "Cspthread.h"
#include "resource.h"
#include "mfcDlg.h"
#include "mfc.h"
#include "../../../../tb_interface/tb_interface.h"
UINT __cdecl ListenFunction( LPVOID pParam )
{
	CmfcDlg * pDlg = (CmfcDlg *)pParam;
	CmfcApp * pAll = (CmfcApp *)AfxGetApp();
	try
	{
		if (pAll->info.cmd.contains("function")==false)
			throw "no function specified.";
		if (pAll->info.cmd.contains("function","example_mfc") == false)
			throw "no function specified.";
		int insplot = pAll->info.cmd.toInt("input", 0);
		bool bfinished = false;
		using namespace TASKBUS;
		while (false==bfinished)
		{
			subject_package_header header;
			std::vector<unsigned char> packagedta = pull_subject(&header);
			if (is_control_subject(header))
			{
				if (strstr((const char *)packagedta.data(), "function=quit;") !=0)
				{
					fprintf(stderr, "Recieved Quit Cmd!");					
					bfinished = true;
				}			
				
				int length = packagedta.size();
				char * addr = new char[length + 1];
				strcpy_s(addr, length , (char *)packagedta.data());
				PostMessage(pDlg->m_hWnd, MSG_NEW_PACK,
					(WPARAM)length,
					(LPARAM)addr);
					
			}
			else if (header.subject_id==insplot)
			{
				char v16[] = "0123456789ABCDEF";
				std::string strV;
				for (size_t i = 0; i < packagedta.size(); ++i)
				{
					strV += v16[(packagedta[i] >> 4)];
					strV += v16[(packagedta[i] &0x0F)];
					strV += " ";
				}
				strV += '\0';
				int length = strV.size();
				char * addr = new char[length+1];
				strcpy_s(addr, length + 1, strV.c_str());
				PostMessage(pDlg->m_hWnd,MSG_NEW_PACK,
					(WPARAM)length,
					(LPARAM)addr);
				
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
		PostMessage(pDlg->m_hWnd, MSG_QUIT_APP,
			(WPARAM)0,
			(LPARAM)0);
	}
	catch (const char * errMessage)
	{
		fprintf_s(stderr,"Error:%s.",errMessage);
		fflush (stderr);
	}

	return 0;
}

