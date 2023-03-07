#ifndef FILE_NAMESLIST_H_INCLUDED
#define FILE_NAMESLIST_H_INCLUDED

#include <iostream>
#include <string>


class NamesList{//Just stores a list with names.
private:
    struct NamesListNode{
        std::string name;
        NamesListNode *next;
        
        NamesListNode(std::string name);
    };
    NamesListNode *First;
public:
    NamesList();
    ~NamesList();
        
    void AddName(std::string name);
    std::string *FindName(std::string name);//Return NULL if not found
};

#endif
