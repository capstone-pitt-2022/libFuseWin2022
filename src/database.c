#include "database.h"
#include "ntapfuse_ops.h"
#include <unistd.h>
//#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <sqlite3.h>

#define TIME_MAX 80
#define BUF_MAX 600

/* global pointer to database connection object */
sqlite3 *DB;

/*TODO: add commented documentation here for each function sig*/
/*TODO: more comments in general to document behavior of code */
/*TODO: update the quotas database on log_read and log write */

int open_db() {
        int rc, rc1, rc2;
        char *err = NULL;
        const char *filename = "log.db";
        /* note: backslash method is the preferred way to do multline strings */
        const char *sql1 = "CREATE TABLE IF NOT EXISTS Logs(\
                                   Time TEXT,\
                                   UID INT,\
                                   Operation TEXT,\
                                   Size INT,\
                                   Path TEXT);";

        /* why does this work? the backlash escapes the newline \n char! */
        const char *sql2 = "CREATE TABLE IF NOT EXISTS Quotas(\
                                   Time TEXT,\
                                   UID INT,\
                                   USAGE INT,\
                                   Limit INT);";
        
        /* do we need to insert path shenagains here */
        rc = sqlite3_open(filename, &DB);

        if (rc) {
                /* we cast void* to string is this right? */
                fprintf(stderr, "Can't open database connection: %s\n", 
                              (char *) sqlite3_errmsg16(DB)); 
                sqlite3_close(DB);
                return 1;
        }


        rc1 = sqlite3_exec(DB, sql1, NULL, NULL,&err);

        if (rc1 != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", err);
                sqlite3_free(err);
        }
        rc2 = sqlite3_exec(DB, sql2, NULL, NULL, &err);

        if (rc2 != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", err);
                sqlite3_free(err);
        }


        return rc && rc1 && rc2;
}


void close_db() {
        sqlite3_close(DB);
}

int log_read(char *operation, char *path, size_t size) {
        const char *filename = "log.db";
        const char *sql = "CREATE TABLE if not exists Logs(\
                               Time TEXT,\
                               UID INT,\
                               Operation TEXT,\
                               Size INT,\
                               Path TEXT);\
                           INSERT INTO Logs\
                               VALUES (%s, %d, %s, %d, %s);";

        //char workdir[PATH_MAX];
        //char logpath[PATH_MAX];
        //char *logfile;
        
        time_t now;

        char *sqlbuf; 
        char timebuf[TIME_MAX];

        char *err = NULL;
        int rc;
        
        //getcwd(workdir, PATH_MAX);
        //fullpath(workdir, logpath);
        /* use realpath to get this to compile for now */

        //logfile = strcat(logpath, filename);

        sqlbuf = malloc(BUF_MAX);
        time(&now);

        strftime(timebuf, TIME_MAX, "%c", localtime(&now));

        sprintf(sqlbuf, sql, addquote(timebuf), 
                        "0", /*hard code user rn */
                        addquote(operation),
                        size, addquote(path));

        rc = sqlite3_exec(DB, sqlbuf, NULL, NULL, &err);
        if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", err);
                sqlite3_free(err);
        }

        free(sqlbuf);
        return rc;
}

int log_write(char *operation, char *path, size_t size){

  char *filename = "log.db";
  int rc;
  //sqlite3 *DB;
	
  /* Changed: should use NULL instead of zero for ptr*/
  char *err = NULL;

  /*Changed commented out stuff that might not be necessary now */
  //char workdir[PATH_MAX];
  //char logpath[PATH_MAX];
  
  //getcwd(workdir,PATH_MAX); // get working directory
  //fullpath (workdir, logpath); // convert to full path
  //filename = strcat(logpath,filename); //add filename to the path
  
  //sqlite3_open(filename,&DB);
  
  /* changed: format to match new # of cols */
  /* write record to a table called Logs */
  char *sql ="CREATE TABLE if not exists Logs(\
                  Time TEXT,\ 
                  UID INT,\ 
                  Operation TEXT,\
                  Size INT,\
                  Path TEXT);\ 
              INSERT INTO Logs\
                  VALUES(%s, %d, %s, %d, %s);";  
  
  /*Changed: we shoul not cast the type of malloc */
  /*Changed: use macros instead of constant */
  char *bufsql = malloc(BUF_MAX);
  time_t now;
  time(&now);
  /*Changed: use macros instead of constant */
  char timebuf[TIME_MAX];
  strftime(timebuf, TIME_MAX,"%c",localtime(&now)); // convert time to readable
  /* Changed: hard code user for now */
  sprintf(bufsql, sql, addquote(timebuf),
                  "0", addquote(operation),
                  size, addquote(path));

  /* Changed: should use null instead of zero again */
  rc = sqlite3_exec(DB, bufsql, NULL, NULL, &err);

  /* Changed: add error handling */
  if (rc != SQLITE_OK) {
          fprintf(stderr, "SQL error: %s\n", err);
          sqlite3_free(err);
  }
	
  //sqlite3_close(DB);

  /* Changed: free the sqlbuf avoid memory leaks */
  free(bufsql);

  /*Changed return the result of the sqlite_exec */
  return rc;
}
