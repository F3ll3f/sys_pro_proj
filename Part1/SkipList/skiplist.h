#ifndef FILE_SKIPLIST_H_INCLUDED
#define FILE_SKIPLIST_H_INCLUDED

#include <iostream>
#include <string>
#include "date_citizen_country.h"
#include "statList.h"


class SkipList{
private:
    int max_height;//maximum possible height of the skiplist
    int N_percent;//N_percent% is the probability that determines if
                  //a node will be created in the next level
    int number_of_levels;
    int number_of_elements;//Number of citizens in skiplist
    
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
    
    SkipListNode *FindNode(int citizenID);/*Find a node which contains "citizenID"
    (in that case, returns the node in the highest possible level). If there is no
    such node, it returns NULL*/
public:
    SkipList(unsigned long max_expected_elements_num,int N_percent=50);
    ~SkipList();
    
    citizenANDdate Search(int citizenID); //Returns the citizen and the relative date(vaccination
    //date in case of the vaccinated SkipList). Returns NULL in field "c" and date 0-0-0 if this
    //citizen was not found
    bool Insert(citizen *data); //Returns false if citizen is already included, else true
    bool Insert(const citizenANDdate &data); //Returns false if citizen is already
                                             //included, else true
    citizen *Delete(int citizenID);//Return NULL if this citizen was not found
    void DestroyAll();/*Destroys(from memory) all citizens(data) in the SkipList*/

    int GetElementsNumber();//Returns the number of citizens in the SkipList
    void PrintList();//Prints all citizens in the SkipList
    
    void UpdateStatList(StatList &SL,std::string country, date d1, date d2);
    /* Checks every citizen in the SkipList who is from a specific country and depending
     on if he is vaccinated during [d1,d2] and on his age, increaces the suitable field
     in SL */
    
    void UpdateStatList(StatList &SL,date d1, date d2);
    /* Checks every citizen in the SkipList and depending on if he is vaccinated during
     [d1,d2], on his country and on his age, increaces the suitable field in SL */
        
    /*Note: When the 2 methods below are used, this SkipList stores for a virus either
     the set of the vaccinated people or the set of the non-vaccinated people. So,
     the vaccination status of the first citizen shows which set this SkipList inludes.*/
    void UpdateStatList(StatList &SL,std::string country);
    /* Checks every citizen in the SkipList who is from a specific country and depending
     on if he is vaccinated and on his age, increaces the suitable field in SL.*/
    
    void UpdateStatList(StatList &SL);
    /* Checks every citizen in the SkipList and depending on if he is vaccinated, on his
     country and on his age, increaces the suitable field in SL.*/
};

#endif
