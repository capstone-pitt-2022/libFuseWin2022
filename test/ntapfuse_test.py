#!/usr/bin/env python3
# ntapfuse unit testing version 0.1
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>

# decompyle3 version 3.8.0
# Python bytecode 3.8.0 (3413)
# Decompiled from: Python 3.8.10 (default, Mar 15 2022, 12:22:08) 
# [GCC 9.4.0]
# Embedded file name: /home/user/source/libFuseWin2022/test/ntapfuse_test.py
# Compiled at: 2022-04-23 21:54:33
# Size of source mod 2**32: 10245 bytes

# Good tests can be thought of as operating in four steps
# 1. Arrange: prepare the environment for our tests
# 2. Act: the singular state changing action we want to test
# 3. Assert: examine the resulting state for expected behaviour
# 4. Cleanup: the test must leave up no trace

import os 
import pwd
import getpass 
import subprocess 
import sqlite3
import math
import random

# cxstant values
QUOTA = 10000000
MOUNT_DIR = 'mount_dir'
BASE_DIR = 'base_dir'
DATABASE_NAME = 'log.db'
BLOCK_SIZE = 4096
GOOD = 'Success'
BAD = 'Failed'
# some variables
# path names
oriDir = os.getcwd()
mountPath = oriDir + '/' + MOUNT_DIR
basePath = oriDir + '/' + BASE_DIR
dbPath = oriDir + '/' + DATABASE_NAME
workPath = oriDir
# user / uid info
oriUser = getpass.getuser()
oriUid = os.getuid()

def get_uid_from_username(username):
    uid = pwd.getpwnam(username).pw_uid
    return uid

def check_connection(con):
    try:
        con.cusor()
        return True
    except Exception as ex:
        return False

def start_test():
    os.chdir(workPath)


def end_test():
    os.chdir(basePath)
    for file in os.listdir(basePath):
        filePath = os.path.join(basePath, file)
        if os.path.isdir(filePath):
            os.rmdir(filePath)
        else:
            os.remove(filePath)
    os.chdir(mountPath)
    for file in os.listdir(mountPath):
        filePath = os.path.join(mountPath, file)
        if os.path.isdir(filePath):
            os.rmdir(filePath)
        else:
            os.remove(filePath)
    # clear_quotas_table()
    os.chdir(workPath)

def check_quota_usage(uid):
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Quotas WHERE UID=?;', (uid,))
    row = cu.fetchone()
    cx.close()
    if row:
        usage = row[3]
        return usage
    return

def check_quota_num_files(uid):
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Quotas WHERE UID=?', (uid,))
    row = cu.fetchone()
    cx.close()
    if row:
        num_files = row[2]
        return num_files
    return

def check_quota_remaining(uid):
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Quotas WHERE UID=?', (uid,))
    row = cu.fetchone()
    cx.close()
    if row:
        remaining = row[4]
        return remaining
    return

def clear_quotas_table():
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute("DELETE FROM Quotas;")
    cx.commit()
    cx.close()
    return

def get_quota_row(uid):
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Quotas WHERE UID=?', (uid,))
    row = cu.fetchone()
    cx.close()
    if row:
        return row 
    return

def check_log_count(uid, oper):
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    args = (uid, oper)
    cu.execute("SELECT COUNT(*) FROM Logs WHERE UID=? AND Operation=?;", args)
    row = cu.fetchone()
    cx.close()
    if row:
        count = row[0]
        return count
    return

def check_last_log_uid():
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Logs ORDER BY Time DESC LIMIT 1;')
    row = cu.fetchone()
    cx.close()
    if row:
        uid = row[1]
        return uid
    return

def check_last_log_op():
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Logs ORDER BY Time DESC LIMIT 1;')
    row = cu.fetchone()
    cx.close()
    if row:
        op = row[2]
        return op
    return

def check_last_log_status():
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Logs ORDER BY Time DESC LIMIT 1;')
    row = cu.fetchone()
    cx.close()
    if row:
        status = row[4]
        return status
    return


