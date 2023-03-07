#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include "helpfulStringFunctions.h"
#include "date_citizen.h"
#include "AppMonitor.h"
#include "messageExchange.h"

using namespace std;

void catchMultipleSignals(int sigNo); //Signal handler

int AppMonitor::Mode=0; //Initialize Mode

AppMonitor::AppMonitor(int monitorReceiverNamedPipe,int monitorSenderNamedPipe)
{
    int i;
    bufferSize=0;
    string name;
    char FirstMessage[1];//It is used only for receiving the first message
                         //which contains the buffer size
    unsigned long mesSize=0;
    char *Message;
    this->monitorSenderNamedPipe=monitorSenderNamedPipe;
    this->monitorReceiverNamedPipe=monitorReceiverNamedPipe;
    AccReqs=0;
    RejReqs=0;
    srand((unsigned int) time(NULL));//Initialize in order to use it in other methods
    
    
    //Read only the first message, which contains a string with the buffer size, byte-by-byte.
    //Each char(byte) is the ascii code of a digit of the buffer size
    int temp_count;
    while ((temp_count=read(monitorReceiverNamedPipe,FirstMessage,1))<=0) //Read the first byte
    {
        if (temp_count==-1 && errno!=EINTR) //If the error was not a signal
        {
            perror("read");
            exit(-1);
        }
    }
    while ((*FirstMessage)!=0)//Continue reading the digits of the buffer size
    {   
        if ((*FirstMessage)!=0)//Stop when '\0' is found
        {
            bufferSize*=10; //Decode the buffer size
            bufferSize+=(unsigned long) ((*FirstMessage)-'0');
            while ((temp_count=read(monitorReceiverNamedPipe,FirstMessage,1))<=0)
            {
                if (temp_count==-1 && errno!=EINTR) //If the error was not a signal
                {
                    perror("read");
                    exit(-1);
                }
            }
        }
    }

    buf=new char[bufferSize]; //Create the buffer that will be used for all the message exchanges
                              //with the parent process from now on

    Message=ReceiveMessageFromParent(mesSize); //Receive input_dir
    input_dir=Message;
    delete[] Message;
    
    Message=ReceiveMessageFromParent(mesSize); //Receive number of countries this monitor will handle
    NumCountries=atoi(Message);
    delete[] Message;
    
    Countries=new string[NumCountries];
    FileNames=new NamesList[NumCountries];
    
    for (i=0;i<NumCountries;i++)//Receive the country names that this monitor will handle
    {
        Message=ReceiveMessageFromParent(mesSize);
        Countries[i]=Message;
        delete[] Message;
    }
    
    Message=ReceiveMessageFromParent(mesSize); //Receive the bloomfilter size
    sizeOfBloom=myStrToUnLong(Message);
    delete[] Message;
    
    VList=new VirusList(sizeOfBloom,50,4000000000); /*50 means fair coin. 4 billions is an
    estimation of the maximum expected number of citizens that will be inserted in the
    SkipLists. It does not actually limit the capacity of the Skiplist and it is only used to
    calculate a maximum possible height for the Skiplists(log4000000000) */
    
    CitizenList=new SkipList(4000000000,50);

    UpdateData(); //Read the files and update the data
    cout<<"Initializing Data... Please wait..."<<endl;
    SendBloomFiltersToParent();
}

AppMonitor::~AppMonitor()
{
    delete[] buf;
    delete[] Countries;
    delete[] FileNames;
    CitizenList->DestroyAll();
    delete CitizenList;
    delete VList;
}

void AppMonitor::StartMonitor()
{
    //Set how to handle the signals
    static  struct  sigaction sAct;
    sigfillset(&(sAct.sa_mask));
    sAct.sa_handler=catchMultipleSignals;
    sigaction(SIGINT,&sAct,NULL); //Include the SIGINT,
    sigaction(SIGQUIT,&sAct,NULL); //SIGQUIT and
    sigaction(SIGUSR1,&sAct,NULL); //SIGUSR1 signals

    SendMessageToParent("READY",6); //Inform parent that this process is ready
    
    while ((1)) //Keep operating forever. Only SIGKILL stops this monitor
    {
        fd_set readFifo;  //Initialize this set in order to watch with select when
        FD_ZERO(&readFifo); //the fifo that we read messages from the parent has
        FD_SET(monitorReceiverNamedPipe,&readFifo); //data to be read
        
        //If no signal was caught, block unitl a signal is caught or the "receiver" fifo has data to
        if (Mode==0 && select(monitorReceiverNamedPipe+1,&readFifo,NULL,NULL,NULL)!=-1)//be read
        {
            if (FD_ISSET(monitorReceiverNamedPipe,&readFifo)!=0)//monitorReceiverNamedPipe
            {                                                   //has data for reading
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
                else if (Command=="/searchVaccinationStatus")
                {
                    Message=ReceiveMessageFromParent(mesSize); //Receive citizen ID
                    long cID=atol(Message);
                    delete[] Message;
                    SearchVaccStatusAndSend(cID); //Start this operation
                }
            }
        }
        else if (errno==EINTR || Mode!=0) //A signal was caught
        {
            if (Mode==1)//Caught SIGINT or SIGQUIT
            {
                Mode--; //Reastore Mode to 0
                CreateFileWithStats(); //Create the log_file
            }
            else if (Mode==2 || Mode==3)//Caught SIGUSR1
            {
                if (Mode==3)//Caught both SIGUSR1 and (SIGINT or SIGQUIT)
                    Mode=1; //Execute the SIGUSR1 operation and then execute the operation
                            //of the other signal
                
                else        //Only SIGUSR1 was caught
                    Mode=0; //Restore Mode to 0
                
                UpdateData(); //Read new files, update data
                SendBloomFiltersToParent(); //and send the updated BloomFilters to parent
            }
        }
    }
    
    return;
}

