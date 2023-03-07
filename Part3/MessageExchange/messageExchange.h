#include <iostream>

/*Messages are exchanged using a given buffer and following this protocol: A message is
 an array of chars(bytes). In order to be sent, 10 bytes are added in the beggining which
 represent the size of the original message. Specifically, each char(byte) of these 10 bytes
 represents a digit of the size of the message. The new message is splitted so that each
 splitted part can be stored in the buffer, and all parts are sent using the buffer through
 the socket one-by-one. Using the given buffer the receiver receives the 10 first bytes,
 calculates the size of the message and then receives all the parts of the rest message. When
 everything has been received, the parts are merged and the original message is returned. */


bool GetMessage(int Socket,char *buf,unsigned long bufferSize,
                char *&Message,unsigned long &mesSize);/*Using the given buffer, get a message
from a socket which is opened in the "Socket" descriptor. Return false if failed.*/
                                                                              

bool SendMessage(int Socket,char *buf,unsigned long bufferSize,
                 const char *Message,unsigned long mesSize);/*Using the given buffer, send a
message to a socket which is opened in the "Socket" descriptor. Return false if
failed.*/
