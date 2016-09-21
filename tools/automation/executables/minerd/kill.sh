#!/bin/bash

for pid in $(ps -ef | grep "minerd" | awk '{print $2}')
do
  kill -9 $pid
done