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
#include <pthread.h>
#include "helpfulStringFunctions.h"
#include "date_citizen.h"
#include "AppMonitor.h"
#include "messageExchange.h"

#ifndef HOST_NAME_MAX //If HOST_NAME_MAX is not defined in this system, use _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif

using namespace std;

//Protection of the variables of the data structures and of the cyclic buffer is happening inside
//the reltive methods.

pthread_mutex_t ExitMtx; //A mutex to protect ExitNow flag

pthread_mutex_t CyclBufMtx; //A mutex to protect CyclicBuffer and synchronize various operations
pthread_cond_t NotEmptyBufOrExitCond; //Wait until CyclicBuffer is not empty or the thread must terminate
pthread_cond_t NotFullBufCond; //Wait until CyclicBuffer is not full

pthread_mutex_t InsertFromFileMtx; //A mutex to protect PossibleProcessing flag and the update of data structures
pthread_cond_t ProcessingDoneCond; //Wait until no updating operation of data structures is happening

void *threadOperation(void *App_Mon)
{
    AppMonitor *AppMon=(AppMonitor *) App_Mon;
    string fName;
    
    pthread_mutex_lock(&ExitMtx);
    while (!(AppMon->ExitNow)) //Continue/wait until ExitNow flag is true
    {
        pthread_mutex_unlock(&ExitMtx);
        
        pthread_mutex_lock(&InsertFromFileMtx);
        //Start of "PossibleProcessing section"
        AppMon->PossibleProcessing++; //Indicate that at least a thread(this one) may currently be
        pthread_mutex_unlock(&InsertFromFileMtx); //updating the data structures

        fName=AppMon->cyclBuf.PopFileName();//Try to get a filename
        if (fName!="") //If a filename was obtained
        {
            //Inform the listening thread that the cyclic buffer is not full and can insert a filename.
            //This signal might not be received by that thread, but this not a problem since we
            //will guarantee later that the listening thread will insert a filename when the cyclic
            pthread_cond_signal(&NotFullBufCond); //buffer is empty(in the worst case).
            
            AppMon->InsertDataFromFile(fName); //Read the file and update the data structures
            
            pthread_mutex_lock(&InsertFromFileMtx);
            AppMon->PossibleProcessing--; //Indicate that this thread is not updating data anymore
            //End of "PossibleProcessing section"
            pthread_cond_signal(&ProcessingDoneCond); //If the listening thread is waiting for the
                            //update to end, inform it that it has ended(from this thread at least)
            pthread_mutex_unlock(&InsertFromFileMtx);
        }
        else
        {
            pthread_mutex_lock(&InsertFromFileMtx);
            AppMon->PossibleProcessing--; //Indicate that this thread is not updating data
            //End of "PossibleProcessing section"
            pthread_cond_signal(&ProcessingDoneCond);//If the listening thread is waiting for an
             //update to end, inform it that no update is happening(from this thread at least)
            pthread_mutex_unlock(&InsertFromFileMtx);

            pthread_mutex_lock(&CyclBufMtx);
            pthread_cond_signal(&NotFullBufCond);//Since this thread found the cyclbuffer empty, send a
                                  //signal to the listening thread to inform it that buffer is not full.
            pthread_mutex_lock(&ExitMtx);//This lock cannot create deadlock since every ExitMtx lock is followed
            if (!(AppMon->ExitNow)) // by an unlock without any intermediate function call that can cause problems.
            {   //Cond_wait only if ExitNow flag is false
                pthread_mutex_unlock(&ExitMtx);
                pthread_cond_wait(&NotEmptyBufOrExitCond,&CyclBufMtx); //Wait until the cylic buffer is not
                //empty or this thread must exit. The lock and the signal here as well as in the listening
                //thread(see code and comments in GetFilesUpdateCyclBufferCondWait) make sure that no thread
                //that needs to procceed will remain in cond_wait.
            }
            else
            {
                pthread_mutex_unlock(&ExitMtx);
            }
            pthread_mutex_unlock(&CyclBufMtx);
            
        }
        pthread_mutex_lock(&ExitMtx);
    }
    pthread_mutex_unlock(&ExitMtx);
    return NULL;
}

