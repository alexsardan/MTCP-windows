#ifndef LOADDLL_H
#define LOADDLL_H

#include <Windows.h>

typedef struct 
{
	long long interval;
	HANDLE mainThread;
} ckpThreadArgs_t;

typedef struct _THREAD_BASIC_INFORMATION {
  NTSTATUS        ExitStatus;
  PVOID           TebBaseAddress;
  HANDLE          ClientIdLow;
  HANDLE		  ClientIdHigh;
  KAFFINITY       AffinityMask;
  LONG            Priority;
  LONG            BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

#endif