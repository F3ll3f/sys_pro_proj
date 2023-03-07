#include <ctime>
#include <cstdlib>
#include "skiplist.h"

using namespace std;

int floor_logN(unsigned long N)//find floor(logN)
{
    int i=0;
    unsigned long powerof2=1; //powerof2=2^i
    
    while (2*powerof2<=N)
    {
        i++;
        powerof2*=2;
    }
    return i;
}

SkipList::SkipListNode::SkipListNode()
{
    data=NULL;
    next=NULL;
    prev=NULL;
    up=NULL;
    down=NULL;
}

SkipList::SkipListNode *SkipList::FindNode(long citizenID)
{
    SkipListNode *cur=top_head;
    
    while (cur!=NULL)
    {//Search for citizenID starting from the top level
        while (cur->next!=NULL && cur->next->data->citizenID<=citizenID)
        {//search the nodes in this level until a bigger number is found
         //or all the nodes in this level are checked
            if (cur->next->data->citizenID==citizenID)//if found
                return cur->next;
            else //else move to the next node
            {
                cur=cur->next;
            }
        }
        cur=cur->down;//move one level lower
    }
    
    return NULL;
}

SkipList::SkipList(unsigned long max_expected_elements_num,int N_percent)
:max_height(floor_logN(max_expected_elements_num)),N_percent(N_percent)
{
    top_head=new SkipListNode;
    bottom_head=top_head;
    number_of_levels=1;
    number_of_elements=0;
}

SkipList::~SkipList()
{
    SkipListNode *cur,*temp,*head=top_head;
    
    while (head!=NULL) //For every level
    {                  //Delete all nodes in this level
        cur=head;
        head=head->down;
        while (cur!=NULL)
        {
            temp=cur->next;
            delete cur;
            cur=temp;
        }
    }
}



citizenANDdate SkipList::Search(long citizenID)
{
    SkipListNode *node=FindNode(citizenID);
    
    if (node!=NULL)
    {
        citizenANDdate cd(node->data,node->d);
        return cd;
    }
    
    date zerod(0,0,0);
    citizenANDdate cd(NULL,zerod);
    return cd;
}

bool SkipList::Insert(const citizenANDdate &data)
{
    long cID=data.c->citizenID;
    int i=number_of_levels-1;
    SkipListNode *temp,*cur=top_head;
    SkipListNode **PrevNodes=new SkipListNode *[number_of_levels];
    /*PrevNodes[i] stores the node which has the biggest ID that is smaller or
     equal to "cID" in level i*/
    while (cur!=NULL)
    {//Search for the nodes which have the biggest ID that is smaller or equal to
     //"cID" in each level
        while (cur->next!=NULL && cur->next->data->citizenID<=cID)
        {//search the nodes in this level until a bigger number is found
         //or all the nodes in this level are checked
            if (cur->next->data->citizenID==cID)//if cID is already inserted
            {
                delete[] PrevNodes;
                return false;
            }
            cur=cur->next;
        }
        PrevNodes[i]=cur;
        cur=cur->down;//move one level lower
        i--;
    }
    
    //Add a new node in the bottom level
    temp=PrevNodes[0]->next;
    PrevNodes[0]->next=new SkipListNode;
    PrevNodes[0]->next->next=temp;
    PrevNodes[0]->next->prev=PrevNodes[0];
    if (temp!=NULL)
        temp->prev=PrevNodes[0]->next;
    PrevNodes[0]->next->data=data.c;
    PrevNodes[0]->next->d=data.d;

    
    i=1;
    while (i<max_height && ( (rand()%100)<N_percent ) )
    {//Decide with N_percent% probability if a new node will be created
        if (i<number_of_levels)//if the new node will be created in an existing level
        {
            temp=PrevNodes[i]->next;
            PrevNodes[i]->next=new SkipListNode;
            PrevNodes[i]->next->down=PrevNodes[i-1]->next;
            PrevNodes[i-1]->next->up=PrevNodes[i]->next;
            PrevNodes[i]->next->next=temp;
            PrevNodes[i]->next->prev=PrevNodes[i];
            if (temp!=NULL)
                temp->prev=PrevNodes[i]->next;
            PrevNodes[i]->next->data=data.c;
            PrevNodes[i]->next->d=data.d;
        }
        else //if a new level will be created
        {
            if (i==number_of_levels)//if this is the first new level added
            {
                cur=PrevNodes[number_of_levels-1]->next;
            }
            
            //Create a new node above the previous one
            cur->up=new SkipListNode;
            cur->up->down=cur;
            cur=cur->up;
            cur->data=data.c;
            cur->d=data.d;
            //Create a new head for the new level
            cur->prev=new SkipListNode;
            cur->prev->next=cur;
            cur->prev->down=top_head;
            top_head->up=cur->prev;
            top_head=top_head->up;
        }
        i++;
    }
    if (i>number_of_levels) //If at least a new level was created
        number_of_levels=i;
    number_of_elements++;
    
    delete[] PrevNodes;
    return true;
}

bool SkipList::Insert(citizen *data)
{
    citizenANDdate cd(data);
    return Insert(cd);
}

citizen *SkipList::Delete(long citizenID)
{
    SkipListNode *temp,*cur=FindNode(citizenID);
    citizen *c;
    
    if (cur==NULL)//if this citizen is not in the SkipList
        return NULL;
    c=cur->data;
    
    //Delete all the nodes with citizenID from "cur" and below
    while (cur!=NULL)
    {//Delete "cur" and then move to the node below "cur"
        temp=cur;
        cur->prev->next=cur->next;
        if (cur->next!=NULL)
            cur->next->prev=cur->prev;
        cur=cur->down;
        delete temp;
    }
    
    //Delete all levels that have only heads(except possibly for bottom level)
    while (top_head->next==NULL && top_head->down!=NULL)
    {
        top_head=top_head->down;
        delete top_head->up;
        top_head->up=NULL;
        number_of_levels--;
    }
    number_of_elements--;
    
    return c;
}

void SkipList::DestroyAll()
{
    SkipListNode *cur=bottom_head->next;
    while (cur!=NULL)
    {
        delete cur->data;
        cur=cur->next;
    }
    return;
}

long SkipList::GetElementsNumber()
{
    return number_of_elements;
}

void SkipList::PrintList()
{
    SkipListNode *cur=bottom_head->next;
    while (cur!=NULL)
    {
        cur->data->print();
        cur=cur->next;
    }
    return;
}