AppMonitor::AppMonitor(int Socket,char **Arguments,int numArgs)
:cyclBuf(myStrToUnLong(Arguments[8]))
{
    this->Socket=-1; //Some initializations until
    buf=NULL;  //the proper initializations
    tids=NULL; //have been completed
    Countries=NULL;
    FileNames=NULL;
    VList=NULL;
    CitizenList=NULL;
    
    int i;
    sockBufferSize=myStrToUnLong(Arguments[6]);
    numThreads=atoi(Arguments[4]);
    string name;
    ExitNow=false;
    PossibleProcessing=0;
    AccReqs=0;
    RejReqs=0;
    srand((unsigned int) time(NULL));//Initialize in order to use it in other methods
    
    buf=new char[sockBufferSize]; //Create the buffer that will be used for all the message exchanges
                                  //with the parent process from now on
    
    string FilePath=Arguments[11];
    SplitPath(FilePath,input_dir,name);
    
    NumCountries=numArgs-11;
    
    Countries=new string[NumCountries];
    FileNames=new NamesList[NumCountries];
    
    for (i=0;i<NumCountries;i++)//Retrieve the country names that this monitor will handle
    {
        FilePath=Arguments[11+i];
        SplitPath(FilePath,name,Countries[i]);
    }
    sizeOfBloom=myStrToUnLong(Arguments[10]);
    
    VList=new VirusList(sizeOfBloom,50,4000000000); /*50 means fair coin. 4 billions is an
    estimation of the maximum expected number of citizens that will be inserted in the
    SkipLists. It does not actually limit the capacity of the Skiplist and it is only used to
    calculate a maximum possible height for the Skiplists(log4000000000) */
    
    CitizenList=new SkipList(4000000000,50);
    
    tids=new pthread_t[numThreads];
    
    pthread_mutex_init(&CyclBufMtx,NULL);
    pthread_mutex_init(&InsertFromFileMtx,NULL);
    pthread_mutex_init(&ExitMtx,NULL);
    
    pthread_cond_init(&NotEmptyBufOrExitCond,NULL);
    pthread_cond_init(&NotFullBufCond,NULL);
    pthread_cond_init(&ProcessingDoneCond,NULL);

    for (i=0;i<numThreads;i++)//Create the threads
    {
        int result;
        if ((result=pthread_create(tids+i,NULL,threadOperation,(void *) this))!=0)
        {
            cout<<"Error: pthread_create: "<<strerror(result)<<endl;
            if (i==0)
                FreeCloseDestroy();
            exit(9);
        }
    }
        
    char MyHostName[HOST_NAME_MAX+1]; //Store here the host name
    struct hostent *LocAddr;
    gethostname(MyHostName,HOST_NAME_MAX+1); //Get the host name
    //Find the IP address
    if ((LocAddr=gethostbyname(MyHostName))==NULL)
    {
        cout<<"Error: gethostbyname"<<endl;
        ExitApp();
        FreeCloseDestroy();
        exit(10);
    }
        
    struct sockaddr_in MonAddr;
    MonAddr.sin_family=AF_INET;
    MonAddr.sin_port=htons(atoi(Arguments[2]));
    //Copy the bytes of local address
    memcpy(&(MonAddr.sin_addr.s_addr),(LocAddr->h_addr_list)[0],LocAddr->h_length);

    //Bind, listen and accept connection with parent process
    if (::bind(Socket,(struct sockaddr *) &MonAddr,sizeof(MonAddr))==-1)
    { //Use "::bind", because "bind" will call the "std::bind" function
        perror("bind");
        ExitApp();
        FreeCloseDestroy();
        exit(5);
    }
    
    if (listen(Socket,1)==-1)
    {
        perror("listen");
        ExitApp();
        FreeCloseDestroy();
        exit(5);
    }
    
    if ((this->Socket=accept(Socket,NULL,NULL))==-1)
    {
        perror("accept");
        ExitApp();
        FreeCloseDestroy();
        exit(5);
    }
    
    GetFilesUpdateCyclBufferCondWait(); //Get the files and make threads update the data structures
    //When this function returns, all data have been updated.
    
    cout<<"Initializing Data... Please wait..."<<endl;
    SendBloomFiltersToParent();
}

