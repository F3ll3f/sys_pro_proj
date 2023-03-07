#include <iostream>
#include <sstream>
#include <fstream>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdint.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include "AppTravelMonitor.h"
#include "helpfulStringFunctions.h"
#include "messageExchange.h"
#include "bloomfilter.h"
#include "virusList.h"

#ifndef HOST_NAME_MAX //If HOST_NAME_MAX is not defined in this system, use _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif

using namespace std;


AppTravelMonitor::AppTravelMonitor(int numMonitors,int numThreads,unsigned long sockBufferSize,
                        unsigned long cyclBufferSize,unsigned long sizeOfBloom,string input_dir):
numMonitors(numMonitors),sockBufferSize(sockBufferSize),input_dir(input_dir)
{
    int j,i;
    long pid;
    string name;
    
    Sockets=NULL; //Initialization until sockets,buffer and other structures are created
    pids=NULL;
    buf=NULL;
    BloomFiltersOfMonitors=NULL;
    CountriesOfMonitors=NULL;
    
    srand((unsigned int) time(NULL));//Initialize
    
    dirs_count=0;
    DIR *dir;
    if ((dir=opendir(input_dir.c_str()))==NULL) //Try to open input_dir
    {
        perror("opendir");
        exit(3);
    }
    struct dirent *str_dir;
    while ((str_dir=readdir(dir))!=NULL) //Read files in input_dir
    {
        if ((str_dir->d_name)[0]!='.') //Count not hidden folders in input_dir
            dirs_count++;
    }
    closedir(dir);
    
    if (dirs_count<numMonitors) //If countries are less than the Monitors
    {
        this->numMonitors=dirs_count; //Modify the numMonitors to be equal
        numMonitors=dirs_count;       //to the number of countries
        cout<<"Only "<<dirs_count<<" Monitors will be created, since there are only "<<
        dirs_count<<" countries."<<endl;
    }
   
    char **list=new char *[dirs_count]; //Create an array that stores the names
                                        //of all directories in input_dir

    dir=opendir(input_dir.c_str()); //open input_dir
    i=0;
    while ((str_dir=readdir(dir))!=NULL) //read input_dir
    {
        if ((str_dir->d_name)[0]!='.') //read the names of not hidden folders of input_dir
        {                              //and copy them in list
            list[i]=new char[strlen(str_dir->d_name)+1];
            strcpy(list[i],str_dir->d_name);
            i++;
        }
    }
    closedir(dir);
    
    qsort(list,dirs_count,sizeof(char *),compare_strings); //Sort the directories names alphabetically
    
    CountriesOfMonitors=new string *[numMonitors];
    
    int NumDirsForMonI;
    for (i=0;i<numMonitors;i++)//Find and store the subdirectories that each monitor will handle
    {
        if (i>=(dirs_count%numMonitors))//Calculate number of subdirectories
                                        //for this monitor
            NumDirsForMonI=dirs_count/numMonitors;
        else
            NumDirsForMonI=1+(dirs_count/numMonitors);
        
        //Keep the names of countries of Monitor i in CountriesOfMonitors[i]
        CountriesOfMonitors[i]=new string[NumDirsForMonI];
        for (j=0;j<NumDirsForMonI;j++) //The countries that Monitor i will handle
        {                    //will be chosen by a alphabetical round-robin share
            CountriesOfMonitors[i][j]=list[i+j*numMonitors];//So, Monitor i will handle countries in
        }                                                   //position i+j*numMonitors for all j
    }
    
    for (i=0;i<dirs_count;i++)
    {
        delete[] list[i];
    }
    delete[] list;
    
    pids=new long[numMonitors]; //Create an array to store the pid of each monitor
    
    int cur_port=6001+(rand()%3500); //Get a random number in [6001,9500]
                                  //Use ports: cur_port,cur_port+1,cur_port+2,...,cur_port+numMonitors-1
    for (i=0;i<numMonitors;i++)
    {
        pid=fork();
        if (pid==-1) //Error
        {
            for (i=0; i<numMonitors; i++)
            {
                delete[] CountriesOfMonitors[i];
            }
            delete[] CountriesOfMonitors;
            delete[] pids;
            perror("fork");
            exit(3);
        }
        
        if (pid==0) //Child
        {
            CreateMonitorReplacingChild(i,cur_port,numThreads,cyclBufferSize,sizeOfBloom);
        }
        else //Parent
        {
            pids[i]=pid; //Store the pid of Monitor i
            cur_port++; //Give the next port to the next child
        }
    }
    
    //Only parent process reaches this point
    cur_port-=numMonitors; //Calculate the first port number
    Sockets=new int[numMonitors]; //Create arrays to store the file descriptors
    
    for (i=0;i<numMonitors;i++) //For every monitor i
    {
        Sockets[i]=socket(AF_INET,SOCK_STREAM,0); /*Create the socket that parent will use to read
                                                          messages from monitor i */
        
        if (Sockets[i]==-1) //if failed to create
        {
            perror("socket");
            for (j=0; j<numMonitors; j++)
            {
                delete[] CountriesOfMonitors[j];
            }
            delete[] CountriesOfMonitors;
            delete[] Sockets;
            delete[] pids;
            exit(6);
        }
    }
    
    
    for (i=0;i<numMonitors;i++) //For every monitor i
    {
        ConnectWithMonitor(i,cur_port); //Establish connection with monitor i
        cur_port++;
    }

    buf=new char[sockBufferSize]; //Create the buffer

    
    BloomFiltersOfMonitors=new VirusList *[numMonitors];
    for (i=0;i<numMonitors;i++) //Initialize BloomFiltersOfMonitors array
    {
        BloomFiltersOfMonitors[i]=new VirusList(sizeOfBloom,0,0);
    }
    
    
    fd_set readSockets;
    int countNumM=numMonitors;
    
    //Receive all BloomFilters and "READY" messages. Use select to read from the Monitors which have
    //already sent data. If a monitor has already sent some data, keep reading messages from this
    while (countNumM!=0)//monitor until the "READY" message, since a monitor starts sending messages
    { //when it has initialized everything. As a result, there will be no significant delay between
        FD_ZERO(&readSockets); //messages from this monitor.
        for (i=0;i<numMonitors;i++) //Watch with select all sockets that travelMonitor is reading
        {
            FD_SET(Sockets[i],&readSockets);
        }
        
        int check;
        check=select(1+FindMaxSocket(),&readSockets,NULL,NULL,NULL);
        if (check!=-1)//If a monitor has sent data
        {
            for (i=0;i<numMonitors;i++) //Find which monitors have sent data to the sockets
            {
                if (FD_ISSET(Sockets[i],&readSockets)!=0)
                {   //Monitor i has sent data

                    ReceiveAndUpdateBloomFilters(i); //Read the bloomfilters that it sent
                    char *Message;  //and update the bloomfilters that travelMonitor keeps
                    unsigned long mesSize;
                    while (strcmp((Message=ReceiveMessageFromMonitor(mesSize,i)),"READY"))
                    {   //Ready message confirms that all data has been sent to travelMonitor
                        delete[] Message;   //and Monitor i is ready to operate
                    }
                    delete[] Message;
                    countNumM--;
                }
            }
        }
    }
}

