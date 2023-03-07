#include <iostream>
#include <cstdio>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "AppMonitor.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc==3)
    {
        int monitorSenderNamedPipe;
        int monitorReceiverNamedPipe;
        
        if ((monitorSenderNamedPipe=open(argv[1],O_WRONLY))==-1) //Open write end of the first fifo
        {
            perror("open");
            return -1;
        }
        
        if ((monitorReceiverNamedPipe=open(argv[2],O_RDONLY))==-1) //Open read end of the second fifo
        {
            close(monitorSenderNamedPipe);
            perror("open");
            return -1;
        }
        
        AppMonitor AppMon(monitorReceiverNamedPipe,monitorSenderNamedPipe);//Create and initialize an
                                  //object that will handle all the operations of the Monitor process
        
        AppMon.StartMonitor(); //After the initialization of the Monitor, start its main operation
        
        close(monitorReceiverNamedPipe);
        close(monitorSenderNamedPipe);
    }

    return 0;
}
