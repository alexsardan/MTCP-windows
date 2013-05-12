/****************************************************************************
 * Helper functions
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
#include <stdio.h>
#include "utils.h"

void printMemoryBasicInfo (MEMORY_BASIC_INFORMATION inf)
{
	printf ("-------------------------------------------\n");
	printf ("Base address: %p\n", inf.BaseAddress);
	printf ("Alloc base: %p\n", inf.AllocationBase);
	printf ("Region size: %llu\n", inf.RegionSize);
	printf ("Region type: ");
	if (inf.Type & MEM_IMAGE)
		printf ("MEM_IMAGE\n");
	else if (inf.Type & MEM_MAPPED)
		printf ("MEM_MAPPED\n");
	else if (inf.Type & MEM_PRIVATE)
		printf ("MEM_PRIVATE\n");
	printf ("Region state: ");
	if (inf.State & MEM_COMMIT)
		printf ("MEM_COMMIT\n");
	else if (inf.State & MEM_FREE)
		printf ("MEM_FREE\n");
	else if (inf.State & MEM_RESERVE)
		printf ("MEM_RESERVE\n");

	printf ("-------------------------------------------\n");
}