#include "stdafx.h"
#include "ExcmdLineInfo.h"


CExcmdLineInfo::CExcmdLineInfo(void)
{
	cmdArgs.push_back("example_mfc.exe");
}


CExcmdLineInfo::~CExcmdLineInfo(void)
{
}
void CExcmdLineInfo::ParseParam( 
	const TCHAR* pszParam,  
	BOOL bFlag, 
	BOOL bLast
	)
{
	std::string strArg = (LPCSTR)CW2A(pszParam);
	if (strArg.length()>2)
		if (strArg[0] == '-' && strArg[1] != '-')
			strArg = "-" + strArg;
	cmdArgs.push_back(strArg);
	if (bLast)
		cmd.parser(cmdArgs);
	CCommandLineInfo::ParseParam(pszParam,bFlag,bLast);
}