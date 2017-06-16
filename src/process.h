/*
   Copyright 2017 <fariouche@yahoo.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <sys/types.h>
#include <unistd.h>

pid_t exec_process(const char* pname, const char** pargs);
int wait_process(pid_t pid);

#endif /* _PROCESS_H_ */
