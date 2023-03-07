#ifndef FILE_SKIPLIST_H_INCLUDED
#define FILE_SKIPLIST_H_INCLUDED

#include <iostream>
#include <string>
#include "date_citizen.h"
#include "statList.h"


class SkipList{
private:
    int max_height;//maximum possible height of the skiplist
    int N_percent;//N_percent% is the probability that determines if
                  //a node will be created in the next level
    int number_of_levels;
    long number_of_elements;//Number of citizens in skiplist
    
    struct SkipListNode{
        citizen *data;//Keeps a pointer to the citizen. SkipList finds the citizenID
                      //from this pointer in order to make the comparisons
                      //When data=NULL, SkipListNode is a head
        date d;/*d is used to store the date of vaccination, in case of a vaccinated
        person SkipList. If a date is not needed for this SkipList,d stores date 0-0-0*/
        
        
        SkipListNode *next; //next node in the same level
        SkipListNode *prev; //previous node in the same level
        SkipListNode *up; //node with the same data 1 level higher
        SkipListNode *down; //node with the same data 1 level lower
        
        SkipListNode();
    };
    
    SkipListNode *top_head; //Head node at the highest level
    SkipListNode *bottom_head; //Head node at the lowest level
    
    SkipListNode *FindNode(long citizenID);/*Find a node which contains "citizenID"
    (in that case, returns the node in the highest possible level). If there is no
    such node, it returns NULL*/
public:
    SkipList(unsigned long max_expected_elements_num,int N_percent=50);
    ~SkipList();
    
    citizenANDdate Search(long citizenID); //Returns the citizen and the relative date(vaccination
    //date in case of the vaccinated SkipList). Returns NULL in field "c" and date 0-0-0 if this
    //citizen was not found
    bool Insert(citizen *data); //Returns false if citizen is already included, else true
    bool Insert(const citizenANDdate &data); //Returns false if citizen is already
                                             //included, else true
    citizen *Delete(long citizenID);//Return NULL if this citizen was not found
    void DestroyAll();/*Destroys(from memory) all citizens(data) in the SkipList*/

    long GetElementsNumber();//Returns the number of citizens in the SkipList
    void PrintList();//Prints all citizens in the SkipList
};

#endif
