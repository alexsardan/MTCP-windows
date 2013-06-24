/****************************************************************************
 * Remote process controll routines
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
#include <winternl.h>
#include <stdio.h>
#include "util\list.h"
#include "remoteprctl.h"
#include "utils.h"

extern HMODULE ntDllModule;
extern PROCESS_BASIC_INFORMATION dummyProcInfo;
extern ULONG_PTR dummyMainTEBAddr;
extern CONTEXT threadContext;
MEMORY_BASIC_INFORMATION oldMemInfo;

BOOL allocTargetMemory(PROCESS_INFORMATION procInfo, MEMORY_BASIC_INFORMATION memInfo, SIZE_T allocSize) 
{
	LPVOID retAddr;
	BOOL ret;

	/* check if only reserving memory */
	if (memInfo.State == MEM_RESERVE)
		memInfo.Protect = PAGE_NOACCESS;

	if ((retAddr = VirtualAllocEx(procInfo.hProcess, memInfo.AllocationBase, 
		allocSize, MEM_RESERVE, memInfo.Protect)) == NULL)
	{
		printf("ERROR (code %d): Cannot allocate memory in remote process: 0x%p.\n", GetLastError(), memInfo.AllocationBase);
		return FALSE;
	}

	if (retAddr != memInfo.BaseAddress)
	{
		printf("WARNING: Memory was not allocated in the right place 0x%p vs 0x%p.\n", retAddr, memInfo.BaseAddress);
		return FALSE;
	}

	return TRUE;
}

BOOL setTargetMemory (PROCESS_INFORMATION procInfo, MEMORY_BASIC_INFORMATION memInfo, void *buff, BOOL hasBuffer) 
{
	SIZE_T bytesWritten;
	LPVOID retAddr;
	BOOL removeWrite = FALSE;
	DWORD oldProtect;
	DWORD dummyProtect;

	/* check if memory region has a protection flag not supported by VirtualAllocEx */
	if (memInfo.Protect == PAGE_EXECUTE_WRITECOPY)
	{
		memInfo.Protect = PAGE_EXECUTE_READWRITE;
	}
	if (memInfo.Protect == PAGE_WRITECOPY)
	{
		memInfo.Protect = PAGE_READWRITE;
	}

	/*
	if (memInfo.BaseAddress == 0x7f7f6000) {
		return TRUE;
	}

	if (memInfo.BaseAddress == 0x7f7ff000) {
		return TRUE;
	}
	*/

	if (memInfo.State != MEM_RESERVE)
	{	
		if ((retAddr = VirtualAllocEx(procInfo.hProcess, memInfo.BaseAddress, 
			memInfo.RegionSize, MEM_COMMIT, memInfo.Protect)) == NULL)
		{
			printf("ERROR (code %d): Cannot COMMIT memory in remote process: 0x%p.\n", GetLastError(), memInfo.BaseAddress);
			return FALSE;
		}

		if (retAddr != memInfo.BaseAddress)
		{
			printf("ERROR: Memory was not allocated in the right place 0x%p vs 0x%p.\n", retAddr, memInfo.BaseAddress);
			return FALSE;
		}
	}
	else
		return TRUE;

	/* check if must write data to a READONLY page and temporally add write permissions */
	if (hasBuffer)
	{
		if ( (!(memInfo.Protect & PAGE_READWRITE)) && (!(memInfo.Protect & PAGE_EXECUTE_READWRITE)) ) 
		{
			removeWrite = TRUE;
			oldProtect = memInfo.Protect;
			/* READONLY page */
			if (memInfo.Protect & PAGE_READONLY) {
				memInfo.Protect &= ~PAGE_READONLY;
				memInfo.Protect |= PAGE_READWRITE;
			}
			/* EXECUTE_READ page */
			if (memInfo.Protect & PAGE_EXECUTE_READ) {
				memInfo.Protect &= ~PAGE_EXECUTE_READ;
				memInfo.Protect |= PAGE_EXECUTE_READWRITE;
			}
		}
	}

	if(!VirtualProtectEx(procInfo.hProcess, memInfo.BaseAddress, memInfo.RegionSize, memInfo.Protect, &dummyProtect))
	{
		printf ("ERROR (code %d): Cannot change protection to region: 0x%p.\n", GetLastError(), memInfo.BaseAddress);
		return FALSE;
	}

	if (hasBuffer) 
	{
		if (WriteProcessMemory(procInfo.hProcess, memInfo.BaseAddress, buff, memInfo.RegionSize, &bytesWritten) == 0)
		{
			printf("ERROR (code %d): Cannot write remote process memory.\n", GetLastError());
			return FALSE;
		}

		if (bytesWritten != memInfo.RegionSize)
		{
			printf("ERROR: Cannot write all memory in remote process.\n");
			return FALSE;
		}

		if (removeWrite) {
			if(!VirtualProtectEx(procInfo.hProcess, memInfo.BaseAddress, memInfo.RegionSize, oldProtect, &dummyProtect))
			{
				printf ("ERROR (code %d): Cannot change protection to region: 0x%p.\n", GetLastError(), memInfo.BaseAddress);
				return FALSE;
			}
		}
	}

	oldMemInfo = memInfo;

	return TRUE;
}