AppMonitor::~AppMonitor()
{
    FreeCloseDestroy();
}

void AppMonitor::StartMonitor()
{
    SendMessageToParent("READY",6); //Inform parent that this process is ready
    
    //ExitNow modifying happens only from this thread, so no protection is needed
    while (!ExitNow) //Keep operating until /exit command
    {
        fd_set readSocket;  //Initialize this set in order to watch with select when
        FD_ZERO(&readSocket); //the socket that we read messages from the parent has
        FD_SET(Socket,&readSocket); //data to be read
        
        //Block unitl the socket has data to be read
        if (select(Socket+1,&readSocket,NULL,NULL,NULL)!=-1)
        {
            if (FD_ISSET(Socket,&readSocket)!=0)//Socket has data for reading
            {
                unsigned long mesSize;
                char *Message=ReceiveMessageFromParent(mesSize); //Receive the message from the parent
                string Command(Message); //and check which command it includes
                delete[] Message;
                if (Command=="/travelRequest")
                {
                    Message=ReceiveMessageFromParent(mesSize); //Receive citizen ID
                    long cID=atol(Message);
                    delete[] Message;
                    Message=ReceiveMessageFromParent(mesSize); //Receive the date
                    string tempD=Message;
                    delete[] Message;
                    date d=date(tempD);
                    Message=ReceiveMessageFromParent(mesSize); //Receive the countryFrom
                    string cFrom=Message;
                    delete[] Message;
                    Message=ReceiveMessageFromParent(mesSize); //Receive the countryTo
                    string cTo=Message;
                    delete[] Message;
                    Message=ReceiveMessageFromParent(mesSize); //Receive the virus
                    string vName=Message;
                    delete[] Message;
                    TravelReqAndSend(cID,d,cFrom,vName); //Start this operation
                }
                else if (Command=="/addVaccinationRecords")
                {
                    GetFilesUpdateCyclBufferCondWait(); //Get new files, update the cyclic buffer, wait
                                                        //for threads to finish updating data
                    SendBloomFiltersToParent(); //Send the updated BloomFilters to parent
                }
                else if (Command=="/searchVaccinationStatus")
                {
                    Message=ReceiveMessageFromParent(mesSize); //Receive citizen ID
                    long cID=atol(Message);
                    delete[] Message;
                    SearchVaccStatusAndSend(cID); //Start this operation
                }
                else if (Command=="/exit")
                {
                    CreateFileWithStats();
                    ExitApp(); //Inform the threads that they must terminate. Wait for them.
                }
            }
        }
    }
    
    return;
}

char *AppMonitor::ReceiveMessageFromParent(unsigned long &mesSize)
{
    char *Message=NULL; //Using the buffer receive a message from parent
    GetMessage(Socket,buf,sockBufferSize,Message,mesSize); //"messageExchange.h" file
    return Message;
}

void AppMonitor::SendMessageToParent(const char *Message,unsigned long size)
{
    //Using the buffer send a message to parent
    SendMessage(Socket,buf,sockBufferSize,Message,size); //"messageExchange.h" file
    return;
}

void AppMonitor::SendBloomFiltersToParent()
{
    string vName;
    BloomFilter *curBF=NULL;
    
    int num_viruses=VList->GetCount();
    char *NumStr=myIntToStr(num_viruses);
    SendMessageToParent(NumStr,strlen(NumStr)+1); //Send number of viruses(=number of BloomFilters) to parent
    delete[] NumStr;
    
    vName=VList->GetFirstBloomFilter(curBF); //get the first virus and its bloomFilter
    while ((num_viruses--)!=0) //Send all the BloomFilters to parent
    {
        SendMessageToParent(vName.c_str(),strlen(vName.c_str())+1); //Send the virus name
        SendMessageToParent(curBF->GetBytesOfBloomFilter(),sizeOfBloom); /*Send the array of chars(array of bytes)
                                                                          of the BloomFilter to parent*/
        vName=VList->GetNextBloomFilter(curBF); //get the next virus and its bloomFilter
    }

    return;
}