AppTravelMonitor::~AppTravelMonitor()
{
    int i;
    if (buf!=NULL)
        delete[] buf;
    if (BloomFiltersOfMonitors!=NULL)
    {
        for (i=0; i<numMonitors; i++)
        {
            delete BloomFiltersOfMonitors[i];
        }
        delete[] BloomFiltersOfMonitors;
    }
    if (CountriesOfMonitors!=NULL)
    {
        for (i=0; i<numMonitors; i++)
        {
            delete[] CountriesOfMonitors[i];
        }
        delete[] CountriesOfMonitors;
    }
    
    for (i=0; i<numMonitors; i++) //Close sockets
    {
        close(Sockets[i]);
    }
    if (pids!=NULL)
        delete[] pids;
    if (Sockets!=NULL)
        delete[] Sockets;
}


bool AppTravelMonitor::StartTravelMonitor()
{
    bool SignalOperation=false; //When true, indicates that a signal interrupted the program
    
    cout<<"Ready!"<<endl; //Print that travelMonitor is ready for operation
    
    string answer;
    int count;
    istringstream is;
    string command="";
    string cID, vName, d1, d2, countryFrom,countryTo;
    
    while (command!="/exit") //Until an exit command is given
    {//Find command and check if format is correct. Then call corresponding method.
        cout<<"Type command: ";
        cout.flush();
        
        //Watch if stdin has data to be read
        fd_set readMyFd;
        FD_ZERO(&readMyFd);
        FD_SET(0,&readMyFd); //Include stdin

        //Βlock until user has written a command in stdin
        if (select(1,&readMyFd,NULL,NULL,NULL)!=-1)
        {   //If the user wrote in stdin
            getline(cin,answer); //Get the command from the user
            cout<<endl;
            if (cin.good()) //no error
            {
                is.str(answer); //Set answer as the content of "is" stream
                count=count_words(answer); //count the words of the answer
                is.clear(); //Clear flags
                is>>command;
            }
            else //error
            {
                is.clear();
                cin.clear();
                command="";
            }
        }
        else if (errno==EINTR) //If a signal interrupted the program
            SignalOperation=true;
        
        if (command=="/travelRequest") //If user gave a /travelRequest command
        {
            if (count!=6)
                cout<<"Wrong format for command /travelRequest"<<endl;
            else
            {
                is>>cID;
                is>>d1;
                is>>countryFrom;
                is>>countryTo;
                is>>vName;
                
                date date1(d1);
                if (onlyLetNumPlusMostOneDash(vName) && isNum(cID) && !date1.isZero() && onlyLetters(countryFrom) && onlyLetters(countryTo))
                    travelReq(atol(cID.c_str()),date1,countryFrom,countryTo,vName);
                else
                    cout<<"Wrong format for command /travelRequest"<<endl;
            }
        }
        else if (command=="/travelStats") //If user gave a /travelStats command
        {
            if (count<4 || count>5)
                cout<<"Wrong format for command /travelStats"<<endl;
            else
            {
                is>>vName;
                is>>d1;
                is>>d2;
                date date1(d1);
                date date2(d2);

                if (onlyLetNumPlusMostOneDash(vName) && !date1.isZero() && !date2.isZero() && date1.compare(date2)<=0)
                {
                    if (count==4)
                        travelStat(vName,date1,date2);
                    else
                    {
                        is>>countryTo;
                        if (onlyLetters(countryTo))
                            travelStat(vName,date1,date2,countryTo);
                        else
                            cout<<"Wrong format for command /travelStats"<<endl;
                    }
                }
                else
                    cout<<"Wrong format for command /travelStats"<<endl;
            }
        }
        else if (command=="/addVaccinationRecords") //If user gave a /addVaccinationRecords command
        {
            if (count!=2)
                cout<<"Wrong format for command /addVaccinationRecords"<<endl;

            else
            {
                is>>countryFrom;
                if (onlyLetters(countryFrom))
                    addVaccRecs(countryFrom);
                else
                    cout<<"Wrong format for command /addVaccinationRecords"<<endl;
            }
        }
        else if (command=="/searchVaccinationStatus") //If user gave a /searchVaccinationStatus command
        {
            if (count!=2)
                cout<<"Wrong format for command /searchVaccinationStatus"<<endl;
            else
            {
                is>>cID;
                if (isNum(cID))
                {
                    searchVaccinationStatus(atol(cID.c_str()));
                }
                else
                    cout<<"Wrong format for command /searchVaccinationStatus"<<endl;
            }
        }
        else if (command!="/exit") //If neither an /exit nor one of the above commands is detected
        {
            if (!SignalOperation) //and if no signal interrupt
                cout<<"Invalid command!"<<endl; //It means a wrong command was given by the user
            else
                SignalOperation=false;
        }
    }
    
    exitApp(); //Start /exit operation
    
    cout<<"Program ended!"<<endl;
    
    return true;
}

