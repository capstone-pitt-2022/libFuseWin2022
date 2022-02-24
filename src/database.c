#include "database.h"
#include "ntapfuse_ops.h"
//#include <fuse.h> ?
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <sqlite3.h>

/*global pointer to database connection object */
sqlite3 *DB;

int init_db() {
        int rc, rc1, rc2;
        char *sql1, sql2;
        char *err = NULL;
        char *filename = "log.db";

        rc = sqlite3_open(filename, &DB);

        if (rc) {
                fprintf(stderr, "Can't open database connection: %s\n", sqlite3_errmsg16(DB));
                sqlite3_close(DB);
                return 1;
        }

        *sql1 = "CREATE TABLE IF NOT EXISTS Logs("
                        "UID INT,"
                        "Time TEXT,"
                        "Operation TEXT,"
                        "Size INT,"
                        "Path TEXT);";

        rc1 = sqlite3_exec(DB, sql1, NULL, NULL,&err);

        if (rc1 != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", err);
                sqlite3_free(err);
        }
        *sql2 = "CREATE TABLE IF NOT EXISTS Quotas("
                        "UID INT,"
                        "USAGE INT,"
                        "Limit INT);";

        rc2 = sqlite3_exec(DB, sql2, NULL, NULL, &err);

        if (rc2 != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", err);
                sqlite3_free(err);
        }


        return rc && rc1 && rc2;
}


void destroy_db() {
        sqlite3_close(DB);
}

int log_write(char *operation, char *path, size_t size){

  char *filename = "log.db";
  int rc;
  //sqlite3 *DB;
	
  char *err=0;

  char workdir[PATH_MAX];
  char logpath[PATH_MAX];
  
  getcwd(workdir,PATH_MAX); // get working directory
  fullpath (workdir, logpath); // convert to full path
  filename = strcat(logpath,filename); //add filename to the path
  
  //sqlite3_open(filename,&DB);
  
  char *sql ="CREATE TABLE if not exists Logs(UID INT, Time TEXT, Operation TEXT, Size INT, Path TEXT);" 
            "INSERT INTO Ops VALUES(%s, %s, %d,%s);" ;  // write record to a talbe called Logs
  
  char *bufsql = (char*)malloc(600);
  time_t now;
  time(&now);
  char timebuf[80];
  strftime(timebuf,80,"%c",localtime(&now)); // convert time to readable
  sprintf(bufsql,sql,addquote(timebuf),addquote(operation),size,addquote(path));

  rc = sqlite3_exec(DB,bufsql,0,0,&err);
  if (rc != SQLITE_OK) {
          fprintf(stderr, "SQL error: %s\n", err);
          sqlite3_free(err);
  }
	
  //sqlite3_close(DB);

  return rc;
}
