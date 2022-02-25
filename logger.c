#include <stdio.h>
#include <time.h>
#include "logger.h"

void log_write(int ret, const char* buf,const char *path)
{
   FILE *fp; 
   fp = fopen("log.txt", "a"); 
   //get the time 
   time_t t; 
   time(&t); 
   //if f>=0, no error, otherwise error
   if(ret>=0)
   {
      fprintf(fp,"Successfully wrote %d bytes\n", ret);
      fprintf(fp,"Wrote: %s", buf);
      fprintf(fp,"Date: %s", ctime(&t)); 
      fprintf(fp,"Path: %s\n", path); 
      fprintf(fp,"\n");
   }
   else
   {
      fprintf(fp,"Unsuccessful write\n");
      fprintf(fp,"Intended to write: %s\n", buf);
      fprintf(fp,"Date: %s", ctime(&t)); 
      fprintf(fp,"Path: %s\n", path); 
      fprintf(fp,"\n");
   }
   fclose(fp);

}
