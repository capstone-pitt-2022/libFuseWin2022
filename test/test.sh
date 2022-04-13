#!/usr/bin/env bash
# testing script for ntapfuse
# code by @author Carter S. Levinson <carter.levinson@pitt.edu>

#TODO:use getopt to allow for options for the script

# get the name of calling user using whoami before sudo
mainUser=$(whoami)
# array of test usernames
users=("testUser" "userTest" "myUserTest")
# variables related to testing files
testFile="../test/ntapfuse_test.py"
testOutput="../test/test_results.txt"

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

# create the testing directories
printf "Creating the mount and base directories...\n"
mkdir -p ntapfuse/base_dir
mkdir -p ntapfuse/mount_dir
printf "Modifying ownership and permissions of mount and base dirs...\n"
sudo chmod -R g+rwx ./ntapfuse
sudo chgrp -R FUSE ./ntapfuse
#printf "Change directory to test directory..."
cd ntapfuse


printf "Starting ntapfuse...\n"
ntapfuse mount base_dir mount_dir -o allow_other
sudo chmod g+rwx log.db
sudo chgrp -R FUSE log.db

# sudo chmod -R g+rw mount_dir base_dir
# sudo chgrp -R FUSE mount_dir base_dir
# run the test suites for main user 
printf "Running the test suite for %s...\n" ${mainUser}
pytest ${testFile} | tee -a ${testOutput}

# run the test suite for rest of the test users
for u in ${!users[@]}; do 
   printf "Running the test suite for %s\n" ${users[$u]} | tee -a ${testOutput}
   sudo runuser -u ${users[$u]} -- pytest ${testFile} | tee -a ${testOutput}
   #rm -rf folder*
done

printf "Unmounting ntapfuse...\n"
fusermount -u mount_dir
pkill ntapfuse

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
# printf "Removing the database file \n"
#rm -f ./log.db
# change back to parent directory
cd ..
#printf "Now removing the mount and base directories...\n"
#rmdir ./ntapfuse/base_dir
#rmdir ./ntapfuse/mount_dir
#rmdir ./ntapfuse
printf "The results of the test(s) were written to: %s\n" ${testOutput}


