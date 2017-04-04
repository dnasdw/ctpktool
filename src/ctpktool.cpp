#include "ctpktool.h"
#include "ctpk.h"

CCtpkTool::SOption CCtpkTool::s_Option[] =
{
	{ USTR("export"), USTR('e'), USTR("export from the target file") },
	{ USTR("import"), USTR('i'), USTR("import to the target file") },
	{ USTR("file"), USTR('f'), USTR("the target file") },
	{ USTR("dir"), USTR('d'), USTR("the dir for the target file") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, nullptr }
};

CCtpkTool::CCtpkTool()
	: m_eAction(kActionNone)
	, m_bVerbose(false)
{
}

CCtpkTool::~CCtpkTool()
{
}

int CCtpkTool::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int CCtpkTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionHelp)
	{
		if (m_sFileName.empty())
		{
			UPrintf(USTR("ERROR: no --file option\n\n"));
			return 1;
		}
		if (m_sDirName.empty())
		{
			UPrintf(USTR("ERROR: no --dir option\n\n"));
			return 1;
		}
		if (!CCtpk::IsCtpkFile(m_sFileName))
		{
			if (!CCtpk::IsCtpkIconFile(m_sFileName))
			{
				UPrintf(USTR("ERROR: %") PRIUS USTR(" is not a ctpk file\n\n"), m_sFileName.c_str());
				return 1;
			}
		}
	}
	return 0;
}

int CCtpkTool::Help()
{
	UPrintf(USTR("ctpktool %") PRIUS USTR(" by dnasdw\n\n"), AToU(CTPKTOOL_VERSION).c_str());
	UPrintf(USTR("usage: ctpktool [option...] [option]...\n"));
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("  ctpktool -evfd input.ctpk outputdir\n"));
	UPrintf(USTR("  ctpktool -ivfd output.ctpk inputdir\n"));
	UPrintf(USTR("\n"));
	UPrintf(USTR("option:\n"));
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
		pOption++;
	}
	return 0;
}

int CCtpkTool::Action()
{
	if (m_eAction == kActionExport)
	{
		if (!exportFile())
		{
			UPrintf(USTR("ERROR: export file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionImport)
	{
		if (!importFile())
		{
			UPrintf(USTR("ERROR: import file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CCtpkTool::EParseOptionReturn CCtpkTool::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("export")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionExport;
		}
		else if (m_eAction != kActionExport && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("import")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionImport;
		}
		else if (m_eAction != kActionImport && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("dir")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sDirName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

CCtpkTool::EParseOptionReturn CCtpkTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, UChar* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool CCtpkTool::exportFile()
{
	CCtpk ctpk;
	ctpk.SetFileName(m_sFileName);
	ctpk.SetDirName(m_sDirName);
	ctpk.SetVerbose(m_bVerbose);
	return ctpk.ExportFile();
}

bool CCtpkTool::importFile()
{
	CCtpk ctpk;
	ctpk.SetFileName(m_sFileName);
	ctpk.SetDirName(m_sDirName);
	ctpk.SetVerbose(m_bVerbose);
	return ctpk.ImportFile();
}

int UMain(int argc, UChar* argv[])
{
	CCtpkTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
