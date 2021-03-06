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

#include "syscalls.h"

/**
* Syscall for writing data to a file - WriteFile
*/
HANDLE WINAPI MtcpWriteFile(
  _In_         HANDLE hFile,
  _In_         LPCVOID lpBuffer,
  _In_         DWORD nNumberOfBytesToWrite,
  _Out_opt_    LPDWORD lpNumberOfBytesWritten,
  _Inout_opt_  LPOVERLAPPED lpOverlapped
)
{
	int syscall = syscallsNo[0];

	return NULL;
}


/**
* Syscall for opening a file - CreateFile
*/
HANDLE WINAPI MtcpCreateFile(
	_In_      LPCTSTR lpFileName,
	_In_      DWORD dwDesiredAccess,
	_In_      DWORD dwShareMode,
	_In_opt_  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_      DWORD dwCreationDisposition,
	_In_      DWORD dwFlagsAndAttributes,
	_In_opt_  HANDLE hTemplateFile) 
{

	return NULL;
}

/**
* Syscall for opening a file - CreateFile
* Needed to open the checkpoint file at restart time
*/