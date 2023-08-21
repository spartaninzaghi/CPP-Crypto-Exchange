# Please ignore this file, it is only intended to be used by instructors.
mv .guides/results.tar.gpg /home/codio/workspace/
echo $1 | gpg --batch --yes --quiet --passphrase-fd 0 /home/codio/workspace/results.tar.gpg 
tar -xf /home/codio/workspace/results.tar