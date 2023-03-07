#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "messageExchange.h"

bool GetMessage(int NamedPipe,char *buf,unsigned long bufferSize,char *&Message,unsigned long &mesSize)
{
    long bytes_read=0; //Number of bytes read after "read" command
    unsigned long count=0,index=0,j; /*"index" stores the total number of bytes that have been read during this exchange.
                                      "count" calculates the same number too (but earlier)*/
    unsigned long min_c_buf=0; //Minimum of bufferSize and of the size of the remaining message that we want to read.
    mesSize=0; //Stores the size of the message without the first 10 bytes
    
    while (index<10) //Read first 10 bytes that store the size of Message. Read until all 10 bytes have been received.
    {
        //min_c_buf here is the minimum of bufferSize and of the number the first 10 bytes that have not been read yet.
        min_c_buf=((10-count)<bufferSize ? (10-count) : bufferSize);
        //Read at most bufferSize bytes and at most 10-count bytes(rest of the 10 first bytes)
        bytes_read=read(NamedPipe,buf,min_c_buf);
        if (bytes_read!=-1) //If no error occured, add to "count" the actual number of bytes that were read. (If a signal
        {                   // was received during read, bytes_read<min_c_buf is possible.)
            count+=(unsigned long) bytes_read;
        }
        else if ((bytes_read==-1) && (errno!=EINTR)) //If an error was received that was not a signal interruption
        {
            perror("read");
            return false;
        }
        
        for (j=index; j<count; j++) //Decode the part of the message size that was received
        {
            mesSize*=10;
            mesSize+=buf[j-index];
        }
        index=count;
    }
    
    Message=new char[mesSize]; //Allocate memory to put the merged message when all parts are received.

    while (index<(10+mesSize))//Retrieve the rest message. Read until all the rest message has been received.
    {
        //min_c_buf here is the minimum of bufferSize and of the number of bytes remaining to be received
        min_c_buf=((10+mesSize-count)<bufferSize ? (10+mesSize-count) : bufferSize);
        //Read at most bufferSize bytes and at most mesSize-count bytes(rest of message)
        bytes_read=read(NamedPipe,buf,min_c_buf);
        if (bytes_read!=-1)
        {
            count+=(unsigned long) bytes_read; //Add the number of bytes that were actually read
        }
        else if ((bytes_read==-1) && (errno!=EINTR))//If an error was received that was not a signal interruption
        {
            delete[] Message;
            perror("read");
            return false;
        }

        for (j=index; j<count; j++) //Retrieve this part of the message from the buffer
        {
            Message[j-10]=buf[j-index]; //and copy it in the right position.
        }
        index=count;
    }
    return true;
}

bool SendMessage(int NamedPipe,char *buf,unsigned long bufferSize,const char *Message,unsigned long mesSize)
{
    long bytes_written=0; //Number of bytes written after "write" command
    unsigned long i,size_copy=mesSize;
    unsigned long bytes_copied=0,index_sent=0; /*"bytes_copied","index_sent" is the total number of bytes that have been
                                                sent during this exchange.*/
    char *NewMessage=new char[mesSize+10]; //Allocate memory for the new message

    for (i=0;i<10;i++) //First 10 bytes represent the size of the original Message.
    {
        NewMessage[9-i]=(char) (size_copy%10);
        size_copy/=10;
    }

    for (i=10;i<(mesSize+10);i++)//Copy the rest message
    {
        NewMessage[i]=Message[i-10];
    }
    
    while (bytes_copied<(mesSize+10)) //Continue writing in the fifo until the whole message has been sent
    {   //Message will be splitted into parts. Each part has bufferSize bytes(or less for the last part)
        for (i=0;i<bufferSize;i++)//Copy next part to buffer
        {
            if (bytes_copied<(mesSize+10))
            {
                buf[i]=NewMessage[bytes_copied];
                bytes_copied++;
            }
        }

        unsigned long count_bytes=0; //Count the bytes that have been sent of this part of the message
        unsigned long min_c_buf; /*Minimum of bufferSize and of the size of the remaining portion of the part of the                              message(the part that was copied to the buffer) that we want to send.*/
        min_c_buf=(((mesSize+10-index_sent)<(bufferSize-count_bytes))? //(Send the rest messsage if its size is less than
                   (mesSize+10-index_sent):(bufferSize-count_bytes)); // bufferSize, else send the remaining portion of the                                                    part of the message that we want to send now)
        
        while ((count_bytes<bufferSize) && ((mesSize+10)>index_sent))//Continue writing until the whole part that has
        {                                   //been copied to buffer has been sent (or the whole message has been sent)
            
            //Write at most bufferSize-count_bytes(remaing portion of this part) bytes and at most mesSize+10-index_sent
            bytes_written=write(NamedPipe,buf,min_c_buf); // bytes(rest of message)
            
            if ((bytes_written==-1) && (errno==EINTR))//If an interruption of a signal created an error
                bytes_written++;                      //ignore it
            else if (bytes_written==-1)
            {
                delete[] NewMessage;
                perror("write");
                return false;
            }

            count_bytes+=(unsigned long) bytes_written; //Calculate bytes that have been sent from this part of the Message
            index_sent+=(unsigned long) bytes_written; //Calculate bytes that have been sent from the whole Message
            min_c_buf=(((mesSize+10-index_sent)<(bufferSize-count_bytes))?
                        (mesSize+10-index_sent):(bufferSize-count_bytes));
            for (i=0;i<min_c_buf;i++) //Move the rest of this part to the begining of the buffer
            {
                buf[i]=buf[i+((unsigned long) bytes_written)];
            }
        }
    }
    delete[] NewMessage; //Delete the new message
    return true;
}

