/**
 * Project: ntapfuse
 * Author: Samuel Kenney <samuel.kenney48@gmail.com>
 *         August Sodora III <augsod@gmail.com>
 *         Qizhe Wang <qiw68@pitt.edu>
 *         Carter S. Levinson <carter.levinson@pitt.edu>
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
#define QUOTA 1000000
#define TIME_MAX 80

#include "ntapfuse_ops.h"
#include "database.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <sys/xattr.h>
#include <sys/types.h>
#include <sqlite3.h>


/*global variable to track?*/
int newfile=0;


char* getTime()
{
  char *timebuf = NULL;
  time_t now;
  timebuf = malloc(TIME_MAX);
  if (!timebuf) {
      return NULL;
  }
  time(&now);
  strftime(timebuf, TIME_MAX, "%c",localtime(&now)); 
  return timebuf;
}

int getOwnerId(char* path)
{
  struct stat stbuf;
  int ret = stat(path,&stbuf);  
  if(ret<0) {
      return -1;
  }else {
      return stbuf.st_uid;
  }
}

int getNumOfFiles(char* dirPath)
{
    FILE *fp;
    int size;
    char sizeChar[100];
	char res[100];
    	char commandBuf[100];
	char *command = "find %s -type f|wc -l";
    sprintf(commandBuf,command,dirPath);
	fp = popen(commandBuf,"r");
	
	int ret = fscanf(fp,"%s",sizeChar);
    if(ret>=0) {
        size = atoi(sizeChar);
    }else{
        return ret;
    } 
    return size;
}

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


/* The following functions describe FUSE operations. Each operation appends
   the path of the root filesystem to the given path in order to give the
   mirrored path. */

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
  newfile=1;


  return mknod (fpath, mode, dev) ? -errno : 0;
}

int
ntapfuse_mkdir (const char *path, mode_t mode)
{
  char fpath[PATH_MAX];
  fullpath (path, fpath);

  //get the updated usage with the additional directory 
  int newUsage = BLOCK_SIZE + getUsage(getuid());

  //check if it surpasses the quota -- if so do not perform the op and return failure 
  if(newUsage > QUOTA) 
  {
    log_file_op("Mkdir",fpath,BLOCK_SIZE,0, "Failed", -ENOBUFS);
    return -ENOBUFS;
  }
  //perform the op 
  int ret = mkdir (fpath, mode | S_IFDIR);
  //mkdir has failed -- log the op do not increment usage 
  if(ret < 0) 
  {
    log_file_op("Mkdir",fpath,BLOCK_SIZE,0, "Failed", ret);
  }
  //mkdir has succeeded -- log the op and increment usage 
  else 
  {
    log_file_op("Mkdir",fpath,BLOCK_SIZE,BLOCK_SIZE,"Success", 0);
    updateQuotas(getTime(),getuid(),BLOCK_SIZE,0);
  }

  return ret ? -errno : 0;

}

int
ntapfuse_unlink (const char *path)
{
  char fpath[PATH_MAX];
  fullpath (path, fpath);

  int uid = getOwnerId(fpath);  
  //get the size of the src 
  int file_size = getFileSize(fpath);
  //get that in blocks 
  int numBlocks = getNumBlocks(file_size);
  //perform the op 
  int ret = unlink(fpath);
  //unlink has failed -- log the op do not decrement usage 
  if(ret < 0) 
  {
    log_file_op("Unlink",fpath,numBlocks*BLOCK_SIZE,0, "Failed", ret);
  }
  //unlink has succeeded -- log the op and decrement usage 
  else 
  {
    log_file_op("Unlink",fpath,numBlocks*BLOCK_SIZE,numBlocks*BLOCK_SIZE,"Success", 0);
    updateQuotas(getTime(),uid,-numBlocks*BLOCK_SIZE,-1);
  }

  return ret ? -errno : 0;
  
}

int
ntapfuse_rmdir (const char *path)
{
  char fpath[PATH_MAX];
  fullpath (path, fpath);

  int uid = getOwnerId(fpath);
  //perform the op 
  int ret = rmdir(fpath);
  
  //rmdir has failed -- log the op do not increment usage 
  if(ret < 0) 
  {
    log_file_op("Rmdir",fpath,-BLOCK_SIZE,0, "Failed", ret);
  }
  //rmdir has succeeded -- log the op and decrement usage 
  else 
  {
    log_file_op("Rmdir",fpath,-BLOCK_SIZE,-BLOCK_SIZE,"Success", 0);
    updateQuotas(getTime(),uid,-BLOCK_SIZE,0);
  }

  return ret ? -errno : 0;
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

  int uid = getOwnerId(fsrc);
  //get the size of the src 
  int file_size = getFileSize(fsrc);
  //get that in blocks 
  int numBlocks = getNumBlocks(file_size);
  //get the new usage 
  int newUsage = getUsage(getuid()) + (numBlocks * BLOCK_SIZE);
  //see if this surpasses the quota 
  if(newUsage>QUOTA)
  {
    log_file_op("Link",fsrc,numBlocks*BLOCK_SIZE,0, "Failed", -ENOBUFS);
    return -ENOBUFS;
  }
  char fdst[PATH_MAX];
  fullpath (dst, fdst);
  //quota not surpassed, perform the op 
  int ret = link(fsrc, fdst);
  //link has failed -- log the op do not increment usage 
  if(ret < 0) 
  {
    log_file_op("Link",fsrc,numBlocks*BLOCK_SIZE,0, "Failed", ret);
  }
  //link has succeeded -- log the op and increment usage 
  else 
  {
    log_file_op("Link",fsrc,numBlocks*BLOCK_SIZE,numBlocks*BLOCK_SIZE,"Success", 0);
    updateQuotas(getTime(),uid,numBlocks*BLOCK_SIZE,1);
  }

  return ret ? -errno : 0;
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
  fullpath (path, fpath);    
  int res;
  struct stat info;
  int ori_uid;
  int ori_gid;
  size_t size;
  int numFile=0;    
  char* time = getTime();  

  if (stat(fpath, &info) != 0) {
      time = "stat failed";
      perror("stat() error");
  }else {
      // check if it's file or directory
      if(S_ISREG(info.st_mode)) {
          numFile = 1; 
      }else if(S_ISDIR(info.st_mode)) {
          numFile = getNumOfFiles(fpath);
      }
      ori_uid = info.st_uid;
      ori_gid = info.st_gid;
      size = info.st_size;
  }  

  size = size>BLOCK_SIZE?size:BLOCK_SIZE;
  int usage = getUsage(uid);
  
  if(usage+size<QUOTA) {
      res = chown (fpath, uid, gid);
  }else {
      return -1;
  }

  if(res<0) {
      log_file_op("Chown", fpath, size, size, "Failed", -errno);
  }else {
      log_file_op("Chown", fpath, size, size, "Success", 0); 
      updateQuotas(time,ori_uid,size*-1,-numFile);
      updateQuotas(time,uid,size,numFile);
  }
  
  return res < 0 ? -errno : 0;
}

int
ntapfuse_truncate (const char *path, off_t off)
{
  char fpath[PATH_MAX];
  fullpath (path, fpath);  
  int initFileSize=0;
  int usage=0;
  FILE *f;
  int res;  
  // get the initail file size
  f = fopen(fpath, "r");
  if(f){
      fseek(f, 0, SEEK_END); 
      initFileSize = ftell(f);
      fclose(f);
  }else{
      return -1;
  }  
  initFileSize = initFileSize%BLOCK_SIZE!=0? (initFileSize/BLOCK_SIZE + 1)*BLOCK_SIZE : initFileSize; 

  /* compare the initail file size, the truncate size, and the block size to determine the usage change
  when truncate size greater than initial file size, nothing change, if truncate size less than initial file
  size, than check if it's less than the block size then decide the usage */  
  if(off>=initFileSize) {
      usage=0;
  }else {
      usage = off>BLOCK_SIZE? ((off-initFileSize)/BLOCK_SIZE)*BLOCK_SIZE:BLOCK_SIZE-initFileSize;
  }  
  res = truncate (fpath, off);  
  if(res < 0){
      log_file_op("Truncate",fpath,0,0, "Failed", -errno);
  }else {
      updateQuotas(getTime(),getuid(),usage,0);
      log_file_op("Truncate",fpath,usage,usage,"Success", 0);
  }      
  return res < 0 ? -errno : 0;
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
  if (fh < 0)
    return -errno;

  fi->fh = fh;

  return 0;
}

