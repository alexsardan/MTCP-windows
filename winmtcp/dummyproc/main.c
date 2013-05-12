/****************************************************************************
 * Dummy app that will have it's address space replaced with 
 * the restarted checkpoint
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

int main (int argc, char **argv)
{
	DWORD tid;
	HANDLE thisThread;

	tid = GetCurrentThreadId();

	if ((thisThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid)) == NULL)
		return 1;
	
	if (SuspendThread(thisThread) == -1)
		return 1;
	return 0;
}