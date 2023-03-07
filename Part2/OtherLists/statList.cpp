#include <iomanip>
#include "statList.h"


using namespace std;


ListOfLists::ListOfListsNode::ListOfListsNode(std::string *name,date d,bool CreateList)
:name(name),d(d)
{
    next=NULL;
    if (CreateList)
        list=new ListOfLists;
    else
        list=NULL;
}

ListOfLists::ListOfListsNode::~ListOfListsNode()
{
    if (list!=NULL)
        delete list;
}

ListOfLists::ListOfLists()
{
    First=NULL;
}

ListOfLists::~ListOfLists()
{
    ListOfListsNode *cur=First;
    while (cur!=NULL)
    {
        First=cur->next;
        delete cur;
        cur=First;
    }
}


ListOfLists *ListOfLists::GetList(string name)
{
    ListOfListsNode *cur=First;
    while (cur!=NULL)
    {
        if ((*(cur->name))==name )
            return cur->list;
        else
            cur=cur->next;
    }
    return NULL;
}

ListOfLists *ListOfLists::GetFirstList()
{
    iterator=First; //Initialize iterator
    if (First!=NULL)
        return iterator->list;
    return NULL;
}

ListOfLists *ListOfLists::GetNextList()
{
    if (iterator==NULL)
        return NULL;
    
    iterator=iterator->next; //Move iterator to the next node
    if (iterator!=NULL)
        return iterator->list;
    
    return NULL;
}

void ListOfLists::AddName(string *name)
{
    ListOfListsNode *temp=First;
    date zeroD;
    First=new ListOfListsNode(name,zeroD);
    First->next=temp;
    return;
}

void ListOfLists::AddDate(string *name,date d)
{
    ListOfListsNode *temp=First;
    First=new ListOfListsNode(name,d,false);
    First->next=temp;
    return;
}

void ListOfLists::CountAccRej(date d1,date d2,unsigned long &countAcc,unsigned long &countRej)
{
    ListOfListsNode *cur=First;
    while (cur!=NULL) //Check all nodes
    {
        if ( d1.compare(cur->d)<=0 &&  d2.compare(cur->d)>=0)//If date is in [d1,d2]
        {
            if ((*(cur->name))=="Y") //Depending on the string, increase the
                countAcc++;          //respective counter
            else
                countRej++;
        }
        cur=cur->next;
    }
    return;
}

StatList::StatList():strYes("Y"),strNo("N")
{}


void StatList::AddRequest(string vName,string country,date d,bool Accepted)
{
    ListOfLists *pLL;
    ListOfLists *temp;

    string *strp=Viruses.FindName(vName);
    if (strp==NULL) //If this virus is not in the viruses list yet
    {
        Viruses.AddName(vName); //Add it in the viruses' list
        strp=Viruses.FindName(vName);
        requestsStats.AddName(strp); //And create a node for this virus on the
                                     //requests list(requestsStats)
    }
    pLL=requestsStats.GetList(vName); //Get the list that this virus points to
    
    strp=Countries.FindName(country); //Check if this country is already in the
                                      //countries' list
    if (strp==NULL) //If not, add it
    {
        Countries.AddName(country);
        strp=Countries.FindName(country);
    }
    
    temp=pLL->GetList(country); //If this country has not already a node associated
    if (temp==NULL)             //with this virus in the requests list, create one
    {
        pLL->AddName(strp);
        temp=pLL->GetList(country);
    }
    pLL=temp;
    
    if (Accepted) //Add this request with the answer("Y" or "N") in a new node
        pLL->AddDate(&strYes,d); //associated with this virus and country
    else
        pLL->AddDate(&strNo,d);
    
    return;
}

void StatList::CountAccAndRej(string vName,date d1,date d2,string country,unsigned long &countAcc,unsigned long &countRej)
{
    countAcc=0;
    countRej=0;
    ListOfLists *pLL;
    
    pLL=requestsStats.GetList(vName); //Get the ListOfLists associated with this virus
    if (pLL==NULL) //If there is no such list, do not increase any counter.
        return;
    
    pLL=pLL->GetList(country);//Get the ListOfLists associated with this country(for this virus)
    if (pLL==NULL) //If there is no such list, do not increase any counter.
        return;
    
    pLL->CountAccRej(d1,d2,countAcc,countRej); //Count the accepted and rejected requests
                                               //of this ListOfLists for dates in [d1,d2]

    return;
}


void StatList::CountAccAndRej(string vName,date d1,date d2,unsigned long &countAcc,unsigned long &countRej)
{
    countAcc=0;
    countRej=0;
    ListOfLists *pLLc,*pLLd;
    
    pLLc=requestsStats.GetList(vName);//Get the ListOfLists associated with this virus
    if (pLLc==NULL) //If there is no such list, do not increase any counter.
        return;
    
    pLLd=pLLc->GetFirstList();//For every country associated with this virus, get the
    while (pLLd!=NULL)        //respected ListOfLists(includes the dates and the answers)
    {
        pLLd->CountAccRej(d1,d2,countAcc,countRej); //And count and increase the
        pLLd=pLLc->GetNextList();                   //respective counters
    }
    
    return;
}

void StatList::CountAccAndRej(unsigned long &countAcc,unsigned long &countRej)
{
    countAcc=0;
    countRej=0;
    ListOfLists *pLLc,*pLLd;
    date fDate("1-1-1000"); //Find all requests between these dates and increase
    date lDate("31-12-2999"); //the respective counters
    
    pLLc=requestsStats.GetFirstList();
    while (pLLc!=NULL) //For every virus
    {
        pLLd=pLLc->GetFirstList();
        while (pLLd!=NULL) //and for every country associated with this virus
        {
            pLLd->CountAccRej(fDate,lDate,countAcc,countRej); //Count
            pLLd=pLLc->GetNextList();
        }
        pLLc=requestsStats.GetNextList();
    }
    
    return;
}
