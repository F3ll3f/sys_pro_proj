#include <iostream>
#include <string>
#include "bloomfilter.h"
#include "skiplist.h"
#include "virusList.h"
#include "namesList.h"

void *threadOperation(void *App_Mon); //The function that is called when a new thread is created

//A class that handles all the operations of the monitor
class AppMonitor{
private:
    int Socket; //Socket open for reading/writing
    char *buf; //The buffer
    unsigned long sockBufferSize;
    pthread_t *tids;
    int numThreads;
    unsigned long sizeOfBloom;
    std::string input_dir;
    
    
    std::string *Countries; //The countries that this monitor handles
    NamesList *FileNames;//NamesList[i] stores a list with the file names of the country in
                         //Countries[i] that have already been read
    int NumCountries;

    VirusList *VList;//VList includes 2 SkipLists and 1 Bloomfilter for each virus
    SkipList *CitizenList;//A SkipList with all the citizens
    unsigned long AccReqs;  //Count accepted requests
    unsigned long RejReqs;  //Count rejected requests
public:
    class CyclicBuffer{ //The cyclic buffer with the files
    private:
        std::string *cyclicBuf; //The array with the files
        unsigned long cyclBufferSize;
        unsigned long first; //Index for the first file. Equal to cyclBufferSize, if buffer is empty.
        unsigned long last;  //Index for the last file. Equal to cyclBufferSize, if buffer is empty.
    public:
        CyclicBuffer(unsigned long cyclBufferSize);
        ~CyclicBuffer();
        
        bool IsEmpty();//Return true only if the buffer is is empty
        bool AddFileName(std::string fName); //Add a file in the position after last.
                                             //Return false, if the buffer is full
        std::string PopFileName(); //Remove and return the first file. Return empty string,
                                   //if the buffer is empty
    };
    
    CyclicBuffer cyclBuf; //The cyclic buffer
    bool ExitNow; //When the ExitNow flag is true, all threads must exit
    int PossibleProcessing; //>0 means that a thread may be processing a file
                            //=0 means that no thread is processing files.
    
    AppMonitor(int Socket,char **Arguments,int numArgs);
    ~AppMonitor();
    
    void StartMonitor(); //Start the main operation of the Monitor after the constructor
    void InsertDataFromFile(std::string fileName); //Insert data from a file
private:
    char *ReceiveMessageFromParent(unsigned long &mesSize); /* Receives a message from the socket
    using the buffer. The merged(original) message is returned according to the protocol in
    the "messageExchange.h" file. The memory for the message is dynamically allocated */
    void SendMessageToParent(const char *Message,unsigned long size);/* Sends a message to the
        socket using the buffer according to the protocol in the "messageExchange.h" file. */
    
    void SendBloomFiltersToParent();//Send all the BloomFilters to the parent process
    
    int FindCountryIndex(std::string cName);//Find potition of cName in Countries
                                            //Returns -1 if there is no such country
    citizen *CreateNewCitizen(long ID,std::string fName,std::string lName,
                              std::string cName,int age); //Create a new citizen
                                                //and add him to the CitizenList
    void GetFilesUpdateCyclBufferCondWait(); /* Find all the new files in the dierectories. Then,
    insert them in FileNames and in cyclBuf. Then, cond-wait until all data from files have been
    read from the threads and data structures have been updated. */
    bool insertCitizenRec(std::string citizenID,std::string fName,std::string lName,
                          std::string cName,int age,std::string vName);
    /*Insert record with NO. Returns true if insert was succesful. Else, it returns
     false if citizen's personal details are incosistent with a previous record*/
    void TravelReqAndSend(long citizenID,date d,std::string countryFrom,std::string vName);/*
    A /travelRequest command was received from the parent. Start this operation*/
    void SearchVaccStatusAndSend(long citizenID);/* A /searchVaccinationStatus was received from
                                                  the parent. Start this operation*/
    void ExitApp(); //Set the ExitNow flag to true and terminate the threads
    void CreateFileWithStats(); //Create a log_file with stats
    
    void FreeCloseDestroy();//Free the allocated memory, destroy the mutexes and condition variables and
                            //close the socket
};

