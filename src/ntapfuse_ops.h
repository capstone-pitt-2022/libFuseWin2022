/**
 * Project: ntapfuse
 * Author: Samuel Kenney <samuel.kenney48@gmail.com>
 *         August Sodora III <augsod@gmail.com>
 * File: ntapfuse_ops.h
 * License: GPLv3
 *
 * ntapfuse is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ntapfuse is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ntapfuse. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NTAPFUSE_H
#define NTAPFUSE_H

#define FUSE_USE_VERSION 26

#define _XOPEN_SOURCE 500

#include <fuse.h>
#include <limits.h>

int ntapfuse_getattr (const char *path, struct stat *buf);
int ntapfuse_readlink (const char *path, char *target, size_t size);
int ntapfuse_mknod (const char *path, mode_t mode, dev_t dev);
int ntapfuse_mkdir (const char *path, mode_t mode);
int ntapfuse_unlink (const char *path);
int ntapfuse_rmdir (const char *path);
int ntapfuse_symlink (const char *path, const char *link);
int ntapfuse_rename (const char *src, const char *dst);
int ntapfuse_link (const char *src, const char *dst);
int ntapfuse_chmod (const char *path, mode_t mode);
int ntapfuse_chown (const char *path, uid_t uid, gid_t gid);
int ntapfuse_truncate (const char *path, off_t off);
int ntapfuse_utime (const char *path, struct utimbuf *buf);
int ntapfuse_open (const char *path, struct fuse_file_info *fi);
int ntapfuse_read (const char *path, char *buf, size_t size, 
                             off_t off, struct fuse_file_info *fi);
int ntapfuse_write (const char *path, const char *buf, size_t size, 
                              off_t off, struct fuse_file_info *fi);
int ntapfuse_statfs (const char *path, struct statvfs *buf);
int ntapfuse_release (const char *path, struct fuse_file_info *fi);
int ntapfuse_fsync (const char *path, int datasync, 
                              struct fuse_file_info *fi);
int ntapfuse_setxattr (const char *path, const char *name, 
                                 const char *value, size_t size, int flags);
int ntapfuse_getxattr (const char *path, const char *name, 
                                 char *value, size_t size);
int ntapfuse_listxattr (const char *path, char *list, size_t size);
int ntapfuse_removexattr (const char *path, const char *name);
int ntapfuse_opendir (const char *path, struct fuse_file_info *fi);
int ntapfuse_readdir (const char *path, void *buf, 
                                fuse_fill_dir_t fill, off_t off, 
                                struct fuse_file_info *fi);
int ntapfuse_releasedir (const char *path, struct fuse_file_info *fi);
int ntapfuse_access (const char *path, int mode);
void *ntapfuse_init (struct fuse_conn_info *conn);

#endif /* NTAPFUSE_H */
