#!/usr/bin/env bash
# testing script for ntapfuse
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>

#TODO: use getopt to allow for parsing of cli options for the script
# Call getopt to parse the options
# default=True
# while getopts ":q:v:s" o; do 
#   case "${o}" in 
#     q) 
#       queit=True
#       default=False
#       ;;
#     v)
#       verbose=True
#       default=False
#       ;;
#     s) 
#       singleUser=True
#       ;;
#     *)
#       printf "Incorrec usage please refer to the README\n"
#       ;;
#   esac
# done


# get the name of calling user using whoami before sudo
mainUser=$(whoami)
# array of test usernames
users=("testUser" "userTest" "myUserTest")
# variables related to testing files
testFile="../test/ntapfuse_test.py"
testChown="../test/test_chown.py"
testOutput="../test/test_results.txt"
testDirectory="../test"

printf "Creating FUSE group...\n"
sudo groupadd -f FUSE
printf "adding the original user to group...\n"
sudo usermod -aG sudo ${mainUser}
sudo usermod -aG FUSE ${mainUser}

for u in ${!users[@]}; do 
  printf "Creating test user: %s...\n" ${users[$u]}
  sudo useradd -M ${users[$u]}
  printf "Adding user %s to FUSE group...\n" ${users[$u]}
  sudo usermod -aG FUSE ${users[$u]}
done


# check for test directory as a substring of PWD
# the script should be run in the root folder of the
# git repository, so cd parent if in test dir
if [[ "$PWD" == *"test"* ]]; then
  cd ..
fi

printf "Installing ntapfuse binary to system...\n"
sudo make install > /dev/null
printf "Modifying ntapfuse permissions...\n"
sudo chmod u+s /usr/local/bin/ntapfuse

# create the testing directories
printf "Creating the mount and base directories...\n"
mkdir -p ntapfuse/base_dir
mkdir -p ntapfuse/mount_dir
printf "Modifying ownership and permissions of mount and base dirs...\n"
sudo chmod -R g+rwx ./ntapfuse
sudo chgrp -R FUSE ./ntapfuse
printf "Change directory to test directory..."
cd ntapfuse

printf "Starting ntapfuse...\n"
ntapfuse mount base_dir mount_dir -o allow_other
sudo chmod g+rwx log.db
sudo chgrp -R FUSE log.db

# run the test suites for main user 
printf "Running the test suite for %s...\n" ${mainUser}
pytest ${testFile} | tee -a ${testOutput}
# run the test suite for rest of the test users
for u in ${!users[@]}; do 
   printf "Running the test suite for %s\n" ${users[$u]} | tee -a ${testOutput}
   sudo runuser -u ${users[$u]} -- pytest ${testFile} | tee -a ${testOutput}
done

# test chown using sudo 
printf "Testing chown using sudo...\n"
pytest ${testChown} | tee -a ${testOutput}

printf "Unmounting ntapfuse...\n"
fusermount -u mount_dir

printf "Now cleaning up the test environment...\n"
for u in ${!users[@]}; do 
  printf "Deleting user: %s...\n" ${users[$u]}
  sudo userdel -f ${users[$u]}
done

printf "Deleting the FUSE group...\n"
sudo groupdel -f FUSE
printf "Removing all lingering files from the test directories..\n"
rm -rf ./base_dir/*
rm -rf ./mount_dir/*
printf "Copying and removing the database file...\n"
cp ./log.db ${testDirectory}
rm -f ./log.db
printf "Returning to main directory...\n"
cd ..
printf "Now removing the mount and base directories...\n"
rmdir ./ntapfuse/base_dir
rmdir ./ntapfuse/mount_dir
rmdir ./ntapfuse
printf "The results of the test(s) were written to: %s\n" ${testOutput}


