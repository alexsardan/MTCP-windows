#ifndef REMOTEPRCTL_H
#define REMOTEPRCTL_H

typedef NTSTATUS (*UnmapViewOfFileRemoteFp)(HANDLE, PVOID);

BOOL clearTargetMemory (PROCESS_INFORMATION procInfo);

#endif