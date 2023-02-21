#!/bin/bash

FILE_LIST=$(ls worker_outputs/*out)

touch tdl_results; rm tdl_results; touch tdl_results;

for tld;do
    echo -n "$tld " >> tdl_results
    count=0
    for file in ${FILE_LIST};do
        while read -r line;do
            site=$(echo "$line" | cut -d " " -f 1)
            number=$(echo "$line" | cut -d " " -f 2)
            if [[ $site == *$tld ]];then
                ((count+=number))
            fi
        done < "${file}"
    done
    echo $count >> tdl_results
done
