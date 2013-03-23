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
#include "winmtcp_main.h"
#include <stdio.h>

HANDLE ckpTimerHandle, ckpHeap;

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

	param = (ckpThreadArgs_t*)lpParamater;
	ckpTime.QuadPart = -10000000*param->interval;
	if (HeapFree(ckpHeap, 0, param) == 0)
		return 1;

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

		/* Rearm the timer */
		if (SetWaitableTimer(ckpTimerHandle, &ckpTime, 0, NULL, NULL, FALSE) == 0)
			return 1;
	}

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

	/* create the checkpointing thread that will handle the timer */
	if ((ckpThread = CreateThread(NULL, 0, ckpThreadFunc, ckpArgs, 0, NULL)) == NULL)
		return -1;

	/* TODO: init other stuff here (like syscall interception code) */

	return 0;
}