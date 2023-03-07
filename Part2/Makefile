obj1= HelpfulFunctions/helpfulStringFunctions.o AppTravelMonitor/AppTravelMonitor.o travelMonitor.o MessageExchange/messageExchange.o BloomFilter/bloomfilter.o OtherLists/virusList.o SkipList/skiplist.o Date_citizen/date_citizen.o OtherLists/statList.o OtherLists/namesList.o

obj2= HelpfulFunctions/helpfulStringFunctions.o OtherLists/namesList.o Date_citizen/date_citizen.o OtherLists/statList.o BloomFilter/bloomfilter.o SkipList/skiplist.o OtherLists/virusList.o AppMonitor/AppMonitor.o Monitor.o MessageExchange/messageExchange.o

obj= $(obj1) $(obj2)

IFlags= -I BloomFilter -I SkipList -I OtherLists -I Date_citizen -I HelpfulFunctions -I AppMonitor -I AppTravelMonitor -I MessageExchange -I .
Flags=-Wall $(IFlags)
CC=g++

Program1=travelMonitor 
Program2=Monitor
Programs=$(Program1) $(Program2)

#Create two executables
all: all1 all2 

#Link to create travelMonitor
all1: $(obj1)
	$(CC) $(Flags) $(obj1) -o $(Program1)

#Link to create Monitor
all2: $(obj2)
	$(CC) $(Flags) $(obj2) -o $(Program2)

#Compile seperately each source file

HelpfulFunctions/helpfulStringFunctions.o: HelpfulFunctions/helpfulStringFunctions.cpp
	$(CC) $(Flags) -c HelpfulFunctions/helpfulStringFunctions.cpp -o $@

OtherLists/namesList.o: ./OtherLists/namesList.cpp
	$(CC) $(Flags) -c ./OtherLists/namesList.cpp -o $@

Date_citizen/date_citizen.o: Date_citizen/date_citizen.cpp
	$(CC) $(Flags) -c Date_citizen/date_citizen.cpp -o $@

OtherLists/statList.o: OtherLists/statList.cpp
	$(CC) $(Flags) -c OtherLists/statList.cpp -o $@

BloomFilter/bloomfilter.o: BloomFilter/bloomfilter.cpp
	$(CC) $(Flags) -c BloomFilter/bloomfilter.cpp -o $@

SkipList/skiplist.o: SkipList/skiplist.cpp
	$(CC) $(Flags) -c SkipList/skiplist.cpp -o $@

OtherLists/virusList.o: OtherLists/virusList.cpp
	$(CC) $(Flags) -c OtherLists/virusList.cpp -o $@

MessageExchange/messageExchange.o: MessageExchange/messageExchange.cpp
	$(CC) $(Flags) -c MessageExchange/messageExchange.cpp -o $@

AppTravelMonitor/AppTravelMonitor.o: AppTravelMonitor/AppTravelMonitor.cpp
	$(CC) $(Flags) -c AppTravelMonitor/AppTravelMonitor.cpp -o $@

travelMonitor.o: travelMonitor.cpp
	$(CC) $(Flags) -c travelMonitor.cpp

AppMonitor/AppMonitor.o: AppMonitor/AppMonitor.cpp
	$(CC) $(Flags) -c AppMonitor/AppMonitor.cpp -o $@

Monitor.o: Monitor.cpp
	$(CC) $(Flags) -c Monitor.cpp

#Clean, compile, link, clean objective files and run
run: clean all clean_obj
	./$(Programs) -c inputFile -b 100000
	
#Clean objective files and executables
clean : 
	rm -f $(obj) $(Programs)

#Clean objective files
clean_obj:
	rm -f $(obj)

