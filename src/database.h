/*
 * file: database.h
 * authors: QZ & Carter S. Levinson
 * status: working
 */

#ifndef DATABASE_H 
#define DATABASE_H
#include <stddef.h>
int open_db();
void close_db(); 
int log_write(char *, char *, size_t, size_t,char *, int);
int log_read(char *, char *, size_t);
int getDirSize(char *);
#endif /* DATABASE_H */