/* Clears a remote process' memory */
BOOL clearTargetMemory (PROCESS_INFORMATION procInfo)
{
	MEMORY_BASIC_INFORMATION memInfo;
	SYSTEM_INFO sysInfo;
	list_t fileViewAddrList;
	DWORD err;
	ULONG_PTR targetEndAddr, currentAddr;
	UnmapViewOfFileRemoteFp unmapViewOfFileRemote;
	BOOL ret;

	if (ntDllModule != NULL)
	{
		unmapViewOfFileRemote = (UnmapViewOfFileRemoteFp) GetProcAddress (ntDllModule, "NtUnmapViewOfSection");
		if (unmapViewOfFileRemote == NULL)
		{
			printf("ERROR (code %d): Cannot get a function pointer to NtUnmapViewOfSection.\n", GetLastError());
			return FALSE;
		}
	}
	else
	{
		printf("ERROR (code %d): Cannot get a handle to NTDLL.\n", GetLastError());
		return FALSE;
	}

	list_init (&fileViewAddrList);

	/* get userspace address space limits */
	GetSystemInfo(&sysInfo);

#ifdef _WIN64
	targetEndAddr = 0x000007FFFFFE0000;
#else
	targetEndAddr = 0x7FFE0000;//(ULONG_PTR) sysInfo.lpMaximumApplicationAddress;
#endif
	currentAddr = (ULONG_PTR) sysInfo.lpMinimumApplicationAddress;

	while (currentAddr < targetEndAddr)
	{
		ZeroMemory(&memInfo, sizeof(MEMORY_BASIC_INFORMATION));
		if (!VirtualQueryEx(procInfo.hProcess, (LPCVOID) currentAddr, 
							&memInfo, sizeof(MEMORY_BASIC_INFORMATION)))
		{
			printf("ERROR (code %d): VirtualQuerry failed.\n", GetLastError());
			return FALSE;
		}

		/* don't unmap ntdll.dll */
		/*
		if ((memInfo.AllocationBase == ntDllModule) || (memInfo.BaseAddress == ntDllModule))
		{
			currentAddr += memInfo.RegionSize;
			continue;
		}
		*/

		/* Don't try to unmap the PEB or the main TEB of the dummy process */
		if ((memInfo.AllocationBase == dummyProcInfo.PebBaseAddress) || 
			(memInfo.BaseAddress == dummyProcInfo.PebBaseAddress) ||
			((ULONG_PTR) memInfo.AllocationBase == (dummyMainTEBAddr)) || 
			((ULONG_PTR) memInfo.BaseAddress == (dummyMainTEBAddr)))
		{
			printf ("Found TEB/PEB and not unmaping\n");
			currentAddr += memInfo.RegionSize;
			continue;
		}

		/* Don't try to unmap WOW64 Shared User Data */
		if ((ULONG_PTR)memInfo.AllocationBase == 0x7FFE0000)
		{
			currentAddr += memInfo.RegionSize;
			continue;
		}

		/* memory already free */
		if (memInfo.State == MEM_FREE)
		{
			currentAddr += memInfo.RegionSize;
			continue;
		}

		/* if memory is a view of a file, store the address in the list*/
		if (memInfo.Type == MEM_MAPPED)
		{
			if (!NT_SUCCESS(unmapViewOfFileRemote(procInfo.hProcess, memInfo.AllocationBase)))
			{
				list_insert_back (&fileViewAddrList, memInfo.AllocationBase);
				printf("Remote unmapping failed.\n");
				return FALSE;
			}
			currentAddr += memInfo.RegionSize;
			continue;
		}
		
		/* clear the target process memory */
		if (!VirtualFreeEx(procInfo.hProcess, memInfo.AllocationBase, 0, MEM_RELEASE))
		{
			err = GetLastError();

			/* VirtualQuery incorrectly reports memory mapped executable files
			 * as MEM_IMAGE instead of MEM_MAPPED. We will add them to the
			 * list in order to clear them later.
			 */
			if (err == ERROR_INVALID_PARAMETER)
			{
				if (!NT_SUCCESS(unmapViewOfFileRemote(procInfo.hProcess, memInfo.AllocationBase)))
				{
					list_insert_back (&fileViewAddrList, memInfo.AllocationBase);
					printf("Remote unmapping failed.\n");
				}
			}
			else 
			{
				printf ("ERROR (code %d): Cannot remove target memory. \n", err);
				printMemoryBasicInfo(memInfo);
			}
		}
		currentAddr += memInfo.RegionSize;
	}

	printf ("Regions scheduled for clear in remote process:\n");
	list_print (fileViewAddrList);
	list_clear (&fileViewAddrList);
	return TRUE;
}