int AppMonitor::FindCountryIndex(std::string cName)
{
    int i;
    
    for (i=0; i<NumCountries; i++) //Search all countries
    {
        if (Countries[i]==cName)
            return i;
    }
    
    return -1;
}

citizen *AppMonitor::CreateNewCitizen(long ID,std::string fName,std::string lName,
                          std::string cName,int age)
{
    int pos=FindCountryIndex(cName);
    
    citizen *pcitizen=new citizen(ID,fName,lName,age,Countries+pos);
    CitizenList->Insert(pcitizen);

    return pcitizen;
}


void AppMonitor::GetFilesUpdateCyclBufferCondWait()
{
    string pathToCountry;
    int i;
    
    for (i=0;i<NumCountries;i++) //For every country(=directory)
    {
        pathToCountry=(input_dir+"/");
        pathToCountry=pathToCountry+Countries[i]; //Create relative path of the directory
        DIR *dir=opendir(pathToCountry.c_str());
        struct dirent *strDirPtr;
        while ((strDirPtr=readdir(dir))!=NULL) //Read the files of the directory
        {
            if ((strDirPtr->d_name)[0]!='.') //Not hidden files
            {
                if (FileNames[i].FindName(strDirPtr->d_name)==NULL)//If this file has not been included yet
                {
                    string fileName=strDirPtr->d_name;
                    FileNames[i].AddName(fileName); //Add its file name to the list of files
                    string pathToFile=pathToCountry+"/";
                    pathToFile.append(fileName);
                    while (!(cyclBuf.AddFileName(pathToFile)))//Keep waiting until a filename had been
                    {                                         //succesfully added in the cyclic buffer
                        pthread_mutex_lock(&CyclBufMtx);
                        pthread_cond_signal(&NotEmptyBufOrExitCond);/*Since this thread found the cyclbuffer full,
                        send a signal to a thread to infrom it that the buffer is not empty. The moment this is
                        sent either at least one thread is in cond_wait or all of them are out of a "locked section".
                        As a result, since the code in the thread function is similar, no thread that needs to
                        procceed will remain in cond_wait.*/
                        pthread_cond_wait(&NotFullBufCond,&CyclBufMtx); //Wait until the cylic buffer is not full.
                        pthread_mutex_unlock(&CyclBufMtx);
                    }
                    /*Inform one thread that the cyclic buffer is not empty and can read a file. This signal might
                     not be received by any thread, but this not a problem since we have guaranteed that a thread
                     will read a filename when the cyclic buffer is full(in the worst case).*/
                    pthread_cond_signal(&NotEmptyBufOrExitCond);
                }
            }
        }
        closedir(dir);
    }
    
    //When the thread reaches this point, all new files have been put to the buffer.
    
    while (!(cyclBuf.IsEmpty())) //Keep waiting until the cyclic buffer is empty
    { //It works similar to the above while loop.
        pthread_mutex_lock(&CyclBufMtx);
        pthread_cond_signal(&NotEmptyBufOrExitCond);//This prevents other threads from stucking in cond_wait while
                                                    //there are files to be read
        pthread_cond_wait(&NotFullBufCond,&CyclBufMtx); //Wait until a file is removed from the buffer or the buffer
                                                        //is empty(=until a NotFullBufCond signal is received)
        pthread_mutex_unlock(&CyclBufMtx);
    }
    
    //When the thread reaches this point, all files have been removed from the buffer.
    
    pthread_mutex_lock(&InsertFromFileMtx);
    while (PossibleProcessing!=0) //Keep waiting until no thread is processing data
    {   //If PossibleProcessing!=0, then at least one thread is in "PossibleProcessing section". When that thread
        //comes out of this section, it will sent a ProcessingDoneCond signal. Before that signal is sent, this thread
        pthread_cond_wait(&ProcessingDoneCond,&InsertFromFileMtx); //will necessarily be waiting in cond_wait.
    }
    pthread_mutex_unlock(&InsertFromFileMtx);

    //When the thread reaches this point, all data structures have been updated.
    
    return;
}

