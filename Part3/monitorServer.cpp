#include <cstdio>
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
#include "AppMonitor.h"

using namespace std;

int main(int argc, char *argv[])
{
    int Socket;
    
    if ((Socket=socket(AF_INET,SOCK_STREAM,0))==-1) //Create the socket from the server
    {
        perror("socket");
        return -1;
    }
    
    
    AppMonitor AppMon(Socket,argv,argc);//Create and initialize an object that will
                                   //handle all the operations of the Monitor process
    
    AppMon.StartMonitor(); //After the initialization of the Monitor, start its main operation
    
    close(Socket); //close the listening socket

    return 0;
}
