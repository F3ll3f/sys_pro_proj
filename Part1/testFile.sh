#!/bin/bash

GenRandFnLnCAge(){ #Generate Random First and Last names, Country and Age and
                   #add them in the string "lines"
    for ((j=1;j<=2;j++)); do #For first and last name
        Randlength=$(( ( ${RANDOM} % 10 ) + 3 )) #Choose random length between 3-12
        Randletter=$(( ${RANDOM} % 26 )) #Choose random letter
        #Put a capital letter(capitalLETTERS=({A..Z}) is defined later) in "lines"
        lines+="${capitalLETTERS[${Randletter}]}"
        for ((m=1;m<${Randlength};m++)); do
            Randletter=$(( ${RANDOM} % 26 )) #Choose random letter
        #Put a small letter(lowercaseLETTERS=({a..z}) is defined later) in "lines"
            lines+="${lowercaseLETTERS[${Randletter}]}"
        done
        lines+=" "
    done
    Randcountry=$(( ${RANDOM} % ${cnum} )) #Choose random country
    lines+="${cFile[${Randcountry}]} " #from the file
    Randage=$(( 1 + ( (${RANDOM}) % 120 ) )) #Choose random age
    lines+="${Randage} "
}

GenRandVirY_NDate(){ #Generate Random Virus, Year, YES or NO and possibly date and
                     #add them in the string "lines"
    Randvirus=$(( ${RANDOM} % ${vnum} )) #Choose random virus
    lines+="${vFile[${Randvirus}]} " #from the file
    if [ $(( ${RANDOM} % 2 )) == 0 ]; then #Choose randomly between YES and NO
        lines+="YES "
        RandD=$(( ( ${RANDOM} % 30 ) + 1 )) #Random Day
        RandM=$(( ( ${RANDOM} % 12 ) + 1 )) #Random Month
        RandY=$(( ( ${RANDOM} % 122 ) + 1900 )) #Random Year between 1900-2021
        lines+="${RandD}-${RandM}-${RandY}"
    elif [ $(( ${RANDOM} % 10 )) -lt 9 ]; then #Choose if NO has a date(10% has date)
        lines+="NO"
    else
        lines+="NO "
        RandD=$(( ( ${RANDOM} % 30 ) + 1 ))
        RandM=$(( ( ${RANDOM} % 12 ) + 1 ))
        RandY=$(( ( ${RANDOM} % 122 ) + 1900 ))
        lines+="${RandD}-${RandM}-${RandY}"
    fi
}

numlines=$3

if [ "$#" != 4 ]; then
    echo "Wrong number of arguments!"
    exit 5
elif [ ! -f "$1" ]; then
    echo "$1 does not exist or is not a regular file!"
    exit 1
elif [ ! -f "$2" ]; then
    echo "$2 does not exist or is not a regular file!"
    exit 2
elif [ "$4" == 0 ]; then
    if [ "${numlines}" -gt 9999 ]; then
        echo "Cannot produce more than 9999 unique IDs!"
        echo "Only 9999 lines will be produced!"
        numlines=9999
    fi
fi

vFile=(`cat "$1"`)
vnum=${#vFile[@]}
cFile=(`cat "$2"`)
cnum=${#cFile[@]}

rm -f inputFile
touch inputFile

capitalLETTERS=({A..Z}) #Capital letters in an array
lowercaseLETTERS=({a..z}) #Lowercase letters in an array

lines=""

if [ "$4" == 0 ]; then #No duplicates
{
    i=0
    allNUMBERS=({1..9999}) #Put all numbers 1-9999 in an array
    k=9999 #The number of Ids not used yet. The first k numbers in allNUMBERS are unused
    while [ "$i" -lt "${numlines}" ]; do #for all lines
        lines=""
        Randnum=$(( ${RANDOM} % $k )) #Choose a random ID from the array allNUMBERS
        lines+="${allNUMBERS[${Randnum}]} " #and put it in the string "lines"
        i=$(( $i + 1 ))
        k=$(( $k - 1 )) #Now the first k numbers in allNUMBERS are unused
        allNUMBERS[${Randnum}]=${allNUMBERS[$k]} #allNUMBERS[k] is unused, so put it
                                            #in place of the used allNUMBERS[Randnum]
        GenRandFnLnCAge
        GenRandVirY_NDate
        echo "${lines}"
    done
} >> inputFile #redirect in inputFile
else #With duplicates
    numOfDuplicates=0 #Keeps an estimate of number of person duplicates. There is a
                      #small chance that real number might be bigger than the estimate.
    NextIsDuplicate=0 #Keeps track if next line has duplicate ID
    for ((i=0;i<"${numlines}";i++)); do #For all lines
        RandIsDupl=$(( ${RANDOM} % 10 )) #Choose if next line has duplicate ID
        if [ ${RandIsDupl} == 0 ]; then #10% next line is duplicate
            NextIsDuplicate=1
        elif [ $(( ${numOfDuplicates} * 100 )) -lt $(( $i * 5 )) ]; then #Check if
        #number of duplicates until now is at least 5% of current lines. If not
        #force next line to have duplicate Id, so there are always many duplicates in
        #the file. In that way, it will be more useful.
            NextIsDuplicate=1
        else #else next line has not duplicate ID
            NextIsDuplicate=0
        fi
        
        if [ ${NextIsDuplicate} == 0 ]; then #Next line has not duplicate ID (although
        # with a small probability next line can still have duplicate ID. As more lines
        #are created, this probability increases.)
            RandID=$(( ( ${RANDOM} % 9999 ) + 1 )) #Random ID
            lines+="${RandID} "
            
            GenRandFnLnCAge
        else #Next line has duplicate ID
            if [ $i == 0 ]; then #In case this is the first line, just put a random line
                RandID=$(( ( ${RANDOM} % 9999 ) + 1 ))
                lines+="${RandID} "
                
                GenRandFnLnCAge
            else
                n=$(( (${RANDOM} % $i) + 1 )) #Choose a random previous line to duplicate
                RandIsDupl=$(( ${RANDOM} % 10 )) #Choose if the citizen or the ID is a duplicate
                echo -n -e "${lines}" >> inputFile
                lines=""
                if [ ${RandIsDupl} -lt 9 ]; then #90% dupliacte ID,fName,lName,country,age
                    numOfDuplicates=$(( ${numOfDuplicates} + 1 ))
                    #find the random line and add ID,fName,lName,country,age to inputFile
                    head -${n} "inputFile" | tail -1|
                    grep -m 1 -o "^[0-9]\+ \w\+ \w\+ \w\+ [0-9]\+ " | tr -d "\n" >> inputFile
                else #10% dupliacte ID
                    #find the random line and add ID to inputFile
                    head -${n} "inputFile" | tail -1 |
                    grep -m 1 -o "^[0-9]\+ " | tr -d "\n" >> inputFile
                    
                    GenRandFnLnCAge
                fi
            fi
        fi
        GenRandVirY_NDate
        lines+="\n"
    done
    echo -n -e "${lines}" >> inputFile
fi
