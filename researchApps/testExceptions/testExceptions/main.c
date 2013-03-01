/**************************************************************************
 * Research app in order to figure out how to
 * transparrently raise an exception in a running 
 * thread from another thread.
 *
 * The goal is to get all the threads at a single
 * point in the exception handler and make them
 * wait at a futex until the checkpoint is complete.
 * This technique is used in DMTCP.
 *
 * We may not need to use exceptions, SuspendThread()
 * and ResumeThread() alone should serve our purpose well.
 * Still it's a great exercise.
 *
 * Copyright (C) 2013, Alexandru-Cezar Sardan
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
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define ITERATIONS 10
typedef VOID (CALLBACK *EXCEPTION_THROWER_FUNC)();

HANDLE hThreads[2];

VOID CALLBACK threadAPC(ULONG_PTR dwParam)
{
	printf("Thread %d executed APC func.\n");
}

//TODO: add parameter to perserve the modified EIP
VOID CALLBACK throwCustomException()
{
	RaiseException(992, 0, 0, NULL);
}

LONG CALLBACK CustomExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	DWORD code;
	printf("Custom handler was called!\n");
	code = ExceptionInfo->ExceptionRecord->ExceptionCode;
	if (code == 992)
		printf("Yay! In the right exception!!\n");
	Sleep(5000);
	printf("Exiting exception handler..\n");
	return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI thredFunc(LPVOID lpParamater)
{
	int counter = 0, i, param;
	param = *((int*)lpParamater);

	for (i = 0; i < ITERATIONS; i++)
	{
		printf("Thread %d value is: %d!\n", param, param+i);
		Sleep(2000+param*10);
	}
	return 0;
}

void RaiseExceptionInThread(HANDLE hThread, EXCEPTION_THROWER_FUNC exFunc)
{
	DWORD err;
	CONTEXT threadContext;
	threadContext.ContextFlags = CONTEXT_CONTROL;
	err = SuspendThread(hThread);
	if (err != -1)
	{
		while(!GetThreadContext(hThread, &threadContext))
		{
			printf("Cannot get context. Error: %d\n", GetLastError());
			threadContext.ContextFlags = CONTEXT_CONTROL;
		}
		//TODO: save Eip and put it on the stack so that exFunc can get it
		//and pass it to the exception handler. The exception handler
		//should restore things as they were before returning EXCEPTION_CONTINUE_EXECUTION
		threadContext.Eip = (DWORD) (DWORD_PTR) exFunc;
		while(!SetThreadContext(hThread, &threadContext));
		while(!ResumeThread(hThread));
	}
}

int main(int argc, char **argv)
{
		int tid1 = 10, tid2 = 30;
		AddVectoredExceptionHandler(1, CustomExceptionHandler);
		
		if ((hThreads[0] = CreateThread(NULL, 0, thredFunc, &tid1, 0, NULL)) == NULL)
		{
			printf ("Thread1 cannot be created! Error: %d", GetLastError());
			return -1;
		}
		if ((hThreads[1] = CreateThread(NULL, 0, thredFunc, &tid2, 0, NULL)) == NULL)
		{
			printf ("Thread2 cannot be created! Error: %d", GetLastError());
			return -1;
		}
		printf("Threads created!\n");
		Sleep(6000);
/*		
		printf("Queueing user APCs...\n");
		QueueUserAPC(threadAPC, hThreads[0], tid1);
		QueueUserAPC(threadAPC, hThreads[1], tid2);
		Sleep(3000);
*/
		printf("Raising exception in all threads!\n");
		RaiseExceptionInThread(hThreads[0], throwCustomException);
		RaiseExceptionInThread(hThreads[1], throwCustomException);

		WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);

		return 0;
}