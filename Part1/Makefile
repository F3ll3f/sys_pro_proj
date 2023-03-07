obj= helpfulStringFunctions.o OtherLists/countryList.o Date_citizen_country/date_citizen_country.o OtherLists/statList.o BloomFilter/bloomfilter.o SkipList/skiplist.o OtherLists/virusList.o vaccapp.o vaccineMonitor.o
IFlags= -I BloomFilter -I SkipList -I OtherLists -I Date_citizen_country
Flags=-Wall $(IFlags)
CC=g++
Programs=vaccineMonitor

#Link
all: $(obj)
	$(CC) $(Flags) $(obj) -o $(Programs)

#Compile seperately each source file

helpfulStringFunctions.o: helpfulStringFunctions.cpp
	$(CC) $(Flags) -c helpfulStringFunctions.cpp

OtherLists/countryList.o: ./OtherLists/countryList.cpp
	$(CC) $(Flags) -c ./OtherLists/countryList.cpp -o $@

Date_citizen_country/date_citizen_country.o: Date_citizen_country/date_citizen_country.cpp
	$(CC) $(Flags) -c Date_citizen_country/date_citizen_country.cpp -o $@

OtherLists/statList.o: OtherLists/statList.cpp
	$(CC) $(Flags) -c OtherLists/statList.cpp -o $@

BloomFilter/bloomfilter.o: BloomFilter/bloomfilter.cpp
	$(CC) $(Flags) -c BloomFilter/bloomfilter.cpp -o $@

SkipList/skiplist.o: SkipList/skiplist.cpp
	$(CC) $(Flags) -c SkipList/skiplist.cpp -o $@

OtherLists/virusList.o: OtherLists/virusList.cpp
	$(CC) $(Flags) -c OtherLists/virusList.cpp -o $@

vaccapp.o: vaccapp.cpp
	$(CC) $(Flags) -c vaccapp.cpp

vaccineMonitor.o: vaccineMonitor.cpp
	$(CC) $(Flags) -c vaccineMonitor.cpp

#Clean, compile, link, clean objective files and run
run: clean all clean_obj
	./$(Programs) -c inputFile -b 100000
	
#Clean objective files and executables
clean : 
	rm -f $(obj) $(Programs)

#Clean objective files
clean_obj:
	rm -f $(obj)

