#include "countryList.h"

using namespace std;

CountryList::CountryListNode::CountryListNode(country *pcountry)
:pcountry(pcountry)
{
    next=NULL;
}

CountryList::CountryListNode::~CountryListNode()
{
    delete pcountry;
}


CountryList::CountryList()
{
    First=NULL;
}

CountryList::~CountryList()
{
    CountryListNode *cur=First;
    while (cur!=NULL)
    {
        First=cur->next;
        delete cur;
        cur=First;
    }
}

void CountryList::AddCountry(country *c)
{
    CountryListNode *temp=First;
    First=new CountryListNode(c);
    First->next=temp;
}

country *CountryList::FindCountry(std::string name)
{
    CountryListNode *cur=First;
    while (cur!=NULL)
    {
        if (!(cur->pcountry->name.compare(name)))
            return cur->pcountry;
        else
            cur=cur->next;
    }
    return NULL;
}
