/****************************************************************************
 * Simple hello world test program
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

#include <stdlib.h>
#include <stdio.h>
#include "../common/include/loadDLL.h"

typedef int (*ckpInitFunc_t) (long long);
int theInt;

void printNum(int num)
{
	HANDLE hStdout;
	DWORD ssize;
	char buffer[65];

	itoa(num, buffer, 10);
	if( (hStdout = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE )  
		exit (1);
	WriteFile(hStdout, buffer, strlen(buffer), &ssize, NULL);
	//CloseHandle(hStdout);
}

int main (int argc, char **argv)
{
	HINSTANCE lib;
	unsigned long long i, j;
	ckpInitFunc_t mtcpInit;
	theInt = 0;
	printf("Adresa lui theInt este: %p", &theInt);
	if ((lib = LoadDLL ("winmtcp.dll")) == NULL)
		return 1;
	mtcpInit = (ckpInitFunc_t) GetProcAddress (lib, "winmtcp_init");
	if (mtcpInit == NULL)
		return 1;
	if (mtcpInit (30ll) == -1)
		return 1;
	//printf ("Hello World\n");
	for (i = 0; i < 100ull; i++) {
		printNum (i);
		j = 0;
		theInt++;
		//sleep a little
		while (j++ < 10000000000ul);
	}
	return 0;
}