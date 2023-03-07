#include <iostream>
#include <string>
#include "bloomfilter.h"
#include "virusList.h"

//A class that handles all the operations of the travelMonitor
class AppTravelMonitor{
private:
    int *receiverNamedPipes; //receiverNamedPipes[i] is a fifo from which we will receive messages
                             //from monitor i
    int *senderNamedPipes; //senderNamedPipes[i] is a fifo from which we will send messages
                           //to monitor i
    long *pids; //Monitor i is the monitor which has pid equal to pids[i]
    int numMonitors; //Number of monitors
    char *buf; //the buffer
    unsigned long bufferSize;
    unsigned long sizeOfBloom;
    std::string input_dir;
    int dirs_count; //Number of directories(=countries)
    
    VirusList **BloomFiltersOfMonitors;/* BloomFiltersOfMonitors is an array which stores one pointer to
    a VirusList for each monitor. Each VirusList contains a BloomFilter for each virus that this
    monitor handles. (The pointers to the Skiplists in each VirusList always point to NULL, since we do
    not use/create them in the parent process.)*/
    std::string **CountriesOfMonitors; /* CountriesOfMonitors[i] stores an array of pointers to the
                                        names of the countries that Monitor i handles*/
    StatList tStats; //A data structure which keeps the necessary info from each request that
                     //travelMonitor receives
public:
    static int Mode; //Mode=1: SIGINT,SIGQUIT was caught.
                     //Mode=2: SIGCHLD was caught.
                     //Mode=3: A SIGINT or a SIGQUIT was caught after a SIGCHLD.
                     //Mode=0: No signal was caught.
        
    AppTravelMonitor(int numMonitors,unsigned long bufferSize,unsigned long sizeOfBloom,std::string input_dir);
    ~AppTravelMonitor();
    
    bool StartTravelMonitor();  //Start the main operation of travelMonitor after the initializations
                                //Returns false, if an error occurred and had to exit.
private:
    bool CreateMonitorWithForkAndExec(int Monitor); //Call fork and exec to create a new monitor(process)
    bool ReplaceTerminatedMonitor(int Monitor); //Replace a terminated monitor with a new one
    
    char *ReceiveMessageFromMonitor(unsigned long &mesSize,int Monitor);/* Receives a message from
    "Monitor" from the respective named pipe using the buffer. The merged(original) message is returned
    according to the protocol in the "messageExchange.h" file. The memory for the message is dynamically
    allocated */
    void SendMessageToMonitor(const char *Message,unsigned long size,int Monitor);/* Sends a message to
    "Monitor" through the respective named pipe using the buffer according to the protocol in the
    "messageExchange.h" file. */
    void SendMessageToMonitor(const char *Message,int Monitor);/* Same as the above one for c-string
                                                    messages only(uses strlen()+1 instead of size). */
    bool SendFirstMessageToMonitor(int Monitor); /* For the first message only, which is the bufferSize,
                                                  send bufferSize as c-string*/
    
    void ReceiveAndUpdateBloomFilters(int Monitor); //Receive the BloomFilters from "Monitor" and update
                                                    //the previous ones
    int FindMonitorCountry(std::string country);//Find the Monitor which handles "country".
                                                //Return -1 if not found
    int FindMaxReceiverNamedPipe();//Return the maximum integer of the receiverNamedPipes.
    
    void travelReq(long citizenID,date d,std::string countryFrom, std::string countryTo,
                   std::string vName); //Start /travelRequest operation
    void travelStat(std::string vName,date d1,date d2,std::string country); //Start /travelStats operation
                                                                            //with country
    void travelStat(std::string vName,date d1,date d2); //Start /travelStats operation without country
    void addVaccRecs(std::string country); //Start /addVaccinationRecords operation
    void searchVaccinationStatus(long citizenID); //Start /searchVaccinationStatus operation
    void exitApp(); //Start /exit operation
};
