#!/bin/bash

numFilesPerDirectory=$3

if [ "$#" != 3 ]; then
    echo "Wrong number of arguments!"
    exit 5
elif [ ! -f "$1" ]; then
    echo "$1 does not exist or is not a regular file!"
    exit 1
elif [ -e "$2" ]; then
    echo "$2 already exists!"
    exit 2
elif [ ! "$3" -gt 0 ]; then
    echo "Give a positive number for numFilesPerDirectory!"
    exit 3
fi


mkdir "$2"

listCountries=()
listDirectories=()
count=0 #Count countries

{
read line
finishedReading=$? #Get return value of read
while [ "${finishedReading}" == 0 ]; do #While read returns 0
    arrayTemp=( ${line} ) #Split the line to an array
    
    if [ ! -e "$2/${arrayTemp[3]}" ]; then #If this country is found for first time
        listCountries+=("${arrayTemp[3]}") #Add it to listCountries
        listDirectories+=($2/${arrayTemp[3]}) #and to listDirectories
        mkdir "${listDirectories[${count}]}" #Create the respective directory
        for ((j=1;j<="${numFilesPerDirectory}";j++)); do #and the respective files
            touch "${listDirectories[${count}]}/${listCountries[${count}]}-$j.txt"
        done
        count=$(( ${count}+1 ))
    fi
    
    read line #Read next line
    finishedReading=$?
done
}< "$1"  #Read from the inputFile


listLinesForFiles=() #Create an array of strings. Each string will contain all the lines that we should put
for ((i=0;i<"${numFilesPerDirectory}";i++)); do #in the respective file of the current country
    listLinesForFiles+=("")
done

nextFile=0 #Keep the next file(number) for the current country in which we should add a line
touch "MyTempFile1.txt" #Create a temporary file that will store the lines for the current country

for ((i=0;i<"${count}";i++)); do #For each country
    nextFile=0
    curD="${listDirectories[${i}]}/${listCountries[${i}]}-" #Current directory
    grep "${listCountries[$i]}" $1 > "MyTempFile1.txt" #Find all the lines of this country and put
    {                                                  #them in the temp file
        read line #We will read all lines for the current country
        finishedReading=$?
        line+="\n" #Put the missing newline after each line
        while [ "${finishedReading}" == 0 ]; do #For every line for this country
            listLinesForFiles[${nextFile}]+="${line}" #Append the next line in the next element(string) in
                                                      #the array "listLinesForFiles"
            nextFile=$(( ( ${nextFile}+1) % ${numFilesPerDirectory}  )) #Move to the next string of the array
            read line #Read next line
            finishedReading=$?
            line+="\n"
        done
    } < "MyTempFile1.txt" #Read the lines of the country from the temporary file
    
    for ((j=0;j<"${numFilesPerDirectory}";j++)); do #Element(string) j of the "listLinesForFiles" contains
        nextJ=$((j+1))   #all the lines that we should put to the file j+1 of this country. So, put string j
        echo -n -e "${listLinesForFiles[$j]}" >> "${curD}${nextJ}.txt" #in the respective file
        listLinesForFiles[$j]="" #Initialize strings for the next country
    done
done

rm -f "MyTempFile1.txt" #Delete temporary file
