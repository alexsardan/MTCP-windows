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
#include "remoteprctl.h"
#include "../winmtcp/winmctp_createChkpt.h"
#include "util\list.h"

typedef struct {
	chkptMemInfo_t *chkptMemInfo;
	void *memBuffer;
} list_entry;

void createDummyProcess (PROCESS_INFORMATION *procInfo)
{
	STARTUPINFO procSi;

	ZeroMemory (&procSi, sizeof(STARTUPINFO));
	procSi.cb = sizeof(STARTUPINFO);
	procSi.dwFlags |= STARTF_USESTDHANDLES;
	ZeroMemory (procInfo, sizeof(PROCESS_INFORMATION));


	if( !CreateProcess (NULL, "dummyproc.exe", NULL, NULL, TRUE, 
						CREATE_SUSPENDED, NULL, NULL, &procSi, procInfo)) 
    {
        printf ("Process creation failed: Error %d.\n", GetLastError());
        exit (1);
    }
}

int main (int argc, char **argv)
{
	PROCESS_INFORMATION procInfo;
	CONTEXT threadContext;
	chkptMemInfo_t *chkptMemInfo;
	void *memBuffer = NULL;
	SIZE_T bufferSize = 0;
	FILE *pFile;
	SYSTEM_INFO si;
	int nread;
	list_entry *entry, *tempEntry;
	ULONG_PTR oldAllocationBase = 0x0;
	list_t memInfoList;
	SIZE_T allocationSize = 0x0;
	int i;
	node_t *current;

	list_init (&memInfoList);

	if (argc < 2) 
	{
		printf("Usage: %s <checkpoint_file>\n", argv[0]);
		exit(1);
	}

	/* create a dummy process that will have it's address space replaced */
	createDummyProcess (&procInfo);
	FlushInstructionCache (procInfo.hProcess, NULL, 0);

	if (!clearTargetMemory (procInfo))
		exit (1);
	printf ("Process memory cleared!\n");

	pFile = fopen (argv[1], "rb");
	if (pFile == NULL) {
		printf ("Error opening file: %s\n", argv[1]);
		exit(1);
	}

	nread = fread (&threadContext, sizeof(threadContext), 1, pFile);
	if (nread != 1) {
		printf ("Problem reading threadcontext.\n");
		fclose(pFile);
		exit(1);
	}

	while (!feof(pFile)) {
		entry = (list_entry *) malloc(sizeof(list_entry));
		chkptMemInfo = (chkptMemInfo_t *) malloc (sizeof(chkptMemInfo_t));

		nread = fread (chkptMemInfo, sizeof(chkptMemInfo_t), 1, pFile);
		if (nread != 1) {
			printf ("Problem reading chkpt mem info: %d.\n", nread);
			//fclose(pFile);
			//exit(1);
			break;
		}

		memBuffer = malloc(chkptMemInfo->meminfo.RegionSize * sizeof(char));

		if (chkptMemInfo->hasData == TRUE) {
			nread = fread (memBuffer, sizeof(char), chkptMemInfo->meminfo.RegionSize, pFile);
			if (nread != chkptMemInfo->meminfo.RegionSize) {
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
			
			if (memInfoList.size != 0) {

				tempEntry = (list_entry*)current->data;

				/* allocate memory for the remote process */
				allocTargetMemory(procInfo, tempEntry->chkptMemInfo->meminfo, allocationSize);
				oldAllocationBase = chkptMemInfo->meminfo.AllocationBase;
			
				/* set protections and write data */
				for (i = 0; i < memInfoList.size; i++)
				{
					tempEntry = (list_entry*)current->data;
					chkptMemInfo = tempEntry->chkptMemInfo;
					memBuffer = tempEntry->memBuffer;

					if (!setTargetMemory(procInfo, chkptMemInfo->meminfo, memBuffer, chkptMemInfo->hasData))
					{
						printf("error\n");
						//exit(1);
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
	SetThreadContext(procInfo.hThread, &threadContext);
	ResumeThread(procInfo.hThread);

	/*// Wait until child process exits.
	WaitForSingleObject (procInfo.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle (procInfo.hProcess);
    CloseHandle (procInfo.hThread);*/
	return 0;
}