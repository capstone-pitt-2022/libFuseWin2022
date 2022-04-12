/*
 * file: database.h
 * authors: Qizhe Wang <qiw68@pitt.edu>
 *         Carter S. Levinson <carter.levinson@pitt.edu>
 * status: working
 */

#ifndef DATABASE_H 
#define DATABASE_H
#include <stddef.h>
int open_db();
void close_db(); 
int log_file_op(char *, char *, size_t, size_t, char*, int );
int getDirSize(char *);
int getUsage();
#endif /* DATABASE_H */
