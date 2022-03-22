# ntapfuse unit testing version 0.1
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>
import unittest
import subprocess
import sqlite3
import regex

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


class TestClass:

    def test_mkdir(self):
        return

    def test_rmdir(self):
        return

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

def check_log_db(self):
    return

def check_quota_db(self):
    return


# need to do more thinking about how this will work
def setup_test_env():
    con = sqlite3.connect('log.db')
    cur = con.cursor()


def destroy_test_env():
    con.close()
