#pragma once
#include "afxwin.h"
#include <map>
#include <vector>
#include <string>
#include "../../../../tb_interface/cmdlineparser.h"
class CExcmdLineInfo :
	public CCommandLineInfo
{
public:
	CExcmdLineInfo(void);
	virtual ~CExcmdLineInfo(void);
public:
	void ParseParam( 
		const TCHAR* pszParam,  
		BOOL bFlag, 
		BOOL bLast
		);
public:
	TASKBUS::cmdlineParser cmd;
private:
	std::vector<std::string> cmdArgs;

};

