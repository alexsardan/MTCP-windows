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

void createDummyProcess (PROCESS_INFORMATION *procInfo)
{
	STARTUPINFO procSi;

	ZeroMemory (&procSi, sizeof(STARTUPINFO));
	procSi.cb = sizeof(STARTUPINFO);
	ZeroMemory (procInfo, sizeof(PROCESS_INFORMATION));

	if( !CreateProcess (NULL, "dummyproc.exe", NULL, NULL, FALSE, 
						CREATE_SUSPENDED, NULL, NULL, &procSi, procInfo)) 
    {
        printf ("Process creation failed: Error %d.\n", GetLastError());
        exit (1);
    }
}

int main (int argc, char **argv)
{
	PROCESS_INFORMATION procInfo;

	/* create a dummy process that will have it's address space replaced */
	createDummyProcess (&procInfo);
	FlushInstructionCache (procInfo.hProcess, NULL, 0);

	if (!clearTargetMemory (procInfo))
		exit (1);
	printf ("Process memory cleared!\n");

	// Wait until child process exits.
	WaitForSingleObject (procInfo.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle (procInfo.hProcess);
    CloseHandle (procInfo.hThread);
	return 0;
}