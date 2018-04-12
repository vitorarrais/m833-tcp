#!/bin/sh

echo "Server IP/Hostname:"
read HOSTNAME
cd ./client
make all
./client $HOSTNAME
