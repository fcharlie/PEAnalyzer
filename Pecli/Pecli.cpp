// Pecli.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Dbghelp.h>
#include <codecvt>
#include <winnt.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
void DebugPrint(const wchar_t *format, ...) {

}
#else
#define DebugPrint __noop
#endif

bool ExecutableFile(const std::wstring &file) {
	/// --->
	return false;
}


int wmain(int argc,wchar_t **argv)
{
	if (argc < 2) {
		fwprintf(stderr, L"%s pefile\n", argv[0]);
		return 1;
	}
	ExecutableFile(argv[1]);
	std::cout << "Press ENTER to exit." << std::endl;
	std::string line;
	std::getline(std::cin, line);
    return 0;
}

