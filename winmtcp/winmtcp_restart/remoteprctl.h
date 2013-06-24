#ifndef REMOTEPRCTL_H
#define REMOTEPRCTL_H

typedef NTSTATUS (*NtQueryInformationThreadFp) (HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS (*UnmapViewOfFileRemoteFp)(HANDLE, PVOID);

typedef struct _THREAD_BASIC_INFORMATION {
  NTSTATUS        ExitStatus;
  PVOID           TebBaseAddress;
  HANDLE          ClientIdLow;
  HANDLE		  ClientIdHigh;
  KAFFINITY       AffinityMask;
  LONG            Priority;
  LONG            BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

BOOL clearTargetMemory (PROCESS_INFORMATION procInfo);
BOOL setTargetMemory (PROCESS_INFORMATION procInfo, MEMORY_BASIC_INFORMATION memInfo, void *buff, BOOL hasBuffer);
BOOL allocTargetMemory(PROCESS_INFORMATION procInfo, MEMORY_BASIC_INFORMATION memInfo, SIZE_T allocSize);

#endif