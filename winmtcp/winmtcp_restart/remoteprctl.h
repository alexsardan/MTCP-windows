#ifndef REMOTEPRCTL_H
#define REMOTEPRCTL_H

typedef NTSTATUS (*NtQueryInformationThreadFp) (HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS (*UnmapViewOfFileRemoteFp)(HANDLE, PVOID);

BOOL clearTargetMemory (PROCESS_INFORMATION procInfo);
BOOL setTargetMemory (PROCESS_INFORMATION procInfo, MEMORY_BASIC_INFORMATION memInfo, void *buff, BOOL hasBuffer);
BOOL allocTargetMemory(PROCESS_INFORMATION procInfo, MEMORY_BASIC_INFORMATION memInfo, SIZE_T allocSize);

#endif