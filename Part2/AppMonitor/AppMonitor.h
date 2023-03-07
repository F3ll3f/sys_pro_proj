#include <iostream>
#include <string>
#include "bloomfilter.h"
#include "skiplist.h"
#include "virusList.h"
#include "namesList.h"

//A class that handles all the operations of the monitor
class AppMonitor{
private:
    int monitorReceiverNamedPipe; //Fifo open for reading
    int monitorSenderNamedPipe; //Fifo open for writing
    char *buf; //The buffer
    unsigned long bufferSize;
    unsigned long sizeOfBloom;
    std::string input_dir;
    
    std::string *Countries; //The countries that this monitor handles
    NamesList *FileNames;//NamesList[i] stores a list with the file names for
                         //the country Countries[i]
    int NumCountries;

    VirusList *VList;//VList includes 2 SkipLists and 1 Bloomfilter for each virus
    SkipList *CitizenList;//A SkipList with all the citizens
    unsigned long AccReqs;  //Count accepted requests
    unsigned long RejReqs;  //Count rejected requests
public:
    static int Mode;//Mode=1: SIGINT,SIGQUIT was caught.
                    //Mode=2: SIGUSR1 was caught.
                    //Mode=3: Both SIGUSR1 and (SIGINT or SIGQUIT) were caught. SIGUSR1 operation
                    //        will be executed first.
                    //Mode=0: No signal was caught.
    AppMonitor(int monitorReceiverNamedPipe,int monitorSenderNamedPipe);
    ~AppMonitor();
    
    void StartMonitor(); //Start the main operation of the Monitor after the constructor
private:
    char *ReceiveMessageFromParent(unsigned long &mesSize); /* Receives a message from the named
    pipe using the buffer. The merged(original) message is returned according to the protocol in
    the "messageExchange.h" file. The memory for the message is dynamically allocated */
    void SendMessageToParent(const char *Message,unsigned long size);/* Sends a message to the
        named pipe using the buffer according to the protocol in the "messageExchange.h" file. */
    
    void SendBloomFiltersToParent();//Send all the BloomFilters to the parent process
    
    int FindCountryIndex(std::string cName);//Find potition of cName in Countries
                                            //Returns -1 if there is no such country
    citizen *CreateNewCitizen(long ID,std::string fName,std::string lName,
                              std::string cName,int age); //Create a new citizen
                                                //and add him to the CitizenList
    void UpdateData(); //Read all the new files and update the data structures
    void InsertDataFromFile(std::string cName,std::string fileName); //Insert data from a file
    bool insertCitizenRec(std::string citizenID,std::string fName,std::string lName,
                          std::string cName,int age,std::string vName);
    /*Insert record with NO. Returns true if insert was succesful. Else, it returns
     false if citizen's personal details are incosistent with a previous record*/
    void TravelReqAndSend(long citizenID,date d,std::string countryFrom,std::string vName);/*
    A /travelRequest command was received from the parent. Start this operation*/
    void SearchVaccStatusAndSend(long citizenID);/* A /searchVaccinationStatus was received from
                                                  the parent. Start this operation*/
    void CreateFileWithStats(); //Create a log_file with stats
};


