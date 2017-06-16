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

#include "process.h"
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>

#define NO_STDOUT

pid_t exec_process(const char* pname, const char** pargs)
{
  pid_t pid,sid;
  
  if((pid = fork()) == (pid_t)-1)
  {
    fprintf(stderr, "could not fork\n");
    return -1;
  }
  else if(pid == 0)
  {
#ifdef NO_STDOUT
    {
      int fd;
      fd = open("/dev/null", O_WRONLY);
      if(fd < 0)
      {
        fprintf(stderr, "Cannot open /dev/null\n");
        return -1;
      }
      /* redirect stdout to /dev/null */
      dup2(fd, STDOUT_FILENO);

      /* redirect stderr to /dev/null */
      dup2(fd, STDERR_FILENO);

      if(fd >= 0)
        close(fd);
    }
#endif        
    //fclose(stdin); //some sub-processes do not like that...
    
    sid = setsid();
    if (sid < 0) 
    {
        fprintf(stderr, "Error setsid()");
        return -1;
    }
    execvp(pname, (char* const*)pargs);
    exit(1); /* should never reach here */
  }
  return pid;
}


int wait_process(pid_t pid)
{
  int status = -1;
  
  while(pid != -1)
  {
    pid = waitpid(pid, &status, 0);
    if(pid == -1)
    {
      fprintf(stderr, "waitpid error...\n");
      return -1;
    }
    else if(WIFEXITED(status) || WIFSIGNALED(status))
    {
      fprintf(stderr, "process (pid %d) exited with status %d\n", pid, status);
      break;
    }
  }
  
  return status;
}
