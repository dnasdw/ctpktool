#include "ctpktool.h"
#include "ctpk.h"

CCtpkTool::SOption CCtpkTool::s_Option[] =
{
	{ "export", 'e', "export from the target file" },
	{ "import", 'i', "import to the target file" },
	{ "file", 'f', "the target file" },
	{ "dir", 'd', "the dir for the target file" },
	{ "verbose", 'v', "show the info" },
	{ "help", 'h', "show this help" },
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

int CCtpkTool::ParseOptions(int a_nArgc, char* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(strlen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != '-')
		{
			printf("ERROR: illegal option\n\n");
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != '-')
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnOptionConflict:
					printf("ERROR: option conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == '-')
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				printf("ERROR: illegal option\n\n");
				return 1;
			case kParseOptionReturnNoArgument:
				printf("ERROR: no argument\n\n");
				return 1;
			case kParseOptionReturnOptionConflict:
				printf("ERROR: option conflict\n\n");
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
		printf("ERROR: nothing to do\n\n");
		return 1;
	}
	if (m_eAction != kActionHelp)
	{
		if (m_sFileName.empty())
		{
			printf("ERROR: no --file option\n\n");
			return 1;
		}
		if (m_sDirName.empty())
		{
			printf("ERROR: no --dir option\n\n");
			return 1;
		}
		if (!CCtpk::IsCtpkFile(m_sFileName))
		{
			if (!CCtpk::IsCtpkIconFile(m_sFileName))
			{
				printf("ERROR: %s is not a ctpk file\n\n", m_sFileName.c_str());
				return 1;
			}
		}
	}
	return 0;
}

int CCtpkTool::Help()
{
	printf("ctpktool %s by dnasdw\n\n", CTPKTOOL_VERSION);
	printf("usage: ctpktool [option...] [option]...\n");
	printf("sample:\n");
	printf("  ctpktool -evfd input.ctpk outputdir\n");
	printf("  ctpktool -ivfd output.ctpk inputdir\n");
	printf("\n");
	printf("option:\n");
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			printf("  ");
			if (pOption->Key != 0)
			{
				printf("-%c,", pOption->Key);
			}
			else
			{
				printf("   ");
			}
			printf(" --%-8s", pOption->Name);
			if (strlen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				printf("\n%16s", "");
			}
		}
		if (pOption->Doc != nullptr)
		{
			printf("%s", pOption->Doc);
		}
		printf("\n");
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
			printf("ERROR: export file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionImport)
	{
		if (!importFile())
		{
			printf("ERROR: import file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CCtpkTool::EParseOptionReturn CCtpkTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "export") == 0)
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
	else if (strcmp(a_pName, "import") == 0)
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
	else if (strcmp(a_pName, "file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

CCtpkTool::EParseOptionReturn CCtpkTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, char* a_pArgv[])
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

int main(int argc, char* argv[])
{
	SetLocale();
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