void AppMonitor::InsertDataFromFile(string fileName)
{
    ifstream infile(fileName);//Open the file
    if (!infile) //If the file is not opened correctly
        return;
    
    string line="";
    istringstream is;
    int count;
    string cID, vName, d1, d2, country, fName, lName, y_or_n,Age;
    int age;
    long ID;
    citizen *pcitizen;
    SkipList *SL;
    
    while (getline(infile,line))//Read one line each time
    {
        count=count_words(line);
        if (count<7 || count>8)
            cout<<"ERROR IN RECORD "<<line<<endl;
        else
        {
            is.str(line);//line is the content of the stream
            is.clear();//clear flags
            //read from the line
            is>>cID;
            is>>fName;
            is>>lName;
            is>>country;
            is>>Age;
            is>>vName;
            is>>y_or_n;
            if (!y_or_n.compare("YES") && (count==8) && isNum(cID) &&
                onlyLetters(fName) && onlyLetters(lName) && (FindCountryIndex(country)!=-1) &&
                isAge(Age) && onlyLetNumPlusMostOneDash(vName) )
            {
                is>>d1;
                date date1(d1);
                ID=atol(cID.c_str());
                age=atoi(Age.c_str());
                
                pthread_mutex_lock(&InsertFromFileMtx);
                SL=VList->GetNonVaccinatedSkipList(vName);
                //Check if virus is already in the virus list
                if (SL==NULL)//if not
                {
                    VList->InsertVirus(vName);//add him
                    pcitizen=NULL;
                }
                else//If it is
                {
                    pcitizen=VList->GetNonVaccinatedSkipList(vName)->Search(ID).c;
                }
                pthread_mutex_unlock(&InsertFromFileMtx);

                //Check if the citizen is in the non Vaccinated SkipList
                if (pcitizen!=NULL)
                    cout<<"ERROR IN RECORD "<<line<<endl;
                else
                {
                    //Check if the citizen is in the citizen SkipList
                    pthread_mutex_lock(&InsertFromFileMtx);
                    pcitizen=CitizenList->Search(ID).c;
                    if (pcitizen==NULL) //If he is not there
                    { //Create new citizen and insert him to the Vaccinated Skiplist
                        pcitizen=CreateNewCitizen(ID,fName,lName,country,age);
                        citizenANDdate CD(pcitizen,date1);
                        //Insert him
                        VList->GetVaccinatedSkipList(vName)->Insert(CD);
                        VList->GetBloomFilter(vName)->Insert(cID);
                        pthread_mutex_unlock(&InsertFromFileMtx);
                    }
                    else if (pcitizen->isTheSame(ID,fName,lName,age,country))
                        //If he is there and personal info is correct
                    {   //Insert him in the Vaccinated SkipList
                        citizenANDdate CD(pcitizen,date1);
                        //Check first if he is already vaccinated
                        if (!VList->GetVaccinatedSkipList(vName)->Insert(CD))
                            cout<<"ERROR IN RECORD "<<line<<endl;
                        else
                            VList->GetBloomFilter(vName)->Insert(cID);
                        pthread_mutex_unlock(&InsertFromFileMtx);
                    }
                    else //If personal info is incorrect
                    {
                        pthread_mutex_unlock(&InsertFromFileMtx);
                        cout<<"ERROR IN RECORD "<<line<<endl;
                    }
                }
            }
            else if (!y_or_n.compare("NO"))
            {
                if (count==8 || !isNum(cID) || !onlyLetters(fName) ||
                    !onlyLetters(lName) || (FindCountryIndex(country)==-1) || !isAge(Age) ||
                    !onlyLetNumPlusMostOneDash(vName) )
                    cout<<"ERROR IN RECORD "<<line<<endl;
                else
                {
                    age=atoi(Age.c_str());
                    //Try to insert the record
                    if (!insertCitizenRec(cID,fName,lName,country,age,vName))
                        cout<<"ERROR IN RECORD "<<line<<endl;
                }
            }
            else
                cout<<"ERROR IN RECORD "<<line<<endl;
        }
    }
    return;
}

bool AppMonitor::insertCitizenRec(string citizenID,string fName,string lName,
                               string cName,int age,string vName)
{
    long ID=atol(citizenID.c_str());
    citizen *pcitizen;
    
    pthread_mutex_lock(&InsertFromFileMtx);
    SkipList *VSL=VList->GetVaccinatedSkipList(vName);
    
    if (VSL==NULL)//if there no record yet for this virus
    {
        VList->InsertVirus(vName);//add him
        pcitizen=NULL;
    }
    else
        pcitizen=VSL->Search(ID).c;
    pthread_mutex_unlock(&InsertFromFileMtx);

    if (pcitizen!=NULL)//If the citizen has already vaccinated
        return false;
    else
    {
        //Find citizen in the citizen Skiplist
        pthread_mutex_lock(&InsertFromFileMtx);
        pcitizen=CitizenList->Search(ID).c;

        if (pcitizen==NULL)
        {//If there is no record of the citizen
            pcitizen=CreateNewCitizen(ID,fName,lName,cName,age);
            VList->GetNonVaccinatedSkipList(vName)->Insert(pcitizen);
            pthread_mutex_unlock(&InsertFromFileMtx);
        }
        else if (pcitizen->isTheSame(ID,fName,lName,age,cName))
        {//If there is a consistent record of the citizen in citizenSkipList
            if (!VList->GetNonVaccinatedSkipList(vName)->Insert(pcitizen))
            {
                pthread_mutex_unlock(&InsertFromFileMtx);
                return false;
            }
            pthread_mutex_unlock(&InsertFromFileMtx);
        }
        else//If his personal info is incosistent with a record of the citizen
        {
            pthread_mutex_unlock(&InsertFromFileMtx);
            return false;
        }
    }
    return true;
}

void AppMonitor::TravelReqAndSend(long citizenID,date d,std::string countryFrom,std::string vName)
{
    SkipList *SL=VList->GetVaccinatedSkipList(vName); //Get vaccinated SkipList for this virus
    
    citizenANDdate cd=SL->Search(citizenID);
    if (cd.c==NULL)//If citizen is not found
    {
        SendMessageToParent("NO",3); //Send "NO" to parent
        RejReqs++;
    }
    else if ((*(cd.c->pcountry))!=countryFrom) //Incosistent data
    {
        SendMessageToParent("X",2); //Send "X" to parent
    }
    else //Citizen is found and the country is consistent
    {
        string answer("YES ");
        answer.append(cd.d.getDate()); //Send a string with "YES " plus the vaccination date(in string form)
        SendMessageToParent(answer.c_str(),strlen(answer.c_str())+1); //to the parent
        
        if (cd.d.compare(d)<=0) //If travel date is after the vaccination
        {
            date sixMbeforeD;
            int year=0,month=0,day=0;
            
            d.getYMD(year,month,day);
            //Calculate the date which is 6 months before the travel date
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
            
            if (cd.d.compare(sixMbeforeD)<=0) //If vaccination date is before 6 months before the travel date
                RejReqs++;
            else //If vaccination date is between 6 months before the travel date and the actual travel date
                AccReqs++;
        }
        else //If travel date is before the vaccination
        {
            RejReqs++;
        }
    }
    
    return;
}


