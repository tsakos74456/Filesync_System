#!/bin/bash

# Check the flags
if [ "$1" != "-p" ]; then
    echo "Wrong -p flag!"
    exit 1
fi

if [ "$3" != "-c" ]; then
    echo "Wrong -c flag!"
    exit 1
fi

path="$2"
command="$4"

# Check if the command is acceptable
if [ "$command" != "listAll" ] && [ "$command" != "listMonitored" ] && [ "$command" != "listStopped" ] && [ "$command" != "purge" ]; then
    echo "Wrong command!"
    exit 1
fi



if [ -e "$path" ]; 
then
    # when the command is listAll we have to print all the pairs of dir 
    if [ "$command" == "listAll" ]; then
        # maps 
        declare -A timestamp  #for timestamp
        declare -A status     #for status
        declare -A target     #for target
        while IFS= read -r line; do
            if echo "$line" | grep -qE "\[.*\] \[.*\] \[[0-9]+\] \[(FULL|DELETED|MODIFIED|ADDED)\] \[(SUCCESS|PARTIAL|ERROR)\]"; 
            then

                source=$(echo "$line" | awk '{print $3}' | tr -d '[]')
                curr_target=$(echo "$line" | awk '{print $4}' | tr -d '[]')
                
                curr_status=$(echo "$line" | awk '{print $7}' | tr -d '[]')
                curr_timestamp=$(echo "$line" | awk '{print $1" "$2}' |tr -d '[]')
                # no need to check if the timestamp is newer than the existing one since int the manager log file everything is written chronologically
                timestamp["$source"]="$curr_timestamp"
                status["$source"]="$curr_status"
                target["$source"]="$curr_target"
            fi
        done < "$path"

        # print all pairs
        for key in "${!status[@]}"; do
            echo "$key -> ${target[$key]} [Last Sync: ${timestamp[$key]}] [${status[$key]}]"
        done

    # when the command is listMonitored we have to print all the monitored pairs 
    elif [ "$command" == "listMonitored" ]; 
    then 
        # maps
        declare -A timestamp  #for timestamp
        declare -A target     #for target

        while IFS= read -r line; do

            # it is added
            if echo "$line" | grep -q "Added directory:"; 
            then
                source=$(echo "$line" | awk '{print $5}')
                curr_target=$(echo "$line" | awk '{print $7}')
                target["$source"]="$curr_target"
            fi

            # get the info about the source
            if echo "$line" | grep -qE "\[.*\] \[.*\] \[[0-9]+\] \[FULL\] \[(SUCCESS|PARTIAL|ERROR)\]"; 
            then
                
                source=$(echo "$line" | awk '{print $3}' | tr -d '[]')
                if [[ -n "${target["$source"]}" ]] ;
                then
                    curr_timestamp=$(echo "$line" | awk '{print $1" "$2}' |tr -d '[]' )
                    # no need to check if the timestamp is newer than the existing one since int the manager log file everything is written chronologically
                    timestamp["$source"]="$curr_timestamp"
                fi
            fi

            # stopped monitoring remove it
            if echo "$line" | grep -q "Monitoring stopped for"; 
            then
                source=$(echo "$line" | awk '{print $6}')
                unset target["$source"]
                unset timestamp["$source"]
            fi

            # it started syncing again so it is monitored again 
            if echo "$line" | grep -q "Sync completed"; 
            then
                source=$(echo "$line" | awk '{print $5}')
                curr_target=$(echo "$line" | awk '{print $7}')
                target["$source"]="$curr_target"
            fi

        done < "$path"

        # print monitored pairs
        for key in "${!target[@]}"; do
            echo "$key -> ${target[$key]} [Last Sync: ${timestamp[$key]}]"
        done

    # when the command is listStopped we have to print all the stopped pairs
    elif [ "$command" == "listStopped" ]; 
    then 

        # maps
        declare -A timestamp  #for timestamp
        declare -A target     #for target
        declare -A stopped    #for stopped

         while IFS= read -r line; do

            # it is added
            if echo "$line" | grep -q "Added directory:"; 
            then
                source=$(echo "$line" | awk '{print $5}')
                curr_target=$(echo "$line" | awk '{print $7}')
                target["$source"]="$curr_target"
                stopped["$source"]=0
            fi

            # get the info about the source
            if echo "$line" | grep -qE "\[.*\] \[.*\] \[[0-9]+\] \[FULL\] \[(SUCCESS|PARTIAL|ERROR)\]"; 
            then
                
                source=$(echo "$line" | awk '{print $3}' | tr -d '[]')
                if [[ -n "${target["$source"]}" ]] ;
                then
                    curr_timestamp=$(echo "$line" | awk '{print $1" "$2}' |tr -d '[]' )
                    # no need to check if the timestamp is newer than the existing one since int the manager log file everything is written chronologically
                    timestamp["$source"]="$curr_timestamp"
                fi
            fi

            # stopped monitoring 
            if echo "$line" | grep -q "Monitoring stopped for"; 
            then
                source=$(echo "$line" | awk '{print $6}')
                curr_target=${target["$source"]}
                curr_timestamp=${timestamp["$source"]}
                stopped["$source"]=1
            fi

            # it started syncing again so it is monitored again 
            if echo "$line" | grep -q "Sync completed"; 
            then
                source=$(echo "$line" | awk '{print $5}')
                curr_target=$(echo "$line" | awk '{print $7}')
                target["$source"]="$curr_target"
                stopped["$source"]=0
            fi

        done < "$path"
        for key in "${!stopped[@]}"; do
            if [ "${stopped["$key"]}" == "1" ];
            then
                echo "$key -> ${target[$key]} [Last Sync: ${timestamp[$key]}]"
            fi
        done
    
    # if the command delete the path or 
    elif [ "$command" == "purge" ];
    then
        if [ -d "$path" ]; 
        then
            echo Deleting "$path"...
            rm -rf "$path"
            echo Purge complete.
        elif [ -f "$path" ];
        then 
            echo Deleting "$path"...
            rm -f "$path"
            echo Purge complete.
        fi

    fi
    
else
    echo "The path doesn't exist"
fi