char *AppMonitor::ReceiveMessageFromParent(unsigned long &mesSize)
{
    char *Message=NULL; //Using the buffer receive a message from parent
    GetMessage(monitorReceiverNamedPipe,buf,bufferSize,Message,mesSize); //"messageExchange.h" file
    return Message;
}

void AppMonitor::SendMessageToParent(const char *Message,unsigned long size)
{
    //Using the buffer send a message to parent
    SendMessage(monitorSenderNamedPipe,buf,bufferSize,Message,size); //"messageExchange.h" file
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


void AppMonitor::UpdateData()
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
                    InsertDataFromFile(Countries[i],strDirPtr->d_name); //Read it and update data
                    FileNames[i].AddName(strDirPtr->d_name); //Add its file name to the list of files
                }
            }
        }
        closedir(dir);
    }
    
    return;
}

void AppMonitor::InsertDataFromFile(std::string cName,std::string fileName)
{
    string rPathToFile=input_dir+"/"; //Create relative path to the file
    rPathToFile.append(cName);
    rPathToFile.append("/");
    rPathToFile.append(fileName);
    
    ifstream infile(rPathToFile.c_str());//Open the file
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

                SL=VList->GetNonVaccinatedSkipList(vName);
                //Check if virus is already in the virus list
                if (SL==NULL)//if not
                {
                    VList->InsertVirus(vName);//add him
                    pcitizen=NULL;
                }
                else//If it is
                    pcitizen=VList->GetNonVaccinatedSkipList(vName)->Search(ID).c;

                //Check if the citizen is in the non Vaccinated SkipList
                if (pcitizen!=NULL)
                    cout<<"ERROR IN RECORD "<<line<<endl;
                else
                {
                    //Check if the citizen is in the citizen SkipList
                    pcitizen=CitizenList->Search(ID).c;
                    if (pcitizen==NULL) //If he is not there
                    { //Create new citizen and insert him to the Vaccinated Skiplist
                        
                        pcitizen=CreateNewCitizen(ID,fName,lName,country,age);
                        citizenANDdate CD(pcitizen,date1);
                        //Insert him
                        VList->GetVaccinatedSkipList(vName)->Insert(CD);
                        VList->GetBloomFilter(vName)->Insert(cID);
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
                    }
                    else //If personal info is incorrect
                    {
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
    SkipList *VSL=VList->GetVaccinatedSkipList(vName);
    
    if (VSL==NULL)//if there no record yet for this virus
    {
        VList->InsertVirus(vName);//add him
        pcitizen=NULL;
    }
    else
        pcitizen=VSL->Search(ID).c;
    
    if (pcitizen!=NULL)//If the citizen has already vaccinated
        return false;
    else
    {
        //Find citizen in the citizen Skiplist
        pcitizen=CitizenList->Search(ID).c;
        if (pcitizen==NULL)
        {//If there is no record of the citizen
            pcitizen=CreateNewCitizen(ID,fName,lName,cName,age);
            VList->GetNonVaccinatedSkipList(vName)->Insert(pcitizen);
        }
        else if (pcitizen->isTheSame(ID,fName,lName,age,cName))
        {//If there is a consistent record of the citizen in citizenSkipList
            if (!VList->GetNonVaccinatedSkipList(vName)->Insert(pcitizen))
                return false;
        }
        else//If his personal info is incosistent with a record of the citizen
            return false;
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


void catchMultipleSignals(int sigNo) //Signal handler
{
    if ((sigNo==SIGINT) || (sigNo==SIGQUIT)) //If SIGINT or SIGQUIT was caught
    {
        if ((AppMonitor::Mode==0) || (AppMonitor::Mode==2)) //Change mode accorgingly
            AppMonitor::Mode++;
    }
    else if (sigNo==SIGUSR1)//If SIGUSR1 was caught
    {
        if ((AppMonitor::Mode==0) || (AppMonitor::Mode==1)) //Change mode accorgingly
            AppMonitor::Mode+=2;
    }
    return;
}

