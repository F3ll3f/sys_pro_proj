#ifndef FILE_STATLIST_H_INCLUDED
#define FILE_STATLIST_H_INCLUDED

#include <iostream>
#include <string>
#include "namesList.h"
#include "date_citizen.h"

//Contains the classes ListOfLists, StatList. ListOfLists is used to create
//the StatList which stores the informarion that we need to keep for the requests.

class ListOfLists{//Each LisOfLists is a list of nodes. Each node contains a pointer to a name,
//a date(optional:0-0-0 date if not needed) and a pointer to a LisOfLists(can be NULL(optional)).
private:
    struct ListOfListsNode{
        std::string *name; //If "d" is not 0-0-0 date, name should
        date d;            //be either "Y" or "N"
        ListOfLists *list;
        ListOfListsNode *next;
        
        ListOfListsNode(std::string *name,date d,bool CreateList=true);//If CreateList
        //is true, a new ListOfLists is created in "list". Otherwise, the "list" is NULL.
        ~ListOfListsNode();
    };
    ListOfListsNode *First;
    
    ListOfListsNode *iterator;//Is is used for iteration of the ListOfLists
public:
    ListOfLists();
    ~ListOfLists();
        
    ListOfLists *GetList(std::string name);//Returns NULL if not found
    ListOfLists *GetFirstList();//Get "list" of the first node or NULL if there are no nodes.
    ListOfLists *GetNextList();/*Use it for iteration of the ListOfLists. Always call
    GetFirstList() first. Then GetNextList() returns the "list" of the next node. Returns NULL if
    the iteration has ended. */
    void AddName(std::string *name);//Each new name creates a node with a new LisOfLists in
                                    //"list" and 0-0-0 date in "d".
    void AddDate(std::string *name,date d);//Each new date creates a node with NULL in "list".
                                           //"name" should be "Y" or "N".
    void CountAccRej(date d1,date d2,unsigned long &countAcc,unsigned long &countRej);//Count
    //the "Y"(accepted requests) in the list with a date in [d1,d2], the "N"(rejected requests)
    //with a date in [d1,d2] and increase the counters accordingly(without making them zero first)
};

class StatList{
private:
    NamesList Countries; //A list of the counries destinations that appeared in the requests
    NamesList Viruses; //A list of the viruses that appeared in the requests
    
    ListOfLists requestsStats; /* Keeps the needed info of each request. It is a list of
    virus-nodes. Each virus-node contains a list of countries-nodes. Each country-node contains a
    list of date-nodes. Each date-node contains a date with the string "Y"(represents an accepted
    request for this virus-country) or the string "N"(represents an accepted request for this
    virus-country) */
    
    std::string strYes; //"Y"  Store "Y","N" strings to avoid data duplication
    std::string strNo;  //"N"
public:
    StatList();
    
    void AddRequest(std::string vName,std::string country,date d,bool Accepted);
    void CountAccAndRej(std::string vName,date d1,date d2,std::string country,
          unsigned long &countAcc,unsigned long &countRej);/* Count all the Accepted(countAcc)
          and rejected(countRej) requests to "country", for virus "vName" with a date in [d1,d2]*/
    void CountAccAndRej(std::string vName,date d1,date d2,
        unsigned long &countAcc,unsigned long &countRej); /* Count all the Accepted(countAcc) and rejected(countRej) requests for virus "vName" with a date in [d1,d2]*/
    void CountAccAndRej(unsigned long &countAcc,unsigned long &countRej); /* Count all the
                                        Accepted(countAcc) and rejected(countRej) requests */
};

#endif

