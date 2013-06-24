/****************************************************************************
* Checkpointing library itinialization routines
*
* Copyright (C) 2013, Alexandru-Cezar Sardan, Marius-Alexandru Caba
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
* 02111-1307, USA.
****************************************************************************/

#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include <intrin.h>
#include "winmtcp_main.h"
#include "winmctp_createChkpt.h"


#pragma comment(lib, "ntdll");

HANDLE ckpTimerHandle, ckpHeap;
PROCESS_BASIC_INFORMATION procInfo;
ULONG_PTR mainTEBAddr;
HANDLE hProcess;

/**
* The checkpointing thread.
* Will perform the checkpointing operation when the timer expires.
*/
DWORD WINAPI ckpThreadFunc(LPVOID lpParamater)
{
	LARGE_INTEGER ckpTime;
	ckpThreadArgs_t *param;
	WORD nChars;
	CHAR ckpMessage[] = "Application is creating a checkpoint now...\n";
	int ret;

	param = (ckpThreadArgs_t*)lpParamater;
	ckpTime.QuadPart = -10000000*param->interval;

	/* start the timer for the first time */
	if (SetWaitableTimer(ckpTimerHandle, &ckpTime, 0, NULL, NULL, FALSE) == 0)
		return -1;

	while (TRUE)
	{
		/* Wait until it's time to checkpoint */
		if (WaitForSingleObject(ckpTimerHandle, INFINITE) != WAIT_OBJECT_0)
			return 1;

		/* 
		* TODO: Checkpointing code goes here
		*/
		printf("%s\n", ckpMessage);
		ret = createCheckpoint(param->mainThread);
		if (ret == 0)
			printf("Succesfully created checkpoint\n");
		else
			printf("Error occured while creating checkpoint\n");

		return 0;

		/* Rearm the timer */
		if (SetWaitableTimer(ckpTimerHandle, &ckpTime, 0, NULL, NULL, FALSE) == 0)
			return 1;
	}

	if (HeapFree(ckpHeap, 0, param) == 0)
		return 1;

	return 0;
}


/**
* Init the checkpointing thread, timer and heap
*/
__declspec(dllexport) int winmtcp_init (long long interval)
{
	HANDLE ckpThread;
	LARGE_INTEGER ckpTime;	
	ckpThreadArgs_t *ckpArgs;
	NTSTATUS stat;
	DWORD procId;
	PVOID infoBuff;

	ckpTime.QuadPart = -10000000*interval;

	/* create a timer that will be signaled when it's time to checkpoint */
	if ((ckpTimerHandle = CreateWaitableTimer(NULL, FALSE, NULL)) == NULL) 
		return -1;

	/* create a new heap so we can easily keep track of the memory 
	* allocated by our library 
	*/
	if ((ckpHeap = HeapCreate(0, 0, 0)) == NULL) 
		return -1;

	/* construct args that need to be passed to ckpThread */
	ckpArgs = (ckpThreadArgs_t*) HeapAlloc (ckpHeap, 0, sizeof(ckpThreadArgs_t));
	if (ckpArgs == NULL)
		return -1;

	ckpArgs->interval = interval;
	ckpArgs->mainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());

	/* Get the current process handle */
	procId = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
	if (!hProcess)
	{
		printf ("ERROR (code 0x%x): Cannot get current process handle. \n", GetLastError());
		exit(1);
	}

	/* Get information about the PEB from the current process */
	stat = NtQueryInformationProcess(hProcess, ProcessBasicInformation, 
									 &procInfo, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (!NT_SUCCESS(stat)) {
		printf ("ERROR (code 0x%x): Cannot get information about about the PEB. \n", stat);
		return 1;
	}
	printf ("\nPEB address of dummy is: 0x%p\n", procInfo.PebBaseAddress);

	/* Get the address of the dummy process main TEB */
	infoBuff = malloc(sizeof(THREAD_BASIC_INFORMATION));
	stat = NtQueryInformationThread(ckpArgs->mainThread, (THREADINFOCLASS) 0, infoBuff, sizeof(THREAD_BASIC_INFORMATION), NULL);
	if (!NT_SUCCESS(stat)) {
		printf ("ERROR (code 0x%x): Cannot get information about about the main TEB. \n", stat);
		return 1;
	}

	mainTEBAddr = (ULONG_PTR)((PTHREAD_BASIC_INFORMATION)infoBuff)->TebBaseAddress;
	free(infoBuff);
	printf ("TEB address of dummy main thread is: 0x%p\n", mainTEBAddr);

	//CloseHandle(hProcess);

	/* create the checkpointing thread that will handle the timer */
	if ((ckpThread = CreateThread(NULL, 0, ckpThreadFunc, ckpArgs, 0, NULL)) == NULL)
		return -1;

	/* TODO: init other stuff here (like syscall interception code) */

	return 0;
}