void AppTravelMonitor::CreateMonitorReplacingChild(int Monitor, int port, int numThreads,
                                 unsigned long cyclBufferSize, unsigned long sizeOfBloom)
{
    int j,NumDirsForMonI;
    
    delete[] pids;
    
    if (Monitor>=(dirs_count%numMonitors))//Calculate number of subdirectories
                                    //for this monitor
        NumDirsForMonI=dirs_count/numMonitors;
    else
        NumDirsForMonI=1+(dirs_count/numMonitors);
    
    char **mArgs=new char *[12+NumDirsForMonI]; //Create arguments for exec

    mArgs[0]=new char[strlen("./monitorServer")+1];
    strcpy(mArgs[0],"./monitorServer");
    mArgs[1]=new char[strlen("-p")+1];
    strcpy(mArgs[1],"-p");
    mArgs[2]=myIntToStr(port);
    mArgs[3]=new char[strlen("-t")+1];
    strcpy(mArgs[3],"-t");
    mArgs[4]=myIntToStr(numThreads);
    mArgs[5]=new char[strlen("-b")+1];
    strcpy(mArgs[5],"-b");
    mArgs[6]=myUnLongToStr(sockBufferSize);
    mArgs[7]=new char[strlen("-c")+1];
    strcpy(mArgs[7],"-c");
    mArgs[8]=myUnLongToStr(cyclBufferSize);
    mArgs[9]=new char[strlen("-s")+1];
    strcpy(mArgs[9],"-s");
    mArgs[10]=myUnLongToStr(sizeOfBloom);
    
    int i_d_len=input_dir.length();
    //Put paths in the arguments
    if (input_dir[i_d_len-1]=='/') //Inlude only one slash between filenames
    {//If there is already a slash in input_dir
        for (j=0; j<NumDirsForMonI; j++)
        {//Create path
            mArgs[11+j]=new char[i_d_len+CountriesOfMonitors[Monitor][j].length()+1];
            strcpy(mArgs[11+j],input_dir.c_str());
            strcpy(i_d_len+(mArgs[11+j]),CountriesOfMonitors[Monitor][j].c_str());
        }
    }
    else
    {//If there is no slash in input_dir
        for (j=0; j<NumDirsForMonI; j++)
        {//Create path
            mArgs[11+j]=new char[i_d_len+CountriesOfMonitors[Monitor][j].length()+2];
            strcpy(mArgs[11+j],input_dir.c_str());
            mArgs[11+j][i_d_len]='/'; //Add a slash
            strcpy(i_d_len+1+(mArgs[11+j]),CountriesOfMonitors[Monitor][j].c_str());
        }
    }
    
    mArgs[11+NumDirsForMonI]=NULL;
    
    for (j=0; j<numMonitors; j++)
    {
        delete[] CountriesOfMonitors[j];
    }
    delete[] CountriesOfMonitors;
    
    execv("./monitorServer",mArgs);
    
    //If execv failed
    for (j=0; j<11+NumDirsForMonI; j++)
    {
        delete[] mArgs[j];
    }
    
    delete[] mArgs;
    perror("execv"); //Go here only if execv failed
    exit(2);
    return;
}

void AppTravelMonitor::ConnectWithMonitor(int Monitor,int port)
{
    char MyHostName[HOST_NAME_MAX+1]; //Store here the host name
    struct hostent *LocAddr;
    gethostname(MyHostName,HOST_NAME_MAX+1); //Get the host name
    //Find the IP address
    if ((LocAddr=gethostbyname(MyHostName))==NULL)
    {
        int j;
        for (j=0; j<numMonitors; j++)
        {
            close(Sockets[j]);
            delete[] CountriesOfMonitors[j];
        }
        delete[] CountriesOfMonitors;
        delete[] Sockets;
        delete[] pids;
        cout<<"gethostbyname: Error"<<endl;
        exit(9);
    }
    
    struct sockaddr_in TrMonAddr;
    TrMonAddr.sin_family=AF_INET;
    TrMonAddr.sin_port=htons(port);
    //Copy the bytes of local address
    memcpy(&(TrMonAddr.sin_addr.s_addr),(LocAddr->h_addr_list)[0],LocAddr->h_length);
    
    //Keep trying to connect until the other end starts listening
    while (connect(Sockets[Monitor],(struct sockaddr *) &TrMonAddr,sizeof(TrMonAddr))==-1)
    {
        if (errno!=ECONNREFUSED)//If connection failed, but not due to ECONNREFUSED
        {
            perror("connect");
            int j;
            for (j=0; j<numMonitors; j++)
            {
                close(Sockets[j]);
                delete[] CountriesOfMonitors[j];
            }
            delete[] CountriesOfMonitors;
            delete[] Sockets;
            delete[] pids;
            exit(9);
        }
        else //If the error  was that the server has not started listening yet(ECONNREFUSED)
        {
            usleep(100000); //This is redundant: It just prevents using CPU while waiting to connect
            close(Sockets[Monitor]); //Close the socket
            Sockets[Monitor]=socket(AF_INET,SOCK_STREAM,0); //And create it again in order to connect
        }
    }
    return;
}

char *AppTravelMonitor::ReceiveMessageFromMonitor(unsigned long &mesSize,int Monitor)
{
    char *Message=NULL; //Using the buffer receive a message from "Monitor"
    GetMessage(Sockets[Monitor],buf,sockBufferSize,Message,mesSize);
    return Message;
}

void AppTravelMonitor::SendMessageToMonitor(const char *Message,unsigned long size,int Monitor)
{   //Using the buffer send a message to "Monitor"
    SendMessage(Sockets[Monitor],buf,sockBufferSize,Message,size);
    return;
}

void AppTravelMonitor::SendMessageToMonitor(const char *Message,int Monitor)
{   //Using the buffer send a c-string to "Monitor"
    SendMessageToMonitor(Message,strlen(Message)+1,Monitor);
    return;
}


void AppTravelMonitor::ReceiveAndUpdateBloomFilters(int Monitor)
{
    unsigned long mesSize=0;
    char *Message=ReceiveMessageFromMonitor(mesSize,Monitor);//Receive number of BloomFilters
                                                             //from "Monitor"
    BloomFilter *BF;
    string vName;
    int num_viruses=atoi(Message);//Number of BloomFilters=number of viruses
    delete[] Message;

    while ((num_viruses--)!=0) //Until all BloomFilters are received
    {
        Message=ReceiveMessageFromMonitor(mesSize,Monitor); //Receive virus name
        vName=Message;
        delete[] Message;
        BF=BloomFiltersOfMonitors[Monitor]->GetBloomFilter(vName);
        if (BF==NULL) //If there is no BloomFilter for this virus, create one
        {
            BloomFiltersOfMonitors[Monitor]->InsertVirus(vName);
            BF=BloomFiltersOfMonitors[Monitor]->GetBloomFilter(vName);
        }
        
        //Receive BloomFilter array of this virus
        Message=ReceiveMessageFromMonitor(mesSize,Monitor);
        BF->UpdateBloomFilter((unsigned char *) Message); //Update the BloomFilter with the new one
        delete[] Message;
    }
    
    return;
}

int AppTravelMonitor::FindMonitorCountry(std::string country)
{
    int i,j,NumDirsForMonI;
    for (i=0;i<numMonitors;i++)
    {
        if (i>=(dirs_count%numMonitors))//Calculate number of countries for this monitor
            NumDirsForMonI=dirs_count/numMonitors;
        else
            NumDirsForMonI=1+(dirs_count/numMonitors);
        
        for (j=0;j<NumDirsForMonI;j++) //Search for the country
        {
            if ((CountriesOfMonitors[i][j])==country)//If found
                return i;
        }
    }
    
    return -1; //If not found
}

int AppTravelMonitor::FindMaxSocket()
{
    int i,max=Sockets[0];
    for (i=0;i<numMonitors;i++) //Clalculate the maximum
    {
        if (Sockets[i]>max)
            max=Sockets[i];
    }
    
    return max;
}


void AppTravelMonitor::travelReq(long citizenID,date d,string countryFrom,
                                 string countryTo,string vName)
{
    int Monitor=FindMonitorCountry(countryFrom); //Find which Monitor handles this country
    if (Monitor==-1)//If no monitor handles this country
    {
        cout<<"Wrong format! There is no such country in our records!"<<endl;
        return;
    }

    //Search if there is a BloomFilter for this virus of this Monitor
    BloomFilter *BF=BloomFiltersOfMonitors[Monitor]->GetBloomFilter(vName);
    string emptyStr="";
    string ID=CreateName(emptyStr,citizenID);
    if (BF==NULL || !(BF->Search(ID)) )//If there is no such BloomFilter
    {            //or its data shows that this citizen is not vaccinated
        cout<<"REQUEST REJECTED – YOU ARE NOT VACCINATED"<<endl;
        tStats.AddRequest(vName,countryTo,d,false);//Add request to the StatList
        return;
    }
    //We reach this point only if the BloomFilter answers "maybe"
    
    //Send the request to the Monitor that handles this country
    //(send each argument seperately)
    SendMessageToMonitor("/travelRequest",Monitor);
    SendMessageToMonitor(ID.c_str(),Monitor);
    SendMessageToMonitor((d.getDate()).c_str(),Monitor);
    SendMessageToMonitor(countryFrom.c_str(),Monitor);
    SendMessageToMonitor(countryTo.c_str(),Monitor);
    SendMessageToMonitor(vName.c_str(),Monitor);
    
    unsigned long mSize=0;
    char *Message=ReceiveMessageFromMonitor(mSize,Monitor);
    //Receive answer
    
    if (mSize==3) //If answer is "NO"
    {
        string answer(Message);
        delete[] Message;
        if (answer=="NO")
        {
            cout<<"REQUEST REJECTED – YOU ARE NOT VACCINATED"<<endl;
            tStats.AddRequest(vName,countryTo,d,false);
        }
    }
    else if (mSize>3) //If answer is "YES"
    {
        Message[3]='\0'; //Now first 4 bytes are the c-string "YES"
        string answer=Message; //read it
        if (answer=="YES") //confirm the answer
        {
            string fullDate=Message+4; //Message+4 points to the date as a c-string
            delete[] Message;
            date vacDate(fullDate); //Decode the vaccination date
            
            if (vacDate.compare(d)<=0) //If travel date is after the vaccination
            {
                date sixMbeforeD;
                int year=0,month=0,day=0;
                
                d.getYMD(year,month,day);
                //Find the date which is 6 months before the travel date
                if (month>6)
                {
                    date temp(year,month-6,day);
                    sixMbeforeD=temp;
                }
                else
                {
                    date temp(year-1,month+6,day);
                    sixMbeforeD=temp;
                }
                
                if (vacDate.compare(sixMbeforeD)<=0)//If vaccination date earlier than 6 months before travel date
                {
                    cout<<"REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE"<<endl;
                    tStats.AddRequest(vName,countryTo,d,false); //Add request to the StatList
                }
                else //If vaccination date is between (6 months before the travel date) and (the actual travel date)
                {
                    cout<<"HAPPY TRAVELS"<<endl;
                    tStats.AddRequest(vName,countryTo,d,true); //Add request to the StatList with positive answer
                }
            }
            else
            {
                cout<<"REQUEST REJECTED – YOU ARE NOT VACCINATED"<<endl;
                tStats.AddRequest(vName,countryTo,d,false); //Add request to the StatList
            }
        }
    }
    else //If answer is "X"
    {
        string answer(Message);
        delete[] Message;
        if (answer=="X") //Incosistent data was found
        {
            cout<<"This citizen is not from this country!"<<endl;
        }
    }
    
    return;
}

void AppTravelMonitor::travelStat(std::string vName,date d1,date d2,string country)
{
    unsigned long cAcc=0,cRej=0;
    
    tStats.CountAccAndRej(vName,d1,d2,country,cAcc,cRej);
    
    cout<<"TOTAL TRAVEL REQUESTS "<<cAcc+cRej<<endl;
    cout<<"ACCEPTED "<<cAcc<<endl;
    cout<<"REJECTED "<<cRej<<endl;
    
    return;
}

void AppTravelMonitor::travelStat(std::string vName,date d1,date d2)
{
    unsigned long cAcc=0,cRej=0;
    
    tStats.CountAccAndRej(vName,d1,d2,cAcc,cRej);
    
    cout<<"TOTAL TRAVEL REQUESTS "<<cAcc+cRej<<endl;
    cout<<"ACCEPTED "<<cAcc<<endl;
    cout<<"REJECTED "<<cRej<<endl;
    
    return;
}

void AppTravelMonitor::addVaccRecs(std::string country)
{
    int Monitor=FindMonitorCountry(country);

    if (Monitor!=-1)
    {
        cout<<"Updating... Please Wait..."<<endl;
        SendMessageToMonitor("/addVaccinationRecords",Monitor); //Send a message to the Monitor which handles this country
        ReceiveAndUpdateBloomFilters(Monitor); //Receive and update the bloomfilters
    }
    else
    {
        cout<<"There is no such country in our records!"<<endl;
    }
    
    return;
}

void AppTravelMonitor::searchVaccinationStatus(long citizenID)
{
    int i,Monitor=-1;
    string emptyStr="";
    string command="/searchVaccinationStatus";
    string ID=CreateName(emptyStr,citizenID);
    char *Message;
    unsigned long mSize;
    
    const char *mesCom=command.c_str();
    const char *mesID=ID.c_str();
    for (i=0;i<numMonitors;i++)
    {
        SendMessageToMonitor(mesCom,i);//Send the command and the
        SendMessageToMonitor(mesID,i); //ID to all the monitors
    }
    
    fd_set readMonitors;
    int countNumM=numMonitors;
    
    while (countNumM!=0) //Find a Monitor which handles citizenID. Use select to read
    {                    //the answers from monitors that have first sent data
        FD_ZERO(&readMonitors);
        for (i=0;i<numMonitors;i++) //Watch all the sockets
        {
            FD_SET(Sockets[i],&readMonitors);
        }
        if (select(1+FindMaxSocket(),&readMonitors,NULL,NULL,NULL)!=-1)
        {   //If at least a socket has data to read
            for (i=0;i<numMonitors;i++)//Check which sockets can be read
            {
                if (FD_ISSET(Sockets[i],&readMonitors)!=0)
                {   //When find one, read the answer from Monitor i
                    Message=ReceiveMessageFromMonitor(mSize,i);
                    if  (Message[0]=='Y') //"Y" if it found ID, "N" if not
                        Monitor=i; //Monitor will show a monitor that handles this ID
                    delete[] Message;
                    countNumM--;
                }
            }
        }
    }
    for (i=0;i<numMonitors;i++) //Send a response to every Monitor
    {
        if (i==Monitor)
            SendMessageToMonitor("A",i); //Accept communication
        else
            SendMessageToMonitor("R",i); //Reject communication
    }
    
    if (Monitor==-1) //If no monitor has a record of ID
    {
        cout<<"Citizen not found"<<endl;
        return;
    }
    
    //Establish communication with "Monitor"
    
    Message=ReceiveMessageFromMonitor(mSize,Monitor);//Receive Id,name,country
    string line1=Message; //Receive them as wanted to print them
    delete[] Message;
    Message=ReceiveMessageFromMonitor(mSize,Monitor);//Receive Age
    string line2=Message; //Receive it as wanted to print it
    delete[] Message;
    Message=ReceiveMessageFromMonitor(mSize,Monitor);//Receive number of positive records,
    int num_vac=atoi(Message); //which is the number of vaccinations that this ID has done
    delete[] Message;
    Message=ReceiveMessageFromMonitor(mSize,Monitor);//Receive number of negative answers,
    int num_NONvac=atoi(Message); //which is the number of viruses that Monitor knows that
    delete[] Message;             //this citizen has not been vaccinated for
    
    char **Vacc;
    if (num_vac>0)
    {
        Vacc=new char *[num_vac]; //Store viruses with positive answers
        for (i=0;i<num_vac;i++)
        {   //Receive all positive records
            Vacc[i]=ReceiveMessageFromMonitor(mSize,Monitor);
        }
    }
    
    char **nonVacc;
    if (num_NONvac>0)
    {
        nonVacc=new char *[num_NONvac]; //Store viruses with negative answers
        for (i=0;i<num_NONvac;i++)
        {   //Receive all negative records
            nonVacc[i]=ReceiveMessageFromMonitor(mSize,Monitor);
        }
    }
    
    cout<<line1<<endl;
    cout<<line2<<endl;
    
    for (i=0;i<num_vac;i++)//Print positive records
    {
        cout<<Vacc[i]<<endl;
        delete[] Vacc[i];
    }
    
    for (i=0;i<num_NONvac;i++)//Print negative records
    {
        cout<<nonVacc[i]<<endl;
        delete[] nonVacc[i];
    }
    
    if (num_vac>0)
        delete[] Vacc;
    if (num_NONvac>0)
        delete[] nonVacc;

    return;
}

void AppTravelMonitor::exitApp()
{
    int i;
    
    for (i=0;i<numMonitors;i++)
    {
        SendMessageToMonitor("/exit",i); //Inform the Monitors that they must terminate
    }
    
    for (i=0;i<numMonitors;i++)
    {
        waitpid(pids[i],NULL,0); //Wait for them to terminate
    }
    
    unsigned long cAcc=0,cRej=0;
    string fileName("log_file.");
    char *pidTravelM=myUnLongToStr(getpid());
    fileName.append(pidTravelM); //Create name of log_file
    delete[] pidTravelM;
    ofstream outfile(fileName.c_str(),ofstream::out|ofstream::trunc);
    
    for (i=0;i<dirs_count;i++) //Write all countries that were handled to the log_file
    {
        outfile<<CountriesOfMonitors[i%numMonitors][i/numMonitors]<<endl;
    }
    
    tStats.CountAccAndRej(cAcc,cRej); //Count accepted and rejected requests
    outfile<<"TOTAL TRAVEL REQUESTS "<<cAcc+cRej<<endl;
    outfile<<"ACCEPTED "<<cAcc<<endl;
    outfile<<"REJECTED "<<cRej<<endl;
    return;
}
