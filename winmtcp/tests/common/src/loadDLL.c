/****************************************************************************
 * Helper functions for dynamically loading DLLs
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

#include <stdio.h>
#include "../include/loadDLL.h"

typedef int (*ckpInitFunc_t) (long long);

HINSTANCE LoadDLL (char *filename)
{
	DWORD err;
	HINSTANCE hLibrary;
	hLibrary = LoadLibrary(filename);
	if (hLibrary == NULL)
	{
		err = GetLastError();
		printf ("Cannot load dynamic library \"%s\". Error: %d\n", filename, err);
		return NULL;
	}
	return hLibrary;
}