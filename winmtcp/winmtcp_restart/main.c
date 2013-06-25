/****************************************************************************
 * WINMTCP restart program
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
#include <stdlib.h>
#include <stdio.h>
#include <winternl.h>
#include <stddef.h>
#include "remoteprctl.h"
#include "../winmtcp/winmctp_createChkpt.h"
#include "util\list.h"

#pragma comment(lib, "ntdll");

typedef struct {
	chkptMemInfo_t *chkptMemInfo;
	void *memBuffer;
} list_entry;

HMODULE ntDllModule;
PROCESS_BASIC_INFORMATION dummyProcInfo;
ULONG_PTR dummyMainTEBAddr;
	CONTEXT threadContext;


void createDummyProcess (PROCESS_INFORMATION *procInfo)
{
	STARTUPINFO procSi;

	ZeroMemory (&procSi, sizeof(STARTUPINFO));
	procSi.cb = sizeof(STARTUPINFO);
	//procSi.dwFlags |= STARTF_USESTDHANDLES;
	ZeroMemory (procInfo, sizeof(PROCESS_INFORMATION));


	if( !CreateProcess (NULL, "dummyproc.exe", NULL, NULL, FALSE, 
						0, NULL, NULL, &procSi, procInfo)) 
    {
        printf ("Process creation failed: Error %d.\n", GetLastError());
        exit (1);
    }
	Sleep(4000);
	SuspendThread(procInfo->hThread);
}

int main (int argc, char **argv)
{
	PROCESS_INFORMATION procInfo;

	chkptMemInfo_t *chkptMemInfo;
	void *memBuffer = NULL;
	SIZE_T bufferSize = 0, allocationSize = 0x0;
	FILE *pFile;
	size_t nread;
	int i;
	list_entry *entry, *tempEntry;
	ULONG_PTR oldAllocationBase = 0x0;
	list_t memInfoList;
	NTSTATUS stat;
	node_t *current;
	PVOID infoBuff;
	BOOL ret;
	RTL_USER_PROCESS_PARAMETERS_MTCP originalParams;
	ULONG_PTR originalParamsPtr, newParamsPtr;

	if (argc < 2) 
	{
		printf("Usage: %s <checkpoint_file>\n", argv[0]);
		exit(1);
	}

	/* Get a handle to ntdll.dll. We will use it to access the Nt* syscalls */
	ntDllModule = GetModuleHandle("ntdll.dll");
	if (ntDllModule == NULL)
	{
		printf("ERROR (code %d): Cannot get handle to ntdll.dll.\n", GetLastError());
		return 1;
	}

	list_init (&memInfoList);

	/* create a dummy process that will have it's address space replaced */
	createDummyProcess (&procInfo);
	FlushInstructionCache (procInfo.hProcess, NULL, 0);

	/* Get information about the PEB from the dummy process */
	stat = NtQueryInformationProcess(procInfo.hProcess, ProcessBasicInformation, 
									 &dummyProcInfo, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (!NT_SUCCESS(stat)) 
	{
		printf ("ERROR (code 0x%x): Cannot get information about about the PEB. \n", stat);
		return 1;
	}
	printf ("PEB address of dummy is: 0x%p\n", dummyProcInfo.PebBaseAddress);

	/* Get the address of the RTL_USER_PROCESS_PARAMETERS structure */
	ReadProcessMemory(procInfo.hProcess, (LPCVOID)((ULONG_PTR)((PPEB_MTCP)dummyProcInfo.PebBaseAddress) + ((ULONG_PTR)&(((PEB_MTCP *) 0)->ProcessParameters))), &originalParamsPtr, sizeof(ULONG_PTR), NULL);

	/* Get the original params of the created process */
	ReadProcessMemory(procInfo.hProcess, (LPCVOID) originalParamsPtr, &originalParams, sizeof(RTL_USER_PROCESS_PARAMETERS_MTCP), NULL);

	/* Get the address of the dummy process main TEB */
	infoBuff = malloc(sizeof(THREAD_BASIC_INFORMATION));
	stat = NtQueryInformationThread(procInfo.hThread, (THREADINFOCLASS) 0, infoBuff, sizeof(THREAD_BASIC_INFORMATION), NULL);
	if (!NT_SUCCESS(stat)) 
	{
		printf ("ERROR (code 0x%x): Cannot get information about about the main TEB. \n", stat);
		return 1;
	}
	dummyMainTEBAddr = (ULONG_PTR)((PTHREAD_BASIC_INFORMATION)infoBuff)->TebBaseAddress;
	free(infoBuff);
	printf ("TEB address of dummy main thread is: 0x%p\n", dummyMainTEBAddr);

	pFile = fopen (argv[1], "rb");
	if (pFile == NULL) 
	{
		printf ("Error opening file: %s\n", argv[1]);
		exit(1);
	}

	nread = fread (&threadContext, sizeof(threadContext), 1, pFile);
	if (nread != 1) 
	{
		printf ("Problem reading threadcontext.\n");
		fclose(pFile);
		exit(1);
	}

	if (!clearTargetMemory (procInfo))
		exit (1);
	printf ("Process memory cleared!\n");

	while (!feof(pFile)) 
	{
		entry = (list_entry *) malloc(sizeof(list_entry));
		chkptMemInfo = (chkptMemInfo_t *) malloc (sizeof(chkptMemInfo_t));

		nread = fread (chkptMemInfo, sizeof(chkptMemInfo_t), 1, pFile);
		if (nread != 1) 
		{
			printf ("Problem reading chkpt mem info: %d.\n", nread);
			//fclose(pFile);
			//exit(1);
			break;
		}

		memBuffer = malloc(chkptMemInfo->meminfo.RegionSize * sizeof(char));

		if (chkptMemInfo->hasData == TRUE) 
		{
			nread = fread (memBuffer, sizeof(char), chkptMemInfo->meminfo.RegionSize, pFile);
			if (nread != chkptMemInfo->meminfo.RegionSize) 
			{
				printf ("Problem reading data.\n");
				//fclose(pFile);
				//exit(1);
			}
		}

		entry->chkptMemInfo = chkptMemInfo;
		entry->memBuffer = memBuffer;

		if (oldAllocationBase != ((ULONG_PTR) (chkptMemInfo->meminfo.AllocationBase))) 
		{
			current = memInfoList.head;
			
			if (memInfoList.size != 0) 
			{
				tempEntry = (list_entry*)current->data;

				/* allocate memory for the remote process */
				if (tempEntry->chkptMemInfo->attr == noAttr) 
					allocTargetMemory(procInfo, tempEntry->chkptMemInfo->meminfo, allocationSize);
				else 
				{
					printf ("Found PEB/TEB 0x%p\n", tempEntry->chkptMemInfo->meminfo.BaseAddress);
					if (tempEntry->chkptMemInfo->attr == teb) 
					{
						ret = WriteProcessMemory(procInfo.hProcess, (LPVOID) dummyMainTEBAddr, 
									   ((LPCVOID)(((ULONG_PTR)tempEntry->memBuffer))), 24, NULL);
						if (!ret)
							printf ("ERROR (code 0x%x): Cannot write the TEB. \n", GetLastError());
					}
					else if (tempEntry->chkptMemInfo->attr == peb)
					{
						newParamsPtr = *((ULONG_PTR*)((ULONG_PTR)tempEntry->memBuffer + ((ULONG_PTR)&(((PEB_MTCP *) 0)->ProcessParameters))));
						ret = WriteProcessMemory(procInfo.hProcess, (LPVOID)((ULONG_PTR)((PPEB_MTCP)dummyProcInfo.PebBaseAddress) + ((ULONG_PTR)&(((PEB_MTCP *) 0)->ProcessParameters))), &newParamsPtr, sizeof(ULONG_PTR), NULL);
						if (!ret)
							printf ("ERROR (code 0x%x): Cannot write the address of RTL_USER_PROCESS_PARAMETERS into the PEB. \n", GetLastError());
					}
				}
				oldAllocationBase = (ULONG_PTR) chkptMemInfo->meminfo.AllocationBase;
			
				/* set protections and write data */
				for (i = 0; i < memInfoList.size; i++)
				{
					tempEntry = (list_entry*)current->data;
					chkptMemInfo = tempEntry->chkptMemInfo;
					memBuffer = tempEntry->memBuffer;

					if (chkptMemInfo->attr == noAttr ) 
					{
						if (!setTargetMemory(procInfo, chkptMemInfo->meminfo, memBuffer, chkptMemInfo->hasData))
						{
							printf("error\n");
							//exit(1);
						}
					}
					current = current->next;
				}
				/* clear list */
				list_clear(&memInfoList);
				allocationSize = 0;
			}
		}

		/* insert new mem info in the list */
		list_insert_back(&memInfoList, entry);
		allocationSize += entry->chkptMemInfo->meminfo.RegionSize;
	}

	fclose (pFile);

	ret = WriteProcessMemory(procInfo.hProcess, (LPVOID)(newParamsPtr + ((ULONG_PTR)&(((RTL_USER_PROCESS_PARAMETERS_MTCP *) 0)->ConsoleHandle))), &originalParams.StdOutputHandle, sizeof(PVOID) + sizeof(ULONG) + 3*sizeof(HANDLE), NULL);
	if (!ret)
	{
		printf ("ERROR (code 0x%x): Cannot cannot write the value of the STDOUT handle. \n", ret);
		return 1;
	}

	ret = SetThreadContext(procInfo.hThread, &threadContext);
	if (!ret)
	{
		printf ("ERROR (code 0x%x): Cannot set remote thead context. \n", ret);
		return 1;
	}
	ResumeThread(procInfo.hThread);

	/*// Wait until child process exits.
	WaitForSingleObject (procInfo.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle (procInfo.hProcess);
    CloseHandle (procInfo.hThread);*/
	return 0;
}