#ifndef LOADDLL_H
#define LOADDLL_H

#include <Windows.h>

typedef struct 
{
	long long interval;
	HANDLE mainThread;
} ckpThreadArgs_t;

#endif