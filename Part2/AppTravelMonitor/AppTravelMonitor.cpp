#include <iostream>
#include <sstream>
#include <fstream>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include "AppTravelMonitor.h"
#include "helpfulStringFunctions.h"
#include "messageExchange.h"
#include "bloomfilter.h"
#include "virusList.h"

using namespace std;


void catchAndHandleSignals(int sigNo); //Signal handler

int AppTravelMonitor::Mode=0; //Initialize Mode

AppTravelMonitor::AppTravelMonitor(int numMonitors,unsigned long bufferSize,unsigned long sizeOfBloom,string input_dir):
numMonitors(numMonitors),bufferSize(bufferSize),sizeOfBloom(sizeOfBloom),input_dir(input_dir)
{
    int j,i;
    long pid;
    string name,name1,name2;
    
    receiverNamedPipes=NULL; //Initialization until named pipes,buffer
    senderNamedPipes=NULL;   //and other structures are created
    pids=NULL;
    buf=NULL;
    BloomFiltersOfMonitors=NULL;
    CountriesOfMonitors=NULL;
    
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
    
    for (i=0;i<2*numMonitors;i++) //Create two named pipes for each Monitor
    {
        if (i<numMonitors) //One to receive messages
            name=CreateName("rec",i);
        else //and one to send messages
            name=CreateName("send",i-numMonitors);
        
        if (mkfifo(name.c_str(),0666)==-1) //try to create the named_pipes
        {
            if (errno!=EEXIST) //If a named pipe with this name already exists, use that one
            {
                perror("mkfifo");
                exit(2);
            }
        }
    }
    
    pids=new long[numMonitors]; //Create an array to store the pid of each monitor
    for (i=0;i<numMonitors;i++)
    {
        pid=fork();
        if (pid==-1) //Error
        {
            delete[] pids;
            perror("fork");
            exit(3);
        }
        
        if (pid==0) //Child
        {
            delete[] pids;
            name1=CreateName("rec",i);
            name2=CreateName("send",i);
            execl("./Monitor","./Monitor",name1.c_str(),name2.c_str(),NULL);
            perror("execl"); //Go here only if execl failed
            exit(2);
        }
        else //Parent
        {
            pids[i]=pid; //Store the pid of Monitor i
        }
    }
    
    //Only parent process reaches this point

    srand((unsigned int) time(NULL));//Initialize in order to use it in other methods
    
    receiverNamedPipes=new int[numMonitors]; //Create arrays to store the file descriptors
    senderNamedPipes=new int[numMonitors];   //of the named pipes

    for (i=0;i<numMonitors;i++) //For every monitor i
    {
        name=CreateName("rec",i);
        receiverNamedPipes[i]=open(name.c_str(),O_RDWR); /*open the fifo that parent will use to read
        messages from monitor i (O_RDWR instead of O_RDONLY is used just in order to keep a write end
                                 always open, when a child is killed) */
        
        if (receiverNamedPipes[i]==-1) //if failed to open
        {
            perror("open");
            delete[] receiverNamedPipes;
            delete[] senderNamedPipes;
            delete[] pids;
            exit(6);
        }
    }
    for (i=0;i<numMonitors;i++) //For every monitor i
    {
        name=CreateName("send",i);
        senderNamedPipes[i]=open(name.c_str(),O_WRONLY); //open the fifo that parent will use to write
                                                         //messages to monitor i
        if (senderNamedPipes[i]==-1) //if failed to open
        {
            perror("open");
            delete[] receiverNamedPipes;
            delete[] senderNamedPipes;
            delete[] pids;
            exit(6);
        }
    }
    
    BloomFiltersOfMonitors=new VirusList *[numMonitors];
    for (i=0;i<numMonitors;i++) //Initialize BloomFiltersOfMonitors array
    {
        BloomFiltersOfMonitors[i]=new VirusList(sizeOfBloom,0,0);
    }
    

    buf=new char[bufferSize]; //Create the buffer
    
    //For the first message only which is the buffer size, use a different protocol.
    //Specifically, send the bufferSize as a c-string
    for (i=0;i<numMonitors;i++)//Send buffer size string to the monitors
    {
        if (!SendFirstMessageToMonitor(i))
            exit(7);
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
            if (list[i][strlen(str_dir->d_name)-1]=='\\')
            {//If last character is a backslash, remove it
                char *temp=list[i];
                list[i]=new char[strlen(str_dir->d_name)];
                temp[strlen(str_dir->d_name)-1]='\0';
                strcpy(list[i],temp);
                delete[] temp;
            }
            i++;
        }
    }
    closedir(dir);
    qsort(list,dirs_count,sizeof(char *),compare_strings); //Sort the directories names alphabetically
    
    for (i=0;i<numMonitors;i++)//Send name of input_dir to monitors
    {
        const char *i_dir=input_dir.c_str();
        SendMessageToMonitor(i_dir,strlen(i_dir)+1,i);
    }

    CountriesOfMonitors=new string *[numMonitors];
    
    int NumDirsForMonI;
    for (i=0;i<numMonitors;i++)//Send number of subdirectories(countries) to monitors and store the subdirectories
    {
        if (i>=(dirs_count%numMonitors))//Calculate number of subdirectories
                                        //for this monitor
            NumDirsForMonI=dirs_count/numMonitors;
        else
            NumDirsForMonI=1+(dirs_count/numMonitors);
        char *NumDirectories=myIntToStr(NumDirsForMonI);
        SendMessageToMonitor(NumDirectories,strlen(NumDirectories)+1,i); //Send this number to monitor
        delete[] NumDirectories;
        
        //Keep the names of countries of Monitor i in CountriesOfMonitors[i]
        CountriesOfMonitors[i]=new string[NumDirsForMonI];
        for (j=0;j<NumDirsForMonI;j++) //The countries that Monitor i will handle
        {                    //will be chosen by a alphabetical round-robin share
            CountriesOfMonitors[i][j]=list[i+j*numMonitors];//So, Monitor i will handle countries in
        }                                                   //position i+j*numMonitors for all j
    }
    
    for (i=0;i<dirs_count;i++) //Send the name of each subdirectory
    {                          //to the respective monitor
        SendMessageToMonitor(list[i],strlen(list[i])+1,i%numMonitors);
    }
    
    for (i=0;i<dirs_count;i++)
    {
        delete[] list[i];
    }
    delete[] list;
    
    for (i=0;i<numMonitors;i++)
    {
        char *BloomSize=myUnLongToStr(sizeOfBloom);
        SendMessageToMonitor(BloomSize,strlen(BloomSize)+1,i);//Send BloomFilter size
        delete[] BloomSize;
    }
    
    fd_set readFifos;
    int countNumM=numMonitors;
    
    //Receive all BloomFilters and "READY" messages. Use select to read from the Monitors which have
    //already sent data. If a monitor has already sent some data, keep reading messages from this
    while (countNumM!=0)//monitor until the "READY" message, since a monitor starts sending messages
    { //when it has initialized everything. As a result, there will be no significant delay between
        FD_ZERO(&readFifos); //messages from this monitor.
        for (i=0;i<numMonitors;i++) //Watch with select all fifos that travelMonitor is reading
        {
            FD_SET(receiverNamedPipes[i],&readFifos);
        }
        
        int check;
        check=select(1+FindMaxReceiverNamedPipe(),&readFifos,NULL,NULL,NULL);
        if (check!=-1)//If a monitor has sent data
        {
            for (i=0;i<numMonitors;i++) //Find which monitors have written to the named pipes
            {
                if (FD_ISSET(receiverNamedPipes[i],&readFifos)!=0)
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
    
    for (i=0; i<numMonitors; i++) //Close named pipes
    {
        close(receiverNamedPipes[i]);
        close(senderNamedPipes[i]);
    }
    if (pids!=NULL)
        delete[] pids;
    if (receiverNamedPipes!=NULL)
        delete[] receiverNamedPipes;
    if (senderNamedPipes!=NULL)
        delete[] senderNamedPipes;
}

bool AppTravelMonitor::StartTravelMonitor()
{
    bool SignalOperation=false; //When true, indicates that a signal operation was executed
    
    static  struct  sigaction sAct;
    //Set how to handle the signals
    sigfillset(&(sAct.sa_mask));
    sAct.sa_handler=catchAndHandleSignals;
    sigaction(SIGCHLD,&sAct,NULL);
    sigaction(SIGINT,&sAct,NULL);
    sigaction(SIGQUIT,&sAct,NULL);
    
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
        
        //Watch if strdin or a receiverNamedPipe has data to be read
        fd_set readMyFd;
        FD_ZERO(&readMyFd);
        FD_SET(0,&readMyFd); //Include stdin
        int j;
        for (j=0;j<numMonitors;j++)//Include receiverNamedPipes
        {
            FD_SET(receiverNamedPipes[j],&readMyFd);
        }

        int max=FindMaxReceiverNamedPipe();
        //If no signal was caught, block until a signal is caught, user has written a command in stdin or a
        if (Mode==0 && select(1+max,&readMyFd,NULL,NULL,NULL)!=-1) //Monitor has sent data through a named pipe
        {   //If a Monitor sent data or the user wrote in stdin
            bool FoundReadyFifo=false;
            int i=0;
            while (i<numMonitors && !FoundReadyFifo) //Search if data can by read from a receiverNamedPipe
            {
                //If found data, it means that this Monitor received a SIGUSR1 signal and has sent Bloomfilters
                if (FD_ISSET(receiverNamedPipes[i],&readMyFd)!=0) //since this is the only case a Monitor starts
                {                                               //sending data without receiving any Message
                    cout<<endl;
                    cout<<"Updating... Please Wait..."<<endl;
                    ReceiveAndUpdateBloomFilters(i); //Receive and update the Bloomfilters
                    FoundReadyFifo=true;
                    SignalOperation=true; //Indicate that an operation performed because of a signal(without a command)
                    command="";
                }
                i++;
            }
            if (!FoundReadyFifo) //If no named pipe had data to be read, it means that stdin has.
            {
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
            
        }
        else if (errno==EINTR || Mode!=0) //If a signal was caught
        {
            if (Mode==1)//Caught SIGINT,SIGQUIT
            {
                command="/exit"; //Execute exit operation
            }
            else if (Mode==2 || Mode==3)//Caught SIGCHLD
            {
                Mode-=2; //Change Mode flag accordingly
                
                int i;
                for (i=0;i<numMonitors;i++) //Find all terminated children
                {
                    int statusW;
                    if ((waitpid(pids[i],&statusW,WNOHANG)>0) &&
                        (WIFSIGNALED(statusW) || WIFEXITED(statusW)) )
                    {   //If Monitor i was terminated
                        if (!ReplaceTerminatedMonitor(i))//replace it
                            return false;
                    }
                        
                }
                command="";
                cout<<endl;
            }
            SignalOperation=true;
        }
        
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
            if (!SignalOperation) //and if no operation was initiated by a signal
                cout<<"Invalid command!"<<endl; //It means a wrong command was given by the user
            else
                SignalOperation=false;
        }
    }
    
    exitApp(); //Start /exit operation
    
    cout<<"Program ended!"<<endl;
    
    return true;
}

bool AppTravelMonitor::CreateMonitorWithForkAndExec(int Monitor)
{
    long pid;
    int i;
    
    pid=fork();
    
    if (pid==0 || pid==-1) //child or error
    {
        //Free first dynamically allocated memory
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
        if (pids!=NULL)
            delete[] pids;
        
        for (i=0; i<numMonitors; i++) //Close all named pipes
        {
            if (i!=Monitor) //"Monitor" 's pipes are already closed
            {               //when this function is called
                close(receiverNamedPipes[i]);
                close(senderNamedPipes[i]);
            }
        }
        
        if (receiverNamedPipes!=NULL)
            delete[] receiverNamedPipes;
        if (senderNamedPipes!=NULL)
            delete[] senderNamedPipes;
        
        if (pid==0) //child
        {
            string name1,name2;
            name1=CreateName("rec",Monitor);
            name2=CreateName("send",Monitor);
            execl("./Monitor","./Monitor",name1.c_str(),name2.c_str(),NULL);
            perror("execl"); //Go here only if execl failed
            return false;
        }
        else //error
        {
            perror("fork");
            return false;
        }
    }
    else //parent
    {
        pids[Monitor]=pid; //Store the pid of "Monitor"
    }
    return true;
}

bool AppTravelMonitor::ReplaceTerminatedMonitor(int Monitor)
{
    close(receiverNamedPipes[Monitor]); //close pipes of the terminated
    close(senderNamedPipes[Monitor]);   //monitor
    if (!CreateMonitorWithForkAndExec(Monitor))//Create a new Monitor process
        return false;
    
    //only parent reaches this point
    string name;
    name=CreateName("rec",Monitor);
    receiverNamedPipes[Monitor]=open(name.c_str(),O_RDWR); //open again the two pipes for communication
    name=CreateName("send",Monitor);                       //with the new Monitor process
    while ((senderNamedPipes[Monitor]=open(name.c_str(),O_WRONLY))==-1)
    {
        if (errno!=EINTR)
        {
            perror("open");
            return false;
        }
    }
    
    //Send buffer size to Monitor
    if (!SendFirstMessageToMonitor(Monitor))
        return false;
    
    //Send input_dir to monitor i
    const char *i_dir=input_dir.c_str();
    SendMessageToMonitor(i_dir,strlen(i_dir)+1,Monitor);
    
    //Send number of subdirectories(countries) to Monitor
    int NumDirsForMon;
    if (Monitor>=(dirs_count%numMonitors))//Calculate number of subdirectories
                                          //for this monitor
        NumDirsForMon=dirs_count/numMonitors;
    else
        NumDirsForMon=1+(dirs_count/numMonitors);
    char *NumDirectories=myIntToStr(NumDirsForMon);
    SendMessageToMonitor(NumDirectories,strlen(NumDirectories)+1,Monitor);
    delete[] NumDirectories;
    
    int j;
    for (j=0;j<NumDirsForMon;j++) //Send the names of the subdirectories that
    {//this monitor will handle(=directories the terminated monitor handled)
        const char *cur_dir=CountriesOfMonitors[Monitor][j].c_str();
        SendMessageToMonitor(cur_dir,strlen(cur_dir)+1,Monitor);
    }
    
    //Send bloomSize to Monitor
    char *BloomSize=myUnLongToStr(sizeOfBloom);
    SendMessageToMonitor(BloomSize,strlen(BloomSize)+1,Monitor);//Send BloomFilter size
    delete[] BloomSize;
    
    ReceiveAndUpdateBloomFilters(Monitor); //Receive the BloomFilters of the
                                           //new monitor
    
    char *Message;
    unsigned long mesSize;
    while (strcmp((Message=ReceiveMessageFromMonitor(mesSize,Monitor)),"READY"))
    {   //Receive the "READY" message to confirm that "Monitor" is ready
        delete[] Message;
    }
    delete[] Message;
    return true;
}

char *AppTravelMonitor::ReceiveMessageFromMonitor(unsigned long &mesSize,int Monitor)
{
    char *Message=NULL; //Using the buffer receive a message from "Monitor"
    GetMessage(receiverNamedPipes[Monitor],buf,bufferSize,Message,mesSize);
    return Message;
}

void AppTravelMonitor::SendMessageToMonitor(const char *Message,unsigned long size,int Monitor)
{   //Using the buffer send a message to "Monitor"
    SendMessage(senderNamedPipes[Monitor],buf,bufferSize,Message,size);
    return;
}

void AppTravelMonitor::SendMessageToMonitor(const char *Message,int Monitor)
{   //Using the buffer send a c-string to "Monitor"
    SendMessageToMonitor(Message,strlen(Message)+1,Monitor);
    return;
}

bool AppTravelMonitor::SendFirstMessageToMonitor(int Monitor)
{
    if (bufferSize>1) //If bufferSize>1 byte
    {
        char *BufSizeString=myUnLongToStr(bufferSize); //Convert bufferSize to a string
        strcpy(buf,BufSizeString);                     //and copy it to the buffer
        int index=strlen(BufSizeString)+1;             //Since bufferSize>1, BufSizeString
        delete[] BufSizeString;                        //can be copied to the buffer

        int count_bytes=0;
        int temp_count=0;
        while (count_bytes<index) //Continue sending until the whole string has been sent
        {
            temp_count=write(senderNamedPipes[Monitor],buf,index-count_bytes);
            if (temp_count!=-1) //if no error occurred
            {
                count_bytes+=temp_count; //Add the number of bytes that were actually written
                int k;
                for (k=0;k<(index-count_bytes);k++)//Copy the rest bytes that we must write
                {                                  //in the beginning of the buffer
                    buf[k]=buf[k+temp_count];
                }
            }
            else if (temp_count==-1 && errno!=EINTR)//if the error was not a signal interruption
            {
                perror("write");
                return false;
            }
        }
    }
    else //If bufferSize is only 1
    {
        buf[0]='1'; //First send Ascii code of 1
        int temp_count=0;
        while ((temp_count=write(senderNamedPipes[Monitor],buf,1))<=0) //Send until succeed
        {
            if (temp_count==-1 && errno!=EINTR)//If an error which is not a signal interruption
            {
                perror("write");
                return false;
            }
        }
        
        buf[0]='\0'; //Then send 0
        while ((temp_count=write(senderNamedPipes[Monitor],buf,1))<=0) //Send until succeed
        {
            if (temp_count==-1 && errno!=EINTR)//If an error which is not a signal interruption
            {
                perror("write");
                return false;
            }
        }
    }
    return true;
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

int AppTravelMonitor::FindMaxReceiverNamedPipe()
{
    int i,max=receiverNamedPipes[0];
    for (i=0;i<numMonitors;i++) //Clalculate the maximum
    {
        if (receiverNamedPipes[i]>max)
            max=receiverNamedPipes[i];
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
        kill(pids[Monitor],SIGUSR1); //Send a SIGUSR1 to the Monitor which handles this country
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
        for (i=0;i<numMonitors;i++) //Watch all the receiverNamedPipes
        {
            FD_SET(receiverNamedPipes[i],&readMonitors);
        }
        if (select(1+FindMaxReceiverNamedPipe(),&readMonitors,NULL,NULL,NULL)!=-1)
        {   //If at least a named pipe has data to read
            for (i=0;i<numMonitors;i++)//Check which named pipes can be read
            {
                if (FD_ISSET(receiverNamedPipes[i],&readMonitors)!=0)
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
        kill(pids[i],SIGKILL); //Send SIGKILL to all monitors
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

void catchAndHandleSignals(int sigNo)
{
    if (sigNo==SIGQUIT || sigNo==SIGINT)//If SIGINT or SIGQUIT was caught
    {
        if (AppTravelMonitor::Mode==0 || AppTravelMonitor::Mode==2) //Change mode accorgingly
            AppTravelMonitor::Mode+=1;
    }
    else if (sigNo==SIGCHLD) //If SIGCHLD was caught
    {
        if (AppTravelMonitor::Mode==0)//If already caught SIGQUIT,SIGINT, ignore SIGCHLD
            AppTravelMonitor::Mode=2; //Change mode accorgingly
    }
    
    return;
}
