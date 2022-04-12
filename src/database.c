/*
 * file: database.h
 * authors: 
        Carter S. Levinson <carter.levinson@pitt.edu>
        Qizhe Wang <qiw68@pitt.edu>
 *         
 * status: working
 */

#include "database.h"
#include "ntapfuse_ops.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <sqlite3.h>

#define TIME_MAX 80
#define BUF_MAX 600
#define QUOTA 1000000

/* global pointer to database connection object */
sqlite3 *DB;

/*TODO: add commented documentation here for each function sig*/
/*TODO: figure out meaning of null parameters in SQL exec */

int open_db() 
{       
    int rc1, rc2, rc3;
    char *err = NULL;
    const char *filename = "log.db";
    /* note: backslash method is the preferred way to do multline strings */
    const char *sql1 = "CREATE TABLE IF NOT EXISTS Logs(\
                               Time TEXT,\
                               UID INT,\
                               Operation TEXT,\
                               Size INT,\
                               Status TEXT,\
                               errorCode INT,\
                               Path TEXT);";
    /* why does this work? the backlash escapes the newline \n char! */
    const char *sql2 = "CREATE TABLE IF NOT EXISTS Quotas(\
                               Time TEXT,\
                               UID INT PRIMARY KEY,\
                               Usage INT,\
                               Remaining_Quota INT);";
    
    /* open the database connection */
    rc1 = sqlite3_open(filename, &DB);
    if (rc1 != SQLITE_OK) {
        /* we cast void* to string is this right? */
        fprintf(stderr, "Can't open database connection: %s\n", 
                      (char *) sqlite3_errmsg16(DB)); 
        sqlite3_close(DB);
        return -1;
    }
    rc2 = sqlite3_exec(DB, sql1, NULL, NULL,&err);
    if (rc2 != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    rc3 = sqlite3_exec(DB, sql2, NULL, NULL, &err);
    if (rc3 != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    return rc1 || rc2 || rc3;
}


void close_db() 
{
    sqlite3_close(DB);
}

int updateQuotas(char* time,int uid, int size )
{
    char *sql = NULL;
    char *sqlbuf = NULL;
    char *err = NULL;
    int rc;
    /* allocate heap data and check for succcess */
    sqlbuf = malloc(BUF_MAX);
    if (!sqlbuf) {
        fprintf(stderr,"ERROR: couldn't allocate SQL buffer\n");
        return -ENOMEM;
    }
    /*  here we search for the user's record, and update its usage and quota,
        and lastest quota alwasy equal initail quota - usage.
        size is positive, we need to increase usage using upsert(if record exist, update; 
        otherwise insert new record) */
    if (size >= 0) {
        sql = "INSERT INTO Quotas(Time,UID, Usage, Remaining_Quota) VALUES\
		(%s,%d,%d,%d-%d) ON CONFLICT(UID) DO UPDATE SET \
    		Time=%s,Usage=Usage+%d,Remaining_Quota=Remaining_Quota-%d;";
    } else {
        /* size is negative, meaning we need to decreasing usage, but the previous sqlite string doesn't work
        because it would become "usage=usage+-size", so we use a new string with a preset "-" in it. */
    	size *= -1;
    	sql = "INSERT INTO Quotas(Time,UID, Usage, Remaining_Quota) VALUES\
		(%s,%d,%d,%d-%d) ON CONFLICT(UID) DO UPDATE SET \
    		Time=%s,Usage=Usage-%d,Remaining_Quota=Remaining_Quota+%d;";
    }
    
    sprintf(sqlbuf,sql,addquote(time),
            uid,size,QUOTA,size,
            addquote(time),size,size);
    rc = sqlite3_exec(DB, sqlbuf, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL update error: %s\n", err);
        sqlite3_free(err);
    }
    free(sqlbuf);
    return rc;
}


int log_file_op(char *operation, char *path, size_t size, size_t usage, char* rstatus, int errorCode) 
{
    /* SQL commands to compile */
    const char *sql = "CREATE TABLE if not exists Logs(\
                        Time TEXT,\
                        UID INT,\
                        Operation TEXT,\
                        Size INT,\
                        Status TEXT,\
                        errorCode INT,\
                        Path TEXT);\
                        INSERT INTO Logs\
                           VALUES(%s, %d, %s, %d, %s,%d, %s);";  
    char *err = NULL;
    char *sqlbuf = NULL;
    char *timebuf = NULL;
    int uid;
    int updateSize;
    time_t now;
    int rc,rc1;
    /* allocate heap data and check for succcess */
    sqlbuf = malloc(BUF_MAX);
    if (!sqlbuf) {
        fprintf(stderr,"ERROR: couldn't allocate SQL buffer\n");
        return -ENOMEM;
    }
    timebuf = malloc(TIME_MAX);
    if (!timebuf) {
        fprintf(stderr,"ERROR: couldn't allocate Time buffer\n");
        return -ENOMEM;
    }
    /* get user id */
    uid = getuid();  
    /* get current time and convert it to readable */
    time(&now);
    strftime(timebuf, TIME_MAX, "%c",localtime(&now)); 
    /* read information into buffer s */
    sprintf(sqlbuf, sql, addquote(timebuf),
                    uid, addquote(operation), 
                    size, addquote(rstatus),errorCode,addquote(path));
    /* execute the SQL statement & check for success*/
    rc = sqlite3_exec(DB, sqlbuf, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    /* update usage */
    updateSize = (int) usage;
    
    /* check for growth in usage */
    if (strcmp(operation,"Write") == 0 || strcmp(operation,"Mkdir") == 0) {
        updateQuotas(timebuf,uid,updateSize);
    }
    /*check for shrinking usage */
    if(strcmp(operation,"Unlink") == 0 || strcmp(operation,"Rmdir") == 0) {
        updateSize *= -1;
        updateQuotas(timebuf,uid,updateSize);

    }
    /* free all heap data */
    free(sqlbuf);
    free(timebuf);
    return rc;
}



/* 
 * i don't know if we can rely on coreutils for ever, so we should
 * look into changing this. see
 * https://stackoverflow.com/questions/472697/how-do-i-get-the-size-of-a-directory-in-c
 * for other optiosn
 */
int getDirSize(char* dirPath) 
{
    FILE *fp;
    int size;
    char sizeChar[100];
    char dir[100];
	char res[100];
    char commandBuf[100];
	char *command = "du -s %s";

    sprintf(commandBuf,command,dirPath);
	fp = popen(commandBuf,"r");
	
	int ret = fscanf(fp,"%s,%s",sizeChar,dir);
    if(ret>=0) {
        size = atoi(sizeChar)*1024;
    }else {
        return ret;
    } 
    return size;
}