void AppMonitor::SearchVaccStatusAndSend(long citizenID)
{
    citizenANDdate cd=CitizenList->Search(citizenID); //Search if the citizen is in monitor's records
    char *Message;
    unsigned long mSize;
    
    if (cd.c==NULL) //If citizen is not found
    {
        SendMessageToParent("N",2); //Send the string "N" to parent
        Message=ReceiveMessageFromParent(mSize); //Receive parent's answer
        delete[] Message; //and discard it
        return;
    }
    else //If citizen is found
    {
        SendMessageToParent("Y",2); //Send the string "Y" to parent
        Message=ReceiveMessageFromParent(mSize);
        if (Message[0]=='R') //If parent's answer is "R", means he rejects communication
        {
            delete[] Message;
            return;
        }
        else //If parent's answer is "A", means he accepts communication
            delete[] Message;
    }
    
    //Communication has been accepted if we reach this point
    string ID=CreateName("",citizenID);
    string line1=ID; //Send Id, first name, last name,country in a string with " " between them
    line1.append(" ");
    line1.append(cd.c->fname);
    line1.append(" ");
    line1.append(cd.c->lname);
    line1.append(" ");
    line1.append(*(cd.c->pcountry));
    SendMessageToParent(line1.c_str(),strlen(line1.c_str())+1);
    string line2=CreateName("AGE ",cd.c->age); //Send the string "AGE " with the age after that
    SendMessageToParent(line2.c_str(),strlen(line2.c_str())+1);
    
    
    SkipList *SL=NULL;
    string vname;
    int i;
    int vnum=VList->GetCount();
    int num_Vac=0,num_NonVac=0;
    
    vname=VList->GetFirstSkipList(SL,true);
    for (i=0;i<vnum;i++)//Count number of viruses that this citizen has been vaccinated for(postive records).
    {
        cd=SL->Search(citizenID);
        if (cd.c!=NULL)
            num_Vac++;
        vname=VList->GetNextSkipList(SL,true);
    }
    string num_p=CreateName("",num_Vac); //Send the number of vaccination records that were found
    SendMessageToParent(num_p.c_str(),strlen(num_p.c_str())+1);
    
    vname=VList->GetFirstSkipList(SL,false);
    for (i=0;i<vnum;i++)//Count number of viruses that this citizen has a negative(not vaccinated) record for.
    {
        cd=SL->Search(citizenID);
        if (cd.c!=NULL)
            num_NonVac++;
        vname=VList->GetNextSkipList(SL,false);
    }
    string num_n=CreateName("",num_NonVac); //Send this number to parent
    SendMessageToParent(num_n.c_str(),strlen(num_n.c_str())+1);
    
    string record;
    
    vname=VList->GetFirstSkipList(SL,true);
    for (i=0;i<vnum;i++)//Send all positive records with the same format that we want to print them
    {
        cd=SL->Search(citizenID);
        if (cd.c!=NULL)
        {
            record=vname;
            record.append(" VACCINATED ON ");
            record.append(cd.d.getDate());
            SendMessageToParent(record.c_str(),strlen(record.c_str())+1);
        }
        vname=VList->GetNextSkipList(SL,true);
    }
    
    vname=VList->GetFirstSkipList(SL,false);
    for (i=0;i<vnum;i++)//Send all negative records with the same format that we want to print them
    {
        cd=SL->Search(citizenID);
        if (cd.c!=NULL)
        {
            record=vname;
            record.append(" NOT YET VACCINATED");
            SendMessageToParent(record.c_str(),strlen(record.c_str())+1);
        }
        vname=VList->GetNextSkipList(SL,false);
    }
    
    return;
}

void AppMonitor::ExitApp()
{
    int i;
    
    //The lock of two mutexes here cannot create deadlock since every ExitMtx lock is followed by an unlock
    //without any intermediate function call that can cause problems.
    pthread_mutex_lock(&CyclBufMtx);
    pthread_mutex_lock(&ExitMtx);
    ExitNow=true;
    pthread_cond_broadcast(&NotEmptyBufOrExitCond); /*Locking both mutexes ensures that when broadcast is
    called, every thread is already blocked in cond_wait or will not enter the section that calls cond_wait.
    So, after this call, all threads will exit the while loop and will return.*/
    pthread_mutex_unlock(&ExitMtx);
    pthread_mutex_unlock(&CyclBufMtx);
    
    for (i=0;i<numThreads;i++)//Wait for the threads to terminate
    {
        pthread_join(tids[i],NULL);
    }
    
    return;
}

