/**
 * Project: ntapfuse
 * Authors: Samuel Kenney <samuel.kenney48@gmail.com>
 *          August Sodora III <augsod@gmail.com>
 *          Qizhe Wang <qiw68@pitt.edu>
 *          Carter S. Levinson <carter.levinson@pitt.edu>
 *          Danny Yu <chy75@pitt.edu>
 * File: ntapfuse_ops.c
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
#define _XOPEN_SOURCE 500
#define BLOCK_SIZE 4096

#include "ntapfuse_ops.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <sys/xattr.h>
#include <sys/types.h>
#include <sqlite3.h>

/* global variable to track */
int newfile = 0;

/**
 * Appends the path of the root filesystem to the given path, returning
 * the result in buf.
 */
void
fullpath (const char *path, char *buf)
{
    char *basedir = (char *) fuse_get_context ()->private_data; 
    strcpy (buf, basedir);
    strcat (buf, path);
}


/** 
 * The following functions describe FUSE operations. Each operation appends
 * the path of the root filesystem to the given path in order to give the
 * mirrored path. 
 */
int
ntapfuse_getattr (const char *path, struct stat *buf)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath); 
    return lstat (fpath, buf) ? -errno : 0;
}

int
ntapfuse_readlink (const char *path, char *target, size_t size)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath); 
    return readlink (fpath, target, size) < 0 ? -errno : 0;
}

int
ntapfuse_mknod (const char *path, mode_t mode, dev_t dev)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);
    newfile = 1;  
    return mknod (fpath, mode, dev) ? -errno : 0;

}

int
ntapfuse_mkdir (const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath); 
    int res;
    res = mkdir (fpath, mode | S_IFDIR);    
    if(res < 0) {
        log_file_op("Mkdir",fpath,0,0, "Failed", -errno);
    }else{
        log_file_op("Mkdir",fpath,0,BLOCK_SIZE,"Success", 0);
    } 
            
    return res < 0 ? -errno : 0;

}

int
ntapfuse_unlink (const char *path)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return unlink (fpath) ? -errno : 0;
}

int
ntapfuse_rmdir (const char *path)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    int res;

    res = rmdir (fpath);

    size_t size = getDirSize(fpath);


    if(res < 0) {
        log_file_op("Rmdir",fpath,0,0, "Failed", -errno);
    }else{
        log_file_op("Rmdir",fpath,0,BLOCK_SIZE,"Success", 0);
    }    

    return res < 0 ? -errno : 0;

  

}

int
ntapfuse_symlink (const char *path, const char *link)
{
    char flink[PATH_MAX];
    fullpath (link, flink);

    return symlink (path, flink) ? -errno : 0;
}

int
ntapfuse_rename (const char *src, const char *dst)
{
    char fsrc[PATH_MAX];
    fullpath (src, fsrc);

    char fdst[PATH_MAX];
    fullpath (dst, fdst);

    return rename (fsrc, fdst) ? -errno : 0;
}

int
ntapfuse_link (const char *src, const char *dst)
{
    char fsrc[PATH_MAX];
    fullpath (src, fsrc);

    char fdst[PATH_MAX];
    fullpath (dst, fdst);

    return link (fsrc, fdst) ? -errno : 0;
}

int
ntapfuse_chmod (const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return chmod (fpath, mode) ? -errno : 0;
}

int
ntapfuse_chown (const char *path, uid_t uid, gid_t gid)
{
    char fpath[PATH_MAX];
    int res;
    int usage = 0;
    fullpath (path, fpath);

    //check file existence
    FILE *f = fopen(fpath);
    if (f == null) { //file does not exist
        log_file_op("Chown", fpath, 0, 0, "Failed", -errno);
        res = -1;
        return res < 0 ? -errno : 0;
    }

    //get file size
    long fileSize = getFileSize (fpath);

    //old user id for quota updating
    int oldUid = getOwnerId (fpath);
    if (!(oldUid >= 0)) { //uid is not valid
        log_file_op("Chown", fpath, fileSize, 0, "Failed", -errno);
        res = -1;
        return res < 0 ? -errno : 0;
    }
    
    //get blocks allocated by file
    int numBlocks = getNumBlocks(fileSize);
    usage += numBlocks * BLOCK_SIZE;

    //op fails due to exceeding user quota
    if (newUsage > QUOTA) {
        log_file_op ("Chown", fpath, fileSize, 0, "Failed", -ENOBUFS);
        return -ENOBUFS;
    }

    //perform chown
    res = chown (fpath, uid, gid);

    if (res < 0) { //failed
        log_file_op ("Chown", fpath, fileSize, 0, "Failed", -errno);
    } else { //success
        log_file_op ("Chown", fpath, fileSize, fileSize, "Success", 0);
        updateQuotas(getTime(), oldUid, -usage, 0);
        updateQuotas(getTime(), uid, usage, 0);
    }

    printf("RES %d\n", res);

    return res < 0 ? -errno : 0;
}

