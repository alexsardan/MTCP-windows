/****************************************************************************
 * Checkpointing library entry point
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
//#include <stdio.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// A process is loading the DLL.
			// printf("A process is attaching winmtcp...\n");
		break;
		case DLL_THREAD_ATTACH:
			// A process is creating a new thread.
			// printf("A thread is attaching winmtcp...\n");
		break;
		case DLL_THREAD_DETACH:
			// A thread exits normally.
			// printf("A thread is detaching winmtcp...\n");
		break;
		case DLL_PROCESS_DETACH:
			// A process unloads the DLL.
			// printf("A process is detaching winmtcp...\n");
		break;
	}
	return TRUE;
}