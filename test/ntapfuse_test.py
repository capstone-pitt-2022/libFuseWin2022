# ntapfuse unit testing version 0.1
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>
from operator import sub
import unittest
import subprocess
import sqlite3
import regex
import pytest
import os

# Note: that there is shell builtin command to interface with every syscall

# Good tests can be thought of as operating in four steps
# 1. Arrange: prepare the environment for our tests
# 2. Act: the singular state changing action we want to test
# 3. Assert: examine the resulting state for expected behaviour
# 4. Cleanup: the test must leave up no trace

# TODO: set up fixtures for temp environment and think about invocation
# procedures
# TODO: define clear list of what we want to be tested, what the edge cases are
# and ideal behavior
# TODO: integrate with sqlite 3
# TODO: think about how the OS and the subprocess layer affect our testing
# environment and procedure

mountName = "mountPoint"
baseDir = "testbase"
dbName = "log.db"
blockSize = 4096
usage=0
numLogs = 0

# need to do more thinking about how this will work
def setup_test_env():
    print("creating basedir and mountpoint...")
    os.chdir("..")
    cmd = '''
    rmdir %s/*
    rmdir %s
    sudo make install
    sudo umount %s 
    rmdir %s
    mkdir %s   
    mkdir %s
    rm %s
    '''%(baseDir,baseDir,mountName,mountName,mountName,baseDir,dbName)

    os.system(cmd)
    res=subprocess.run("ntapfuse mount %s %s"%(baseDir,mountName),shell=True,check=True,capture_output=True,text=True)
    if res.stderr:
        print("stderr:",res.stderr)




setup_test_env()



class TestClass:

   
    def test_mkdir(self):

        global usage
        global numLogs

        print("Testing mkdir...")

        os.chdir("./%s"%mountName)

        print("creating some empty folders")
      
        for i in range(30):
            res = subprocess.run('mkdir testFolder%s'%str(i),shell=True,check=True,capture_output=True,text=True)
            
            if res.stderr:
                print("stderr:",res.stderr)
            else:
                usage+=blockSize
                numLogs+=1
                

        print("checking if usage and logs match...")
        os.chdir("..")  
        uid=os.getuid()
        usageRes = check_quota_db(self,uid)
        numLogsRes = check_log_db(self,uid,None)

        print("expecting numbers of logs: %s  result is: %s"%(str(numLogs),str(numLogs)))
        print("expecting user usage: %s  result is: %s"%(str(usage),str(usageRes)))


        assert usageRes==usage and numLogsRes==numLogs
        

     
    def test_rmdir(self):
        # need to assume mkdir work
        print("Testing rmdir...")
        global usage
        global numLogs

        os.chdir("./%s"%mountName)

        print("deleting some empty folders that mkdir create")
        for i in range(10):
            res = subprocess.run('rmdir testFolder%s'%str(i),shell=True,check=True,capture_output=True,text=True)
            if res.stderr:
                print("stderr:",res.stderr)
            else:
                usage-=blockSize
                numLogs+=1

        os.chdir("..")

        # mkdir rmdir test done, remove test folders
        cmd = '''
            rmdir %s/*
            rmdir %s
            sudo umount %s
            rm %s/* -f
            rmdir %s
            '''%(baseDir,baseDir,mountName,mountName,mountName)

        # check if usage and logs match
        print("checking if usage and logs match...")
        os.system(cmd)
        uid=os.getuid()
        usageRes = check_quota_db(self,uid)
        numLogsRes = check_log_db(self,uid)

        print("expecting numbers of logs: %s  result is: %s"%(str(numLogs),str(numLogs)))
        print("expecting user usage: %s  result is: %s"%(str(usage),str(usageRes)))

        assert usageRes==usage and numLogsRes==numLogs

    

    def test_write(self):
        return

    def test_unlink(self):
        return

    def test_chown(self):
        return 
    
    def test_truncate(self):
        return 

    def test_read(self):
        return

    def test_utime(self):
        return 

def check_log_db(self,uid,op=None):
    con = sqlite3.connect(dbName)
    uid=os.getuid()
    cur = con.cursor()
    if op:
        cur.execute("select count(*) from Logs where UID=%s and Operation=%s"%(uid,op))
    else:
        cur.execute("select count(*) from Logs where UID=%s"%uid)
    res = cur.fetchall()
    con.close()
    return res[0][0]
    

def check_quota_db(self,uid):
    con = sqlite3.connect(dbName)
    uid=os.getuid()
    cur = con.cursor()
    cur.execute("select Usage from Quotas where UID=%s"%uid)
    res = cur.fetchall()
    con.close()
    return res[0][0]  # return the usage of an user if valid


# May use it for destroying the testing env
def destroy_test_env():

    pass

