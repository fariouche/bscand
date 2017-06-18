/*
   Copyright 2017 <fariouche@yahoo.fr>

   The MIT License

   Permission is hereby granted, free of charge, to any person obtaining a copy 
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights 
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
   copies of the Software, and to permit persons to whom the Software is 
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  
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
