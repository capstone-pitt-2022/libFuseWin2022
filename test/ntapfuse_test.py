# ntapfuse unit testing version 0.1
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>
import os
import pwd
import getpass
import subprocess
import sqlite3
import math

# Note: that there is shell builtin command to interface with every syscall

# Good tests can be thought of as operating in four steps
# 1. Arrange: prepare the environment for our tests
# 2. Act: the singular state changing action we want to test
# 3. Assert: examine the resulting state for expected behaviour
# 4. Cleanup: the test must leave up no trace

# Some Constants
OK = 0
QUOTA = 1000000
MOUNT_DIR = "mount_dir"
BASE_DIR = "base_dir"
DATABASE_NAME = "log.db"
BLOCK_SIZE = 4096

# Some Globals
oriDir = os.getcwd()
mountPath = oriDir + "/" + MOUNT_DIR
basePath = oriDir + "/" + BASE_DIR
workPath = oriDir

# get current username
oriUser = getpass.getuser()
oriUid = os.getuid()


def start_test():
    os.chdir(workPath)


def end_test():
    os.chdir(workPath)
    subprocess.run(["rm", "-r", "-f", f"{basePath}/*"])
    subprocess.run(["rm", "-r", "-f", f"{mountPath}/*"])


def get_uid_from_username(username):
    uid = pwd.getpwnam(username).pw_uid
    return uid


def check_log_tab(uid, op=None):
    con = sqlite3.connect(DATABASE_NAME)
    cur = con.cursor()
    if op is not None:
        cur.execute(
            f"SELECT COUNT(*) FROM Logs WHERE UID={uid} AND Operation='{op}';")
        res = cur.fetchall()
    else:
        cur.execute(f"SELECT COUNT(*) FROM Logs WHERE UID={uid};")
        res = cur.fetchall()
    con.close()
    try:
        return res[0][0]
    except:
        return 0


def check_quota_tab(uid):
    con = sqlite3.connect(DATABASE_NAME)
    cur = con.cursor()
    cur.execute(f"SELECT * FROM Quotas WHERE UID={uid};")
    res = cur.fetchall()
    con.close()
    try:
        # return usage and remaining quotas
        return (res[0][2], res[0][3])
    except:
        # return nothing
        return (0, 0)