def check_last_log_size():
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Logs ORDER BY Time DESC LIMIT 1;')
    row = cu.fetchone()
    cx.close()
    if row:
        size = row[3]
        return size
    return


def get_last_log_row():
    cx = sqlite3.connect(dbPath)
    cu = cx.cursor()
    cu.execute('SELECT * FROM Logs ORDER BY Time DESC LIMIT 1;')
    row = cu.fetchone()
    cx.close()
    if row:
        return row
    return


class TestClass:

    def test_mkdir(self):
        start_test()

        user = oriUser
        uid = get_uid_from_username(user)

        numLogs = check_log_count(uid, 'Mkdir')
        if numLogs is None:
            numLogs = 0
        print(numLogs)
        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0

        for i in range(22):
            os.chdir(mountPath)
            subprocess.run(['mkdir', f"folder{i}"])
            os.chdir(workPath)

            print(check_last_log_status())
            if check_last_log_status() == GOOD:
                numLogs += 1
                usage += BLOCK_SIZE
            else:
                numLogs += 1

        numLogsRes = check_log_count(uid, 'Mkdir')
        if numLogsRes is None:
            numLogsRes = 0
        usageRes = check_quota_usage(uid)
        if usageRes is None:
            usageRes = 0
        remaining = check_quota_remaining(uid)
        if remaining is None:
            remaining = 0

        end_test()

        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remaining == QUOTA

    def test_rmdir(self):
        start_test()

        user = oriUser
        uid = get_uid_from_username(user)

        numLogs = check_log_count(uid, 'Rmdir')
        if numLogs is None:
            numLogs = 0
        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0
            
        for i in range(18):
            os.chdir(mountPath)
            subprocess.run(['mkdir', f"folder{i}"])
            if check_last_log_status() == GOOD:
                usage += BLOCK_SIZE
                subprocess.run(['rmdir', f"folder{i}"])
                if check_last_log_status() == GOOD:
                    numLogs += 1
                    usage -= BLOCK_SIZE
            os.chdir(workPath)

        numLogsRes = check_log_count(uid, 'Rmdir')
        if numLogsRes is None:
            numLogsRes = 0
        usageRes = check_quota_usage(uid)
        if usageRes is None:
            usageRes = 0
        remaining = check_quota_remaining(uid)
        if remaining is None:
            remaining = 0

        end_test()

        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remaining == QUOTA


    # test writes smaller than one block
    def test_write1(self):
        start_test()
        user = oriUser
        uid = get_uid_from_username(user)

        numLogs = check_log_count(uid, 'Write')
        if numLogs is None:
            numLogs = 0
        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0

        for i in range(0, 10):
            os.chdir(mountPath)
            file = mountPath + '/' + f"test{i}"
            with open(file, 'w') as testfile:
                subprocess.run(['echo', 'this is my test'], stdout=testfile)
                if check_last_log_status() == GOOD:
                    numLogs += 1
                    usage += BLOCK_SIZE
                else:
                    numLogs += 1
            os.chdir(workPath)

        numLogsRes = check_log_count(uid, 'Write')
        if numLogsRes is None:
            numLogsRes = 0
        usageRes = check_quota_usage(uid)
        if usageRes is None:
            usageRes = 0
        remaining = check_quota_remaining(uid)
        if remaining is None:
            remaining = 0

        end_test()

        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remaining == QUOTA

    # test writes of various block sizes--use DD?
    def test_write2(self):
        return

    def test_unlink(self):
        start_test()

        user = oriUser
        uid = get_uid_from_username(user)

        numLogs = check_log_count(uid, 'Unlink')
        if numLogs is None:
            numLogs = 0
        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0

        count = 0
        for i in range(0, 8):
            os.chdir(mountPath)
            subprocess.run(['touch', f"file{i}"])
            if check_last_log_status() == GOOD:
                usage += BLOCK_SIZE
                count += 1
            os.chdir(workPath)

        for i in range(0, count):
            os.chdir(mountPath)
            subprocess.run(['unlink', f"file{i}"])
            if check_last_log_status() == GOOD:
                numLogs += 1
                usage -= BLOCK_SIZE
            os.chdir(workPath)

        numLogsRes = check_log_count(uid, 'Unlink')
        if numLogsRes is None:
            numLogsRes = 0
        usageRes = check_quota_usage(uid)
        if usageRes is None:
            usageRes = 0
        remaining = check_quota_remaining(uid)
        if remaining is None:
            remaining = 0

        end_test()
        
        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remaining == QUOTA


    def test_link(self):
        start_test()

        testUser = oriUser
        uid = get_uid_from_username(testUser)

        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0

        testFile1 = 'test1.txt'
        testFile2 = 'test2.txt'
        linkFile1 = 'test1link.txt'
        linkFile2 = 'test2link.txt'
        testText = 'test truncate ....' * 2000

        os.chdir(mountPath)
        with open(testFile1, 'w') as tf:
            subprocess.run(['echo', testText], stdout=tf)

        subprocess.run(['ln', testFile1, linkFile1])
        fsize = math.ceil(len(testText) / 4096) * 4096
        usage += fsize * 2 if len(testText) > BLOCK_SIZE else BLOCK_SIZE * 2
        usageRes = check_quota_usage(uid)
        if usageRes is None:
            usageRes = 0
        remaining = check_quota_remaining(uid)
        if remaining is None:
            remaining = 0

        end_test()

        assert usage == usageRes
        assert usageRes + remaining == QUOTA

    def test_truncate(self):
        start_test()

        user = oriUser
        uid = get_uid_from_username(user)
        numLogs = check_log_count(uid, 'Truncate')
        if numLogs is None:
            numLogs = 0
        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0

        # truncate with a size that is smaller than original size
        # truncate with a size that is larger than original size
        # truncate with a size that is smaller than block size
        testText = 'test truncate ....' * 2000
        testFile1 = 'truncateTest1.txt'
        testFile2 = 'truncateTest2.txt'
        truncateSize1 = 3000
        truncateSize2 = 10000
        # truncate with extreme size will not change size at all
        extremeSize = 100000000
        oriFileSize = len(testText)
        os.chdir(mountPath)
        with open(testFile1, 'w') as tf:
            subprocess.run(['echo', testText], stdout=tf)

        subprocess.run(['truncate', '-s', (f"{truncateSize1}"), testFile1])
        if check_last_log_status() == GOOD:
            numLogs += 1
        subprocess.run(['truncate', '-s', (f"{extremeSize}"), testFile1])
        if check_last_log_status() == GOOD:
            numLogs += 1

        with open(testFile2, 'w') as tf:
            subprocess.run(['echo', testText], stdout=tf)
        subprocess.run(['truncate', '-s', (f"{truncateSize2}"), testFile2])
        if check_last_log_status() == GOOD:
            numLogs += 1
        subprocess.run(['truncate', '-s', (f"{extremeSize}"), testFile2])
        if check_last_log_status() == GOOD:
            numLogs += 1
        os.chdir(workPath)

        change1 = 0
        if truncateSize1 >= oriFileSize:
            change1 = 0
        else:
            change1 = math.ceil(truncateSize1/BLOCK_SIZE)*BLOCK_SIZE - math.ceil(oriFileSize/BLOCK_SIZE)*BLOCK_SIZE

        change2 = 0
        if truncateSize2 > oriFileSize:
            change2 = 0
        else:
            change2 = math.ceil(truncateSize2/BLOCK_SIZE)*BLOCK_SIZE - math.ceil(oriFileSize/BLOCK_SIZE)*BLOCK_SIZE

        usage += math.ceil(oriFileSize/BLOCK_SIZE)*BLOCK_SIZE +change1
        usage += math.ceil(oriFileSize/BLOCK_SIZE)*BLOCK_SIZE +change2

        numLogsRes = check_log_count(uid, 'Truncate')
        if numLogsRes is None:
            numLogsRes = 0
        usageRes = check_quota_usage(uid)
        if usageRes is None:
            usageRes = 0
        remaining = check_quota_remaining(uid)
        if remaining is None:
            remaining = 0

        end_test()

        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usage + remaining == QUOTA



    def test_integrate(self):
        start_test()

        user = oriUser
        uid = get_uid_from_username(user)
        numLogsW = check_log_count(uid, 'Write')
        numLogsM = check_log_count(uid, 'Mkdir')
        numLogsR = check_log_count(uid, 'Rmdir')
        numLogsU = check_log_count(uid, 'Unlink')
        numLogsL = check_log_count(uid, 'Link')
        numLogsT = check_log_count(uid, 'Truncate')
        ops = [numLogsW,numLogsM,numLogsR,numLogsU,numLogsL,numLogsT]
        for op in ops:
            if op is None:
                op = 0

        usage = check_quota_usage(uid)
        if usage is None:
            usage = 0

        textSize = random.randint(1,50000)

        os.chdir(mountPath)
        # create some random size file
        for i in range(100):
            os.system(f"head -c {textSize} /dev/urandom > file{i}.txt ")
            if check_last_log_status() == GOOD:
                numLogsW += 1
                usage += math.ceil(textSize/BLOCK_SIZE)*BLOCK_SIZE
            else:
                numLogsW += 1

        # remove some files
        for i in range(50):
            os.system(f"rm file{i}.txt")
            if check_last_log_status() == GOOD:
                numLogsU += 1
                usage -= math.ceil(textSize/BLOCK_SIZE)*BLOCK_SIZE
            else:
                numLogsU += 1

        # link some files
        for i in range(50,100):
            os.system(f"ln file{i}.txt file{i}link.txt")
            if check_last_log_status() == GOOD:
                numLogsL += 1
                fsize = math.ceil(textSize / BLOCK_SIZE) * BLOCK_SIZE
                usage += fsize if textSize > BLOCK_SIZE else BLOCK_SIZE  
            else:
                numLogsL += 1

        # truncate some file with randome size
        oriFileSize = textSize
        for i in range(50,100):
            truncateSize = random.randint(1000,100000)
            os.system(f"truncate -s {truncateSize} file{i}.txt")

            change = 0
            if truncateSize >= oriFileSize:
                change = 0
            else:
                change = math.ceil(truncateSize/BLOCK_SIZE)*BLOCK_SIZE - math.ceil(oriFileSize/BLOCK_SIZE)*BLOCK_SIZE
         

            if check_last_log_status() == GOOD:
                numLogsT += 1
                usage += change
                # usage =math.ceil(usage/BLOCK_SIZE)*BLOCK_SIZE        
            else:
                numLogsT += 1

        # create some dirs
        for i in range(100):
            os.system(f"mkdir folder{i}")
            if check_last_log_status() == GOOD:
                numLogsM += 1
                usage += BLOCK_SIZE
            else:
                numLogsM += 1

        # remove some dirs
        for i in range(50):
            os.system(f"rmdir folder{i}")
            if check_last_log_status() == GOOD:
                numLogsR += 1
                usage-=BLOCK_SIZE 
            else:
                numLogsR += 1
        

        numLogsMres = check_log_count(uid, 'Mkdir')
        numLogsRres = check_log_count(uid, 'Rmdir')
        numLogsUres = check_log_count(uid, 'Unlink')
        numLogsLres = check_log_count(uid, 'Link')
        numLogsTres = check_log_count(uid, 'Truncate')

        usageRes = check_quota_usage(uid)

        end_test()

        assert numLogsM == numLogsMres
        assert numLogsR == numLogsRres
        assert numLogsU == numLogsUres
        assert numLogsL == numLogsLres
        assert numLogsT == numLogsTres
        assert usage == usageRes

