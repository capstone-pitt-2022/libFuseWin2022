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
#define QUOTA 10000000

/* global pointer to database connection object */
sqlite3 *DB;

/*TODO: add commented documentation here for each function sig*/
/*TODO: figure out meaning of null parameters in SQL exec */

int open_db() {
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
                           NumOfFiles INT,\
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


void close_db() {
        sqlite3_close(DB);
}

int log_file_op(char *operation, char *path, size_t size, int uid, char* rstatus, int errorCode) {
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
        
    /* free all heap data */
    free(sqlbuf);
    free(timebuf);
    return rc;
}

int updateQuotas(char* time,int uid, int size, int fileChange )
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
        sql = "INSERT INTO Quotas(Time,UID, NumOfFiles,Usage, Remaining_Quota) VALUES\
		(%s,%d,%d,%d,%d-%d) ON CONFLICT(UID) DO UPDATE SET \
    		Time=%s,NumOfFiles=NumOfFiles+%d,Usage=Usage+%d,Remaining_Quota=Remaining_Quota-%d;";
    } else {
        /* size is negative, meaning we need to decreasing usage, but the previous sqlite string doesn't work
        because it would become "usage=usage+-size", so we use a new string with a preset "-" in it. */
    	size *= -1;
        fileChange *= -1;
    	sql = "INSERT INTO Quotas(Time,UID, NumOfFiles,Usage, Remaining_Quota) VALUES\
		(%s,%d,%d,%d,%d-%d) ON CONFLICT(UID) DO UPDATE SET \
    		Time=%s,NumOfFiles=NumOfFiles-%d,Usage=Usage-%d,Remaining_Quota=Remaining_Quota+%d;";
    }
    
    sprintf(sqlbuf,sql,addquote(time),
            uid,fileChange,size,QUOTA,size,
            addquote(time),fileChange,size,size);
    rc = sqlite3_exec(DB, sqlbuf, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL update error: %s\n", err);
        sqlite3_free(err);
    }
    free(sqlbuf);
    return rc;
}

/* 
 * i don't know if we can rely on coreutils for ever, so we should
 * look into changing this. see
 * https://stackoverflow.com/questions/472697/how-do-i-get-the-size-of-a-directory-in-c
 * for other optiosn
 */
int getDirSize(char* dirPath) {
        FILE *fp;
	char res[100];
        char commandBuf[100];
	char *command = "du -sh %s";
        sprintf(commandBuf,command,dirPath);
	fp = popen(commandBuf,"r");
	fread(res,50,1,fp);
        int size = atoi(strtok(res, " "))*1024;
	printf("%d\n",size);
        return size;
}

//returns the current user's usage 
/*
TODO:  IMPLEMENT ERROR HANDLING!!!
*/
int getUsage(int uid)
{
    int rc;
    char *err = NULL;
    //holds the SQL command w/ placeholders 
    char *sql = NULL;
    //holds the SQL command w/ placeholders filled in 
    char *sqlbuf = malloc(BUF_MAX);//allocate space 
    //define the SQL command 
    sql ="SELECT Usage FROM Quotas WHERE UID = %d;";
    //place it into sqlbuf w/ getuid()
    sprintf(sqlbuf,sql,uid);
    //stmt to be executed 
    sqlite3_stmt * stmt;
    //prepare the statement with the proper parameters 
    rc = sqlite3_prepare_v2(DB, sqlbuf, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL prepare error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    //execute statement 
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        fprintf(stderr, "SQL step error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    //retrieve the usage from the table 
    int usage = sqlite3_column_int(stmt,0);
    //delete the statement 
    rc = sqlite3_finalize(stmt);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL finalize error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    //free the buffer 
    free(sqlbuf);
    //return the usage 
    return usage;
}