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

/* Clears a remote process' memory */
BOOL clearTargetMemory (PROCESS_INFORMATION procInfo)
{
	MEMORY_BASIC_INFORMATION memInfo;
	SYSTEM_INFO sysInfo;
	list_t fileViewAddrList;
	DWORD err;
	ULONG_PTR targetEndAddr, currentAddr;
	HMODULE ntDllModule;
	UnmapViewOfFileRemoteFp unmapViewOfFileRemote;
	
	ntDllModule = GetModuleHandle("ntdll.dll");

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
	targetEndAddr = (ULONG_PTR) sysInfo.lpMaximumApplicationAddress;
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
		if ((memInfo.AllocationBase == ntDllModule) || (memInfo.BaseAddress == ntDllModule))
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