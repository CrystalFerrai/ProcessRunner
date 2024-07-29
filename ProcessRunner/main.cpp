// Copyright 2023 Crystal Ferrai
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <Windows.h>

// How long to wait after process termination before restarting the process (in milliseconds)
#define RESTART_DELAY 5000

HANDLE startShutdownHandle;
HANDLE endShutdownHandle;
volatile bool isLooping;

BOOL WINAPI ConsoleHandler(DWORD);
void HandleProcessExit(HANDLE process);
void PrintMessage(const char* message, ...);
void PrintSystemError(DWORD error);

// Entry point
int main(int argc, char** argv)
{
	if (argc < 2 || argc > 3)
	{
		printf_s("Usage: ProcessRunner \"command line to run\" [optional process id to attach to initially]\n");
		return EXIT_SUCCESS;
	}

	printf_s("Press Ctrl+C to detach from the running process and terminate this program.\n");
	printf_s("%s\n\n", argv[1]);

	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE))
	{
		fprintf_s(stderr, "Internal error: Failed to set console handler\n");
		return EXIT_FAILURE;
	}

	startShutdownHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
	HANDLE waitHandles[2];
	waitHandles[0] = startShutdownHandle;

	endShutdownHandle = CreateEventA(NULL, FALSE, FALSE, NULL);

	isLooping = true;

	if (argc == 3)
	{
		// Attach to pre-existing process
		DWORD pid = 0;
		{
			size_t len = strnlen_s(argv[2], 128);
			char* end;
			pid = strtol(argv[2], &end, 10);
			if (argv[2] + len > end)
			{
				fprintf_s(stderr, "Invalid process id: %s\n", argv[2]);
				return EXIT_FAILURE;
			}
		}
		HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (process == NULL)
		{
			DWORD error = GetLastError();
			fprintf_s(stderr, "Could not attach to process with id: %d\n", pid);
			PrintSystemError(error);
			return EXIT_FAILURE;
		}
		PrintMessage("Attached to process %d\n", pid);

		waitHandles[1] = process;
		WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

		if (isLooping)
		{
			HandleProcessExit(process);
		}

		CloseHandle(process);

		if (isLooping)
		{
			WaitForSingleObject(startShutdownHandle, RESTART_DELAY);
		}
	}

	while (isLooping)
	{
		STARTUPINFOA si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		PrintMessage("Starting process\n");
		if (!CreateProcessA(NULL, argv[1], NULL, NULL, false, 0, NULL, NULL, &si, &pi))
		{
			DWORD error = GetLastError();
			fprintf_s(stderr, "Failed to start process\n");
			PrintSystemError(error);
			return EXIT_FAILURE;
		}
		PrintMessage("Attached to process %d\n", pi.dwProcessId);

		waitHandles[1] = pi.hProcess;

		DWORD i = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

		if (isLooping)
		{
			HandleProcessExit(pi.hProcess);
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (isLooping)
		{
			WaitForSingleObject(startShutdownHandle, RESTART_DELAY);
		}
	}

	SetEvent(endShutdownHandle);
	return EXIT_SUCCESS;
}

// Console event handler
BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		PrintMessage("Exiting\n");
		isLooping = false;
		SetEvent(startShutdownHandle);

		// Returning from a close event immediately exits the process, so wait for things to clean up first
		WaitForSingleObject(endShutdownHandle, 2000);
		return TRUE;
	}
	return FALSE;
}

// Prints a process terminated message including the process exit code if possible
void HandleProcessExit(HANDLE process)
{
	DWORD exitCode;
	BOOL knownExitCode = GetExitCodeProcess(process, &exitCode);
	if (knownExitCode)
	{
		PrintMessage("Process terminated with code %d.\n", exitCode);
	}
	else
	{
		DWORD error = GetLastError();
		fprintf_s(stderr, "Process terminated. Could not obtain process exit code. ");
		PrintSystemError(error);
	}
}

// Prints a timestamped message to stdout
void PrintMessage(const char* message, ...)
{
	SYSTEMTIME time;
	GetSystemTime(&time);

	printf_s("[%04d-%02d-%02d %02d:%02d:%02d] ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

	va_list args;
	va_start(args, message);
	vprintf_s(message, args);
	va_end(args);
}

// Prints a formatted system error to stderr
void PrintSystemError(DWORD error)
{
	LPVOID buffer;

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&buffer,
		0, NULL);

	fprintf_s(stderr, "Error %d: %s\n", error, (LPSTR)buffer);

	LocalFree(buffer);
}