int
ntapfuse_truncate (const char *path, off_t off)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return truncate (fpath, off) ? -errno : 0;
}

int
ntapfuse_utime (const char *path, struct utimbuf *buf)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return utime (fpath, buf) ? -errno : 0;
}

int
ntapfuse_open (const char *path, struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    int fh = open (fpath, fi->flags);
    if (fh < 0) {
        return -errno;
    }


    fi->fh = fh;

    return 0;
}


char* 
addquote(char* str) 
{
    char* newstr = calloc(1,strlen(str)+2);
    *newstr='\'';
    strcat(newstr,str);
    char* t = "\'";
    strcat(newstr,t);
    return newstr;
}

int
ntapfuse_read (const char *path, char *buf, size_t size, off_t off,
	   struct fuse_file_info *fi)
{

    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return pread (fi->fh, buf, size, off) < 0 ? -errno : size;
}

int
ntapfuse_write (const char *path, const char *buf, size_t size, off_t off,
	    struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    int res;
    int initFileSize=0;
    int usage=0;
    FILE *f;

    fullpath (path, fpath);

    if (newfile==1) { /* if file is new */
        usage=size<BLOCK_SIZE?BLOCK_SIZE:size;
        newfile=0;
    } else {  /* if the file already exist */

        f = fopen(fpath, "r");
        fseek(f, 0, SEEK_END); 
        initFileSize = ftell(f);
        fclose(f);

        if(initFileSize<BLOCK_SIZE) {
            usage = initFileSize+size>BLOCK_SIZE?initFileSize+size-BLOCK_SIZE:0;
        }else{
            usage = size;
        }

    }

    res = pwrite (fi->fh, buf, size, off);

    if(res < 0) {
        log_file_op("Write",fpath,size,0, "Failed", -errno);
    }else{
        log_file_op("Write",fpath,size,usage,"Success", 0);
    } 

    return res < 0 ? -errno : size;
}

int
ntapfuse_statfs (const char *path, struct statvfs *buf)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return statvfs (fpath, buf) ? -errno : 0;
}

int
ntapfuse_release (const char *path, struct fuse_file_info *fi)
{
    return close (fi->fh) ? -errno : 0;
}

int
ntapfuse_fsync (const char *path, int datasync, struct fuse_file_info *fi)
{
    if (datasync)
        return fdatasync (fi->fh) ? -errno : 0;
    else
        return fsync (fi->fh) ? -errno : 0;
}

int
ntapfuse_setxattr (const char *path, const char *name, const char *value,
	    size_t size, int flags)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return lsetxattr (fpath, name, value, size, flags) ? -errno : 0;
}

int
ntapfuse_getxattr (const char *path, const char *name, char *value, size_t size)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    ssize_t s = lgetxattr (fpath, name, value, size);
    return s < 0 ? -errno : s;
}

int
ntapfuse_listxattr (const char *path, char *list, size_t size)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return llistxattr (fpath, list, size) < 0 ? -errno : 0;
}

int
ntapfuse_removexattr (const char *path, const char *name)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return lremovexattr (fpath, name) ? -errno : 0;
}

int
ntapfuse_opendir (const char *path, struct fuse_file_info *fi)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath); 
    DIR *dir = opendir (fpath);
    if (dir == NULL) {
        return -errno;
    }
    
    fi->fh = (uint64_t) dir;    
    return 0;
}

int
ntapfuse_readdir (const char *path, void *buf, fuse_fill_dir_t fill, off_t off,
	    struct fuse_file_info *fi)
{
    struct dirent *de = NULL;

    while ((de = readdir ((DIR *) fi->fh)) != NULL) {    
        struct stat st;
        memset (&st, 0, sizeof (struct stat));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;    
        if (fill (buf, de->d_name, &st, 0)) {
            break;
        }
        
    }
    return 0;
}

int
ntapfuse_releasedir (const char *path, struct fuse_file_info *fi)
{
    return closedir ((DIR *) fi->fh) ? -errno : 0;
}

int
ntapfuse_access (const char *path, int mode)
{
    char fpath[PATH_MAX];
    fullpath (path, fpath);

    return access (fpath, mode) ? -errno : 0;
}

void *
ntapfuse_init (struct fuse_conn_info *conn)
{
    return (fuse_get_context())->private_data;
}

