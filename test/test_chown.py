import os
import subprocess
import getpass

from ntapfuse_test import start_test
from ntapfuse_test import end_test
from ntapfuse_test import get_uid_from_username
from ntapfuse_test import check_quota_usage
from ntapfuse_test import check_quota_remaining
from ntapfuse_test import check_log_count

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
dbPath = oriDir + "/" + DATABASE_NAME
workPath = oriDir

# get current username
oriUser = getpass.getuser()
altUser = "testUser"


def test_chown():
    # only test changing the ownership of top level directory and file
    start_test()
    testUser1 = oriUser
    testUser2 = altUser

    uid1 = get_uid_from_username(testUser1)
    uid2 = get_uid_from_username(testUser2)

    os.chdir(mountPath)
    subprocess.run(['mkdir', 'testFolder'])
    with open("testFile.txt", 'w') as tf:
        subprocess.run(['echo', "test text"], stdout=tf)

    usage1 = check_quota_usage(uid1)
    if usage1 is None:
        usage1 = 0
    usage2 = check_quota_usage(uid2)
    if usage2 is None:
        usage2 = 0

    filePath = os.path.join(mountPath + "/testFile.txt")
    folderPath = os.path.join(mountPath + "/testFolder")

    fileSize = os.path.getsize(filePath)
    folderSize = os.path.getsize(folderPath)

    fileSize = 4096 if fileSize < 4096 else fileSize
    folderSize = 4096 if folderSize < 4096 else folderSize

    subprocess.run(['sudo', 'chown', testUser2, 'testFolder'])
    subprocess.run(['sudo', 'chown', testUser2, 'testFile.txt'])

    usage1 -= (fileSize + folderSize)
    usage2 += (fileSize + folderSize)

    usageRes1 = check_quota_usage(uid1)
    if usageRes1 is None:
        usageRes1 = 0
    usageRes2 = check_quota_usage(uid2)
    if usageRes2 is None:
        usageRes2 = 0

    end_test()
    assert usage1 == usageRes1 and usage2 == usageRes2
