# ntapfuse unit testing version 0.1
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>
from operator import sub
import subprocess
import sqlite3
import os
import getpass
import math

# Note: that there is shell builtin command to interface with every syscall

# Good tests can be thought of as operating in four steps
# 1. Arrange: prepare the environment for our tests
# 2. Act: the singular state changing action we want to test
# 3. Assert: examine the resulting state for expected behaviour
# 4. Cleanup: the test must leave up no trace

OK = 0
QUOTA = 1000000

mountName = "mountPoint"
baseDir = "testbase"
dbName = "log.db"
blockSize = 4096
usage = 0
numLogs = 0
oriDir = os.getcwd()
workDir = oriDir[:-5] if oriDir[-5:] == "/test" else oriDir
mountDir = workDir + "/%s" % mountName

userList = ["user666", "user888", "user999"]

# get current username
oriUser = getpass.getuser()
oriUid = os.getuid()


# setup_test_env mainly create some test users and make install
def setup_test_env():

    os.system("sudo make install")
    print("creating some test users...")

    for user in userList:
        cmd = f'''
        sudo useradd -p $(openssl passwd -1 password) {user}
        sudo adduser {user} sudo
        sudo adduser {user} {oriUser}
        ''' 
        os.system(cmd)


def destroy_test_env():
    print("deleting all test users...")
    for user in userList:
        os.system("sudo userdel -f %s" % user)
    # go back to the test directory
    os.chdir(oriDir)


# will be put at the beginning of each test.
# It create base and mountpoint directories, and give permission to group users


def init_test():

    print("\ncreating basedir and mountpoint...")
    os.chdir("%s" % workDir)
    cmd = f'''
    rm {dbName}
    mkdir {mountName}   
    mkdir {baseDir}
    chmod ugo+rwx {mountName}
    chmod ugo+rwx {baseDir}
    ntapfuse mount {baseDir} {mountName}
    chmod ugo+rwx {dbName}
    ''' 

    os.system(cmd)


# will be put at the end of each test; It removes all the test files and folders


def doneTest():
    print("removing all test files and folders...")
    cmd = f'''
    cd {workDir}
    rm -r {baseDir} -f
    sudo umount {mountName}
    rm -r {mountName} -f
    '''

    os.system(cmd)


def get_uid_from_username(username):
    p = subprocess.run("id -u %s" % username, shell=True, capture_output=True)
    uid = p.stdout.decode("utf-8")
    return str(int(uid))


setup_test_env()


