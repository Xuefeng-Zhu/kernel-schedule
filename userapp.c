#include "userapp.h"

int fibonacci(int n)
{
   if (n <= 0) 
      return 0;
   else if (n == 1)
      return 1;
   else
      return fibonacci(n-1) + fibonacci(n-2);
} 

int main(int argc, char* argv[])
{
   unsigned long pid = getpid();
   char echo_buf[50];

   sprintf(echo_buf, "echo '%lu'>/proc/mp1/status", pid);
   system(echo_buf);
     
   fibonacci(50);
   return 0;
}