class TestClass:

    def test_mkdir(self):
        start_test()
        user = oriUser
        uid = get_uid_from_username(user)
        numLogs = check_log_tab(uid, "Mkdir")
        usage, remaining = check_quota_tab(uid)

        for i in range(22):
            os.chdir(mountPath)
            subprocess.run(["mkdir", f"folder{i}"])
            os.chdir(workPath)
            numLogs += 1
            usage += BLOCK_SIZE

        usageRes, remainingRes = check_quota_tab(uid)
        numLogsRes = check_log_tab(uid, "Mkdir")

        end_test()

        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remainingRes == QUOTA

    def test_rmdir(self):
        start_test()
        user = oriUser
        uid = get_uid_from_username(user)
        numLogs = check_log_tab(uid, "Rmdir")
        usage, remaining = check_quota_tab(uid)

        for i in range(18):
            os.chdir(mountPath)
            subprocess.run(["mkdir", f"folder{i}"])
            subprocess.run(["rmdir", f"folder{i}"])
            numLogs += 1
            usage -= BLOCK_SIZE
            os.chdir(workPath)

        usageRes, remainingRes = check_quota_tab(uid)
        numLogsRes = check_log_tab(uid, "Rmdir")

        end_test()
        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remainingRes == QUOTA

    def test_write(self):
        start_test()

        user = oriUser
        uid = get_uid_from_username(user)
        numLogs = check_log_tab(uid, "Write")
        usage, remaining = check_quota_tab(uid)

        print("Writing some files..")
        for i in range(0, 10):
            os.chdir(mountPath)
            file = mountPath + "/" + f"test{i}"
            with open(file, "w") as testfile:
                subprocess.run(["echo", "this is my test"], stdout=testfile)
                numLogs += 1
                usage += BLOCK_SIZE
            os.chdir(workPath)

        usageRes, remainingRes = check_quota_tab(uid)
        numLogsRes = check_log_tab(uid, "Write")

        end_test()
        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remainingRes == QUOTA

    def test_unlink(self):
        start_test()
        user = oriUser
        uid = get_uid_from_username(user)
        # numLogs = check_log_tab(uid, "Write")
        numLogs = check_log_tab(uid, "Unlink")
        usage, remaining = check_quota_tab(uid)

        for i in range(0, 8):
            os.chdir(mountPath)
            subprocess.run(["touch", f"file{i}"])
            usage += BLOCK_SIZE
            # numLogs += 1
            os.chdir(workPath)

        for i in range(0, 8):
            os.chdir(mountPath)
            subprocess.run(["unlink", f"file{i}"])
            numLogs += 1
            usage -= BLOCK_SIZE
            os.chdir(workPath)

        usageRes, remainingRes = check_quota_tab(uid)
        # numLogsRes = check_log_tab(uid, "Write")
        numLogsRes = check_log_tab(uid, "Unlink")

        end_test()

        assert numLogs == numLogsRes
        assert usage == usageRes
        assert usageRes + remainingRes == QUOTA

    def test_link(self):
        start_test()
        testUser = oriUser
        uid = get_uid_from_username(testUser)
        usage, remaining = check_quota_tab(uid)

        testFile1 = "test1.txt"
        testFile2 = "test2.txt"
        linkFile1 = "test1link.txt"
        linkFile2 = "test2link.txt"

        testText = "test truncate ...." * 2000

        os.chdir(mountPath)
        with open(testFile1, 'w') as tf:
            subprocess.run(['echo', testText], stdout=tf)
        subprocess.run(['ln', testFile1, linkFile1])

        fsize = math.ceil(len(testText) / 4096) * 4096
        usage += fsize * 2 if len(testText) > BLOCK_SIZE else BLOCK_SIZE * 2

        logUsage, logRemaining = check_quota_tab(uid)
        end_test()

        assert usage == logUsage
        assert logUsage + logRemaining == QUOTA

    def test_truncate(self):

        start_test()
        user = oriUser
        uid = get_uid_from_username(user)
        numLogs = check_log_tab(uid, "Truncate")
        usage, remaining = check_quota_tab(uid)

        # truncate with a size that is smaller than original size
        # truncate with a size that is larger than original size
        # truncate with a size that is smaller than block size
        testText = "test truncate ...." * 2000
        testFile1 = "truncateTest1.txt"
        testFile2 = "truncateTest2.txt"
        truncateSize1 = 3000
        truncateSize2 = 10000
        extremeSize = 100000000  # when truncate with this size, no usage change
        oriFileSize = len(testText)

        os.chdir(mountPath)
        with open(testFile1, 'w') as tf:
            subprocess.run(['echo', testText], stdout=tf)
        subprocess.run(['truncate', '-s', f"{truncateSize1}", testFile1])
        numLogs += 1
        subprocess.run(['truncate', '-s', f"{extremeSize}", testFile1])
        numLogs += 1

        with open(testFile2, 'w') as tf:
            subprocess.run(['echo', testText], stdout=tf)
        subprocess.run(['truncate', '-s', f"{truncateSize2}", testFile2])
        numLogs += 1
        subprocess.run(['truncate', '-s', f"{extremeSize}", testFile2])
        numLogs += 1
        change1 = 0

        os.chdir(workPath)
        if truncateSize1 > oriFileSize:
            change1 = 0
        else:
            if truncateSize1 > BLOCK_SIZE:
                change1 = truncateSize1 - oriFileSize
            else:
                change1 = BLOCK_SIZE - oriFileSize

        change2 = 0

        if truncateSize2 > oriFileSize:
            change2 = 0
        else:
            if truncateSize2 > BLOCK_SIZE:
                change2 = truncateSize2 - oriFileSize
            else:
                change2 = BLOCK_SIZE - oriFileSize

        usage += oriFileSize + change1
        usage += oriFileSize + change2

        numLogsRes = check_log_tab(uid, "Truncate")
        logUsage = check_quota_tab(uid)

        end_test()
        assert numLogs == numLogsRes
        assert usage == logUsage
