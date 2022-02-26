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
int log_write(char *, char *, size_t);
int log_read(char *, char *, size_t);
#endif /* DATABASE_H */