class TestClass:

    def test_mkdir(self):

        init_test()

        testuser1 = oriUser
        testuser2 = userList[1]
        uid1 = get_uid_from_username(testuser1)
        uid2 = get_uid_from_username(testuser2)
        numLogs1 = check_log_db(uid1, "Mkdir")
        numLogs2 = check_log_db(uid2, "Mkdir")
        usage1 = check_quota_db(uid1)
        usage2 = check_quota_db(uid2)

        print("Testing mkdir...")

        cmd = '''
        cd %s
        mkdir folder%s
        cd %s
        '''
        print("creating some empty folders for current user...")
        for i in range(22):
            os.system(cmd % (mountDir, i, workDir))
            numLogs1 += 1
            usage1 += blockSize

        print("switching to another user and create some empty folders")

        cmd = '''
        sudo umount %s
        rmdir %s/*
        sudo runuser %s << EOF
        ntapfuse mount %s %s
        cd %s
        mkdir folder%s
        '''

        for i in range(30, 50):
            os.system(cmd % (mountName, baseDir, testuser2, baseDir, mountName,
                             mountDir, i))
            numLogs2 += 1
            usage2 += blockSize

        usageRes1 = check_quota_db(uid1)
        numLogsRes1 = check_log_db(uid1, "Mkdir")

        usageRes2 = check_quota_db(uid2)
        numLogsRes2 = check_log_db(uid2, "Mkdir")

        print("User1 expecting numbers of logs: %s  result is: %s" %
              (str(numLogs1), str(numLogsRes1)))
        print("User1 expecting user usage: %s  result is: %s" %
              (str(usage1), str(usageRes1)))
        print("User2 expecting numbers of logs: %s  result is: %s" %
              (str(numLogs2), str(numLogsRes2)))
        print("User2 expecting user usage: %s  result is: %s" %
              (str(usage2), str(usageRes2)))

        doneTest()

        assert usageRes1 == usage1 and numLogsRes1 == numLogs1 and usageRes2 == usage2 and numLogsRes2 == numLogs2

    def test_rmdir(self):
        init_test()
        # need to assume mkdir work, otherwise there is no folder to delete
        print("Testing rmdir...")
        testuser1 = oriUser
        testuser2 = userList[1]
        uid1 = get_uid_from_username(testuser1)
        uid2 = get_uid_from_username(testuser2)
        numLogs1 = check_log_db(uid1, "Rmdir")
        numLogs2 = check_log_db(uid2, "Rmdir")
        usage1 = check_quota_db(uid1)
        usage2 = check_quota_db(uid2)

        cmd = '''
        cd %s
        mkdir folder%s
        mkdir folder%s
        rmdir folder%s
        cd %s
        '''

        print("creating some folders and then delete some of them...")
        for i in range(18):
            os.system(cmd % (mountDir, i + 18, i, i, workDir))
            numLogs1 += 1
            usage1 += blockSize

        cmd = '''
        sudo umount %s
        sudo runuser %s << EOF
        ntapfuse mount %s %s
        cd %s
        mkdir folder%s
        mkdir folder%s
        rmdir folder%s
        cd %s
        '''

        print("switch to another user to do the same stuff...")
        for i in range(100, 120):
            os.system(cmd % (mountName, testuser2, baseDir, mountName,
                             mountDir, i + 40, i, i, workDir))
            numLogs2 += 1
            usage2 += blockSize

        # check if usage and logs match

        usageRes1 = check_quota_db(uid1)
        numLogsRes1 = check_log_db(uid1, "Rmdir")

        usageRes2 = check_quota_db(uid2)
        numLogsRes2 = check_log_db(uid2, "Rmdir")

        print("User1 expecting numbers of logs: %s  result is: %s" %
              (str(numLogs1), str(numLogsRes1)))
        print("User1 expecting user usage: %s  result is: %s" %
              (str(usage1), str(usageRes1)))
        print("User2 expecting numbers of logs: %s  result is: %s" %
              (str(numLogs2), str(numLogsRes2)))
        print("User2 expecting user usage: %s  result is: %s" %
              (str(usage2), str(usageRes2)))

        doneTest()

        assert usageRes1 == usage1 and numLogsRes1 == numLogs1 and usageRes2 == usage2 and numLogsRes2 == numLogs2

    def test_write(self):
        init_test()

        testuser1 = oriUser
        uid1 = get_uid_from_username(testuser1)
        numLogs1 = check_log_db(uid1, "Write")
        usage1 = check_quota_db(uid1)

        print("Writing some files as original user")
        with open('mountPoint/test1.txt', 'w') as testfile:
            result = subprocess.run(["echo", 'this is a test'],
                                    stdout=testfile)
            retcode = result.returncode
            if retcode == OK:
                numLogs1 += 1
                usage1 += blockSize

        with open('mountPoint/test2.txt', 'w') as testfile:
            result = subprocess.run(["echo", 'hello world'], stdout=testfile)
            retcode = result.returncode
            if retcode == OK:
                numLogs1 += 1
                usage1 += blockSize

        with open('mountPoint/test3.txt', 'w') as testfile:
            result = subprocess.run(["echo", 'goodbye friend'],
                                    stdout=testfile)
            retcode = result.returncode
            if retcode == OK:
                numLogs1 += 1
                usage1 += blockSize

        
        print("Gathering usage and log results")

        usageRes1 = check_quota_db(uid1)
        numLogsRes1 = check_log_db(uid1, "Write")

        doneTest()
        print("User1 expecting numbers of logs: %s  result is: %s" %
              (str(numLogs1), str(numLogsRes1)))
        print("User1 expecting user usage: %s  result is: %s" %
              (str(usage1), str(usageRes1)))
        assert usageRes1 == usage1 and numLogsRes1 == numLogs1

    def test_unlink(self):
        init_test()

        testuser1 = oriUser
        uid1 = get_uid_from_username(testuser1)
        numLogs1 = check_log_db(uid1, "Unlink")
        usage1 = check_quota_db(uid1)
        cmd = '''
        touch %s 
        '''

        print("Creating some empty files")
        for i in range(0, 8):
            usage1 += blockSize
            os.system(cmd % ("mountPoint/file" + str(i)))

        print("Removing some empty files")
        file = "mountPoint/file%s"
        for i in range(0, 8):
            subprocess.run(['rm', file % str(i)])
            numLogs1 += 1
            usage1 -= blockSize

        print("Gathering usage data")
        usageRes1 = check_quota_db(uid1)
        numLogsRes1 = check_log_db(uid1, "Unlink")

        doneTest()
        print("User1 expecting numbers of logs: %s  result is: %s" %
              (str(numLogs1), str(numLogsRes1)))
        print("User1 expecting user usage: %s  result is: %s" %
              (str(usage1), str(usageRes1)))

        assert usage1 == usageRes1 and numLogs1 == numLogsRes1

    def test_link(self):
        init_test()

        testuser1 = oriUser
        testuser2 = userList[2]
        uid1 = get_uid_from_username(testuser1)
        uid2 = get_uid_from_username(testuser2)
        usage1 = check_quota_db(uid1)
        usage2 = check_quota_db(uid2)

        testFile1 = "test1.txt"
        testFile2 = "test2.txt"
        linkFile1 = "test1link.txt"
        linkFile2 = "test2link.txt"

        testtext = "test truncate ...." * 2000

        cmd = f'''
        sudo umount {mountName}
        sudo runuser {testuser1} << EOF
        ntapfuse mount {baseDir} {mountName}
        cd {mountDir}
        echo {testtext} > {testFile1}
        ln {testFile1} {linkFile1}
        cd {workDir}
        ''' 

        os.system(cmd)
        fsize = math.ceil(len(testtext) / 4096) * 4096
        usage1 += fsize * 2 if len(testtext) > blockSize else blockSize * 2

        cmd = f'''
        sudo umount {mountName}
        sudo runuser {testuser2} << EOF
        ntapfuse mount {baseDir} {mountName}
        cd {mountDir}
        echo {testtext} > {testFile2}
        ln {testFile2} {linkFile2}
        cd {workDir}
        ''' 

        os.system(cmd)

        usage2 += fsize * 2 if len(testtext) > blockSize else blockSize * 2

        logUsage1 = check_quota_db(uid1)
        logUsage2 = check_quota_db(uid2)

        print("User1 expecting user usage: %s  result is: %s" %
              (str(usage1), str(logUsage1)))
        print("User2 expecting user usage: %s  result is: %s" %
              (str(usage2), str(logUsage2)))

        doneTest()

        assert usage1 == logUsage1 and usage2 == logUsage2

    def test_chown(self):
        # only test changing the ownership of top level directory and file
        init_test()

        testuser1 = oriUser
        testuser2 = userList[1]

        uid1 = get_uid_from_username(testuser1)
        uid2 = get_uid_from_username(testuser2)

        cmd = f'''
        whoami
        cd {mountName}
        echo creating test folder and file...
        mkdir testFolder
        echo "test text" > testFile.txt
        echo before chown
        ls -l
        '''

        os.system(cmd)

        usage1 = check_quota_db(uid1)
        usage2 = check_quota_db(uid2)

        filePath = mountDir + "/testFile.txt"
        folderPath = mountDir + "/testFolder"

        fileSize = os.path.getsize(filePath)
        folderSize = os.path.getsize(folderPath)

        fileSize = 4096 if fileSize < 4096 else fileSize
        folderSize = 4096 if folderSize < 4096 else folderSize

        print("switching to root user to execute chown....")
        print("after chown: ")

        cmd = f'''
        sudo umount {mountName}
        sudo runuser root << EOF
        ntapfuse mount {baseDir} {mountName}
        cd {mountDir}
        chown {testuser2} testFile.txt
        chown {testuser2} testFolder
        ls -l
        '''

        os.system(cmd)

        usage1 -= (fileSize + folderSize)
        usage2 += (fileSize + folderSize)

        logUsage1 = check_quota_db(uid1)
        logUsage2 = check_quota_db(uid2)

        print("User1 expecting user usage: %s  result is: %s" %
              (str(usage1), str(logUsage1)))
        print("User2 expecting user usage: %s  result is: %s" %
              (str(usage2), str(logUsage2)))

        doneTest()
        assert usage1 == logUsage1 and usage2 == logUsage2

    def test_truncate(self):
        # truncate with a size that is smaller than original size
        # truncate with a size that is larger than original size
        # truncate with a size that is smaller than block size

        init_test()

        testtext = "test truncate ...." * 2000
        print("writing size is: " + str(len(testtext)))
        testuser1 = userList[1]
        testuser2 = userList[2]

        uid1 = get_uid_from_username(testuser1)
        uid2 = get_uid_from_username(testuser2)

        usage1 = check_quota_db(uid1)
        usage2 = check_quota_db(uid2)

        testfile1 = "test1.txt"
        testfile2 = "test2.txt"
        truncateSize1 = 3000
        truncateSize2 = 10000
        extremeSize = 100000000  # when truncate with this size, no usage change
        os.chdir("%s" % workDir)
        print("mounting and creating test files to truncate...")

        cmd = f'''
        sudo umount {mountName}
        sudo runuser {testuser1} << EOF
        ntapfuse mount {baseDir} {mountName}
        cd {mountDir}
        echo {testtext} > {testfile1}
        truncate -s {truncateSize1} {testfile1}
        truncate -s {extremeSize} {testfile1}
        cd {workDir}
        ''' 
        os.system(cmd)

        orifilesize = len(testtext)
        if len(testtext)%blockSize==0:
            orifilesize = len(testtext)
        else:
            orifilesize = math.ceil(len(testtext)/blockSize)*blockSize

        print("switching to another user to create file and truncate....")

        cmd = f'''
        sudo umount {mountName}
        sudo runuser {testuser2} << EOF
        ntapfuse mount {baseDir} {mountName}
        cd {mountDir}
        echo {testtext} > {testfile2}
        truncate -s {truncateSize2} {testfile2}
        truncate -s {extremeSize} {testfile2}
        cd {workDir}
        sudo umount {mountName}
        sudo runuser {oriUser} << EOF
        ''' 

        os.system(cmd)

        change1 = 0

        if truncateSize1 > orifilesize:
            change1 = 0
        else:
            if truncateSize1 > blockSize:
                change1 = truncateSize1 - orifilesize
            else:
                change1 = blockSize - orifilesize

        change2 = 0

        if truncateSize2 > orifilesize:
            change2 = 0
        else:
            if truncateSize2 > blockSize:
                change2 = truncateSize2 - orifilesize
            else:
                change2 = blockSize - orifilesize

        usage1 = usage1+orifilesize+change1
        usage1 = math.ceil(usage1/blockSize)*blockSize
        usage2 = usage2+orifilesize+change2
        usage2 = math.ceil(usage2/blockSize)*blockSize
        logUsage1 = check_quota_db(uid1)
        logUsage2 = check_quota_db(uid2)

        print("User1 expecting user usage: %s  result is: %s"%(str(usage1),str(logUsage1)))
        print("User2 expecting user usage: %s  result is: %s"%(str(usage2),str(logUsage2)))

        doneTest()

        # for some reason, can't put this outside the testclass, will cause error
        destroy_test_env()

        assert usage1 == logUsage1 and usage2 == logUsage2


def check_log_db(uid, op=None):
    con = sqlite3.connect(dbName)
    cur = con.cursor()
    if op:
        cur.execute(
            "select count(*) from Logs where UID=%s and Operation='%s'" %
            (uid, op))
    else:
        cur.execute("select count(*) from Logs where UID=%s" % uid)
    res = cur.fetchall()
    con.close()
    try:
        return res[0][0]
    except:
        return 0


def check_quota_db(uid):
    con = sqlite3.connect(dbName)
    cur = con.cursor()
    cur.execute("select Usage from Quotas where UID=%s" % uid)
    res = cur.fetchall()
    con.close()
    try:
        return res[0][0]
    except:
        return 0  # return the usage of an user if valid
