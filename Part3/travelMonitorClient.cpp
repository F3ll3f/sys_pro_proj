#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include "helpfulStringFunctions.h"
#include "AppTravelMonitor.h"

int findValidFlag(char **arguments,const char *flag);

using namespace std;

int main(int argc,char *argv[])
{
    int numThreads,numMonitors;
    unsigned long sockBufferSize,cyclBufferSize,sizeOfBloom;
    string input_dir;
    int pos;
    
    if (argc!=13)
    {
        cout<<"Wrong format: Wrong number of arguments"<<endl;
        return -1;
    }
    
    if ((pos=findValidFlag(argv,"-m"))>0) //Find position of the "-m" flag in the arguments
    {
        if (isNum(argv[pos+1]))//Check that next argument is a number
        {
            numMonitors=atoi(argv[pos+1]);
            if (numMonitors<=0)
            {
                cout<<"numMonitors is not a positive number."<<endl;
                return -2;
            }
        }
        else
        {
            cout<<"numMonitors is not a positive number."<<endl;
            return -2;
        }
    }
    else
    {
        cout<<"Wrong format: Flag -m was not found"<<endl;
        return -2;
    }
        
    if ((pos=findValidFlag(argv,"-b"))>0)//Find position of the "-b" flag in the arguments
    {
        if (isNum(argv[pos+1])) //Check that next argument is a number
        {
            sockBufferSize=(unsigned long) (atol(argv[pos+1]));
            if (sockBufferSize<=0)
            {
                cout<<"socketBufferSize is not a positive number."<<endl;
                return -3;
            }
        }
        else
        {
            cout<<"socketBufferSize is not a positive number."<<endl;
            return -3;
        }
    }
    else
    {
        cout<<"Wrong format: Flag -b was not found"<<endl;
        return -3;
    }
    
    if ((pos=findValidFlag(argv,"-c"))>0)//Find position of the "-c" flag in the arguments
    {
        if (isNum(argv[pos+1])) //Check that next argument is a number
        {
            cyclBufferSize=(unsigned long) (atol(argv[pos+1]));
            if (cyclBufferSize<=0)
            {
                cout<<"cyclicBufferSize is not a positive number."<<endl;
                return -8;
            }
        }
        else
        {
            cout<<"cyclicBufferSize is not a positive number."<<endl;
            return -8;
        }
    }
    else
    {
        cout<<"Wrong format: Flag -c was not found"<<endl;
        return -8;
    }
    
    if ((pos=findValidFlag(argv,"-t"))>0)//Find position of the "-t" flag in the arguments
    {
        if (isNum(argv[pos+1])) //Check that next argument is a number
        {
            numThreads=atoi(argv[pos+1]);
            if (numThreads<=0)
            {
                cout<<"numThreads is not a positive number."<<endl;
                return -7;
            }
        }
        else
        {
            cout<<"numThreads is not a positive number."<<endl;
            return -7;
        }
    }
    else
    {
        cout<<"Wrong format: Flag -t was not found"<<endl;
        return -7;
    }
    
    if ((pos=findValidFlag(argv,"-s"))>0)//Find position of the "-s" flag in the arguments
    {
        if (isNum(argv[pos+1]))//Check that next argument is a number
        {
            sizeOfBloom=(unsigned long) (atol(argv[pos+1]));
            if (sizeOfBloom<=0)
            {
                cout<<"sizeOfBloom is not a positive number."<<endl;
                return -4;
            }
        }
        else
        {
            cout<<"sizeOfBloom is not a positive number."<<endl;
            return -4;
        }
    }
    else
    {
        cout<<"Wrong format: Flag -s was not found"<<endl;
        return -4;
    }
    
    if ((pos=findValidFlag(argv,"-i"))>0)//Find position of the "-i" flag in the arguments
    {
        struct stat info;
        if (stat(argv[pos+1],&info)==-1) //Check that next argument is a valid directory that
        {                                //can be accessed
            cout<<argv[pos+1]<<" cannot be accessed."<<endl;
            return -5;
        }
        else if ((info.st_mode & S_IFMT) != S_IFDIR)
        {
            cout<<argv[pos+1]<<" is not a directory."<<endl;
            return -5;
        }
        else
            input_dir=argv[pos+1];
    }
    else
    {
        cout<<"Wrong format: Flag -i was not found"<<endl;
        return -5;
    }
    
    AppTravelMonitor App(numMonitors,numThreads,sockBufferSize,cyclBufferSize,sizeOfBloom,input_dir);//Create and initialize an
                            //object that will handle all the operations of the AppTravelMonitor
    
    if (!App.StartTravelMonitor()) //Start the operation of the AppTravelMonitor(parent process)
    {
        cout<<"An error occured!. Program was terminated!"<<endl;
        return -6;
    }
    
    return 0;
}

int findValidFlag(char **arguments,const char *flag) //Find the position of a flag in the arguments
{
    int flag_pos;
    
    if (!strcmp(flag,arguments[1]))
        flag_pos=1;
    else if (!strcmp(flag,arguments[3]))
        flag_pos=3;
    else if (!strcmp(flag,arguments[5]))
        flag_pos=5;
    else if (!strcmp(flag,arguments[7]))
        flag_pos=7;
    else if (!strcmp(flag,arguments[9]))
        flag_pos=9;
    else if (!strcmp(flag,arguments[11]))
        flag_pos=11;
    else
        return -1;
    
    return flag_pos;
}