char* addquote(char* str){
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
  //used for testing, delete 
  // printf("WRITE CALLED!\n");
  // printf("USAGE: %d\n",getUsage() );


  char fpath[PATH_MAX];
  int res;
  //user usage 
  int usage=0;
  fullpath (path, fpath);
  //get the file size 
  int file_size = getFileSize(fpath);

  int uid = getOwnerId(fpath);

  //empty file, allocate empty block (and then some if circumstances dictate)
  if(file_size==0)
  {
    //update usage for the initial block 
    usage+=BLOCK_SIZE; 
    //update size 
    int newSize = size - BLOCK_SIZE;
    //check if more blocks need to be allocated 
    if(newSize > 0)
    {
      int numBlocks = getNumBlocks(newSize);
      usage+=(numBlocks*BLOCK_SIZE);
    }
  }
  //file not empty, see if more blocks need to be allocated 
  else
  {
    //get number of blocks in the file 
    int num_blocks = getNumBlocks(file_size);
    //get the remainining space in these blocks 
    int space_remain = (num_blocks*BLOCK_SIZE) - file_size; 
    //reset num blocks 
    num_blocks = 0;
    //does the write require extra blocks?
    if(size > space_remain)//yes 
    {
      //fill up the rest of the remaining block 
      int newSize = size - space_remain; 
      //determine the number of additional blocks required 
      num_blocks = getNumBlocks(newSize);
    }
    //update usage 
    usage+=(num_blocks*BLOCK_SIZE);
  }
  
  //get the updated usage 
  int newUsage = usage + getUsage(getuid()); 
  /*
    If the newUsage exceeds the quota, error returned will 
    be ENOBUFS which = Insufficient resources were available in the system to perform the operation.
    (I assume this is the correct one) - source: https://man7.org/linux/man-pages/man3/errno.3.html
  */
  //check if it surpasses the quota  
  if(newUsage>QUOTA)
  {
    /*
      Write fails because quota is maxed out, do not perform the write operation. 
      Log the failure and return error 
    */
    log_file_op("Write",fpath,size,0, "Failed", -ENOBUFS);
    return -ENOBUFS;
  }

  //perform the write 
  res = pwrite (fi->fh, buf, size, off);

  //write has failed -- log the op do not increment usage 
  if(res < 0) 
  {
    log_file_op("Write",fpath,size,0, "Failed", -errno);
  }
  //write has succeeded -- log the op and increment usage 
  else 
  {
    log_file_op("Write",fpath,size,usage,"Success", 0);
    updateQuotas(getTime(),uid,usage,0);
  }
  printf("RES %d\n",res);


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
  if (dir == NULL)
    return -errno;

  fi->fh = (uint64_t) dir;

  return 0;
}

int
ntapfuse_readdir (const char *path, void *buf, fuse_fill_dir_t fill, off_t off,
	      struct fuse_file_info *fi)
{
  struct dirent *de = NULL;

  while ((de = readdir ((DIR *) fi->fh)) != NULL)
    {
      struct stat st;
      memset (&st, 0, sizeof (struct stat));
      st.st_ino = de->d_ino;
      st.st_mode = de->d_type << 12;

      if (fill (buf, de->d_name, &st, 0))
	break;
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

//return the size of the file given by the path 
int getFileSize(char * path)
{
  //pointer to the file 
  FILE *f;
  //open the file 
  f = fopen(path,"rb");
  //check if it's empty
  if(f==NULL)
  {
    printf("Error opening file\n");
    return -1; 
  }
  //it's not -- get its size 
  fseek(f, 0, SEEK_END); 
  int file_size = ftell(f);
  fseek(f, 0, SEEK_SET); 
  fclose(f);
  return file_size;
}

//get the # of blocks the file is taking up 
int getNumBlocks(int fileSize)
{
  return ceil(((double)fileSize/(double)BLOCK_SIZE)); 
}














