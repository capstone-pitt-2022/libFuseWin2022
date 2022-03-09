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
int log_file_op(char *, char *, size_t, size_t, char*, int );
int getDirSize(char *);
#endif /* DATABASE_H */
