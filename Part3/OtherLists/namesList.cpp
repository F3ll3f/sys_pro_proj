#include "namesList.h"


using namespace std;

NamesList::NamesListNode::NamesListNode(string name)
:name(name)
{
    next=NULL;
}


NamesList::NamesList()
{
    First=NULL;
}

NamesList::~NamesList()
{
    NamesListNode *cur=First;
    while (cur!=NULL)
    {
        First=cur->next;
        delete cur;
        cur=First;
    }
}

void NamesList::AddName(string name)
{
    NamesListNode *temp=First;
    First=new NamesListNode(name);
    First->next=temp;
}


string *NamesList::FindName(string name)
{
    NamesListNode *cur=First;
    while (cur!=NULL) //Check each node
    {
        if ((cur->name)==name )
            return &(cur->name);
        else
            cur=cur->next;
    }
    return NULL;
}