void AppMonitor::CreateFileWithStats()
{
    string fileName("log_file.");
    char *pidM=myUnLongToStr(getpid());
    fileName.append(pidM); //Create the name of log_file
    delete[] pidM;
    ofstream outfile(fileName.c_str(),ofstream::out|ofstream::trunc); //Open for writing and truncate it
    
    int i;
    for (i=0;i<NumCountries;i++)
    {
        outfile<<Countries[i]<<endl; //Write all the countries that this monitor handles
    }
    
    //Write stats in the log_file
    outfile<<"TOTAL TRAVEL REQUESTS "<<AccReqs+RejReqs<<endl;
    outfile<<"ACCEPTED "<<AccReqs<<endl;
    outfile<<"REJECTED "<<RejReqs<<endl;
    
    return;
}

void AppMonitor::FreeCloseDestroy()
{
    if (Socket!=-1)
        close(Socket);
    
    pthread_mutex_destroy(&ExitMtx);
    
    pthread_mutex_destroy(&CyclBufMtx);
    pthread_cond_destroy(&NotEmptyBufOrExitCond);
    pthread_cond_destroy(&NotFullBufCond);
    
    pthread_mutex_destroy(&InsertFromFileMtx);
    pthread_cond_destroy(&ProcessingDoneCond);
    
    if (buf!=NULL)
        delete[] buf;
    if (tids!=NULL)
        delete[] tids;
    if (Countries!=NULL)
        delete[] Countries;
    if (FileNames!=NULL)
        delete[] FileNames;
    if (CitizenList!=NULL)
    {
        CitizenList->DestroyAll();
        delete CitizenList;
    }
    if (VList!=NULL)
        delete VList;
    return;
}

AppMonitor::CyclicBuffer::CyclicBuffer(unsigned long cyclBufferSize)
:cyclBufferSize(cyclBufferSize)
{
    cyclicBuf=new string[cyclBufferSize];
    first=cyclBufferSize;
    last=cyclBufferSize;
}

AppMonitor::CyclicBuffer::~CyclicBuffer()
{
    delete[] cyclicBuf;
}

bool AppMonitor::CyclicBuffer::IsEmpty()
{
    bool BufEmpty=false;
    
    pthread_mutex_lock(&CyclBufMtx);
    if (last==cyclBufferSize) //Empty buffer
        BufEmpty=true;
    pthread_mutex_unlock(&CyclBufMtx);
    
    return BufEmpty;
}

bool AppMonitor::CyclicBuffer::AddFileName(string fName)
{
    pthread_mutex_lock(&CyclBufMtx);
    if ( ((last+1)%cyclBufferSize) == first) //Full buffer
    {
        pthread_mutex_unlock(&CyclBufMtx);
        return false;
    }
    
    if (last==cyclBufferSize) //Empty buffer
    {
        first=0;
        last=0;
    }
    else
    {
        last=(last+1)%cyclBufferSize;
    }
    cyclicBuf[last]=fName; //Add the new file
    pthread_mutex_unlock(&CyclBufMtx);
    return true;
}

string AppMonitor::CyclicBuffer::PopFileName()
{
    string temp;
    pthread_mutex_lock(&CyclBufMtx);
    if (first==last) //Empty buffer or buffer with one file
    {
        if (first==cyclBufferSize) //Empty buffer
        {
            temp="";
        }
        else //Buffer with one file
        {
            temp=cyclicBuf[first];
            first=cyclBufferSize;
            last=cyclBufferSize;
        }
    }
    else
    {
        temp=cyclicBuf[first];
        first=(first+1)%cyclBufferSize;
    }
    pthread_mutex_unlock(&CyclBufMtx);
    return temp;
}
