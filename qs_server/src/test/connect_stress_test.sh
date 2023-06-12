#!/bin/bash

for i in {1..50}
do
   (nc 127.0.0.1 8080 > /dev/null 2>&1 & echo $! >> pids.txt)
done

sleep 5

# Kill the netcat processes
while read pid; do
    kill $pid
done < pids.txt

# Remove the temporary file
rm pids.txt