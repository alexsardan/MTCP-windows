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
#include <stdio.h>
#include "winmctp_createChkpt.h"

#define CHKPT_FILE_NAME_L L"checkpoint.mtcp"
#define CHKPT_FILE_NAME "checkpoint.mtcp"

/* creates a checkpoint for the current process
 * returns 0 if succesfully created checkpoint,
 * return -1 if error occured
 */
int createCheckpoint(HANDLE mainThread) {

	//HANDLE hChkptFile;
	HANDLE procHandle;
	MEMORY_BASIC_INFORMATION meminfo;
	ULONG addr = 0x00000000;
	SIZE_T size;
	BOOL errorFlag;
	char *memoryBuffer = NULL;
	SIZE_T bufferSize = 0;
	SIZE_T bytesRead, elWritten;
	int ret;
	FILE *chkptFile, *log;
	CONTEXT threadContext;

	/* create checkpoint file */
	/*
	hChkptFile = CreateFile(
		CHKPT_FILE_NAME,		// name of the write
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,             // create new file only
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

	if (hChkptFile == INVALID_HANDLE_VALUE) 
	{ 
		printf("Unable to create file %s\n", CHKPT_FILE_NAME);
		return;
	}*/

	/* suspend main thread */
	ret = SuspendThread(mainThread);
	if (ret == ((DWORD) - 1)) 
	{
		printf("Cannot suspend main thread.\n");
		return 1;
	}
	else
		printf("Suspended main thread\n");

	/* open checkpoint file */
	chkptFile = fopen(CHKPT_FILE_NAME, "wb");
	log = fopen("log.txt", "wb");

	if (chkptFile == NULL) 
	{
		printf("Unable to create file %s\n", CHKPT_FILE_NAME);
		return 1;
	}

	/* save main thread's context */
	GetThreadContext(mainThread, &threadContext);

	/* write thread context on disk */
	elWritten = fwrite (&threadContext, sizeof(threadContext), 1, chkptFile);
	if (elWritten != 1) {
		fprintf(log, "Failed writing memory info on disk\n");
		fclose(chkptFile);
		fclose(log);
		return 1;
	}
	else {
		fprintf(log, "Succesfully written thread context on disk\n");
	}

	/* get handle for the current process */
	procHandle = GetCurrentProcess();

	/* scan and write to the checkpoint file all the process memory */
	do 
	{
		/* query process memory */
		size = VirtualQuery((LPCVOID) addr, &meminfo, sizeof(MEMORY_BASIC_INFORMATION));
		if (!size)
		{
			fprintf(log, "Failed with error %d.\n", GetLastError());
			break;
		}

		/* compute next address to query */
		addr = (ULONG)meminfo.BaseAddress + meminfo.RegionSize;

		fprintf(log, "Adddress Range :0x%08x - 0x%08x\n", meminfo.BaseAddress, addr);
		if (meminfo.State == MEM_FREE)
		{
			fprintf(log, "Skipping free memory\n");
			fprintf(log, "**********************************************\n");
			continue;
		}

		/* prepare the buffer for reading process memory */
		if (bufferSize <  meminfo.RegionSize) 
		{ 
			memoryBuffer = realloc(memoryBuffer, meminfo.RegionSize * sizeof(char));
			bufferSize = meminfo.RegionSize;
		}

		errorFlag = ReadProcessMemory(procHandle, meminfo.BaseAddress, memoryBuffer, meminfo.RegionSize, &bytesRead);
		/* check status of process memory reading */
		if (errorFlag == 0) 
		{
			fprintf(log, "Size read : 0x%08x\n", bytesRead);
			fprintf(log, "Reading process memory failed with error %d\n", GetLastError());
		}
		else 
		{
			fprintf(log, "Succesfully read process memory\n");
			elWritten = fwrite (&meminfo, sizeof(meminfo), 1, chkptFile);
			if (elWritten != 1)
				fprintf(log, "Failed writing memory info on disk\n");

			elWritten = fwrite (memoryBuffer, sizeof(char), bytesRead, chkptFile);
			if (elWritten < bytesRead)
				fprintf(log, "Failed writing entire process memory on file\n");
			else
				fprintf(log, "Succesfully written process memory on disk\n");
		}

		fprintf(log, "**********************************************\n");

	} while(1);

	fclose(chkptFile);
	fclose(log);
	free (memoryBuffer);

	/* suspend main thread */
	ret = ResumeThread(mainThread);
	if (ret == ((DWORD) - 1)) 
	{
		printf ("Failed resuming main thread\n");
		return 1;
	}
	printf ("Main thread resumed\n"); 

	return 0;
}