#include "virusList.h"
#include "date_citizen_country.h"

using namespace std;

VirusList::VirusListNode::VirusListNode(std::string virusName, unsigned long bloomSize,
                                        unsigned long MaxExpNum, int hfN, int NpercentCoin)
:virusName(virusName)
{
    next=NULL;
    BF=new BloomFilter(bloomSize,hfN);
    vaccinated_persons=new SkipList(MaxExpNum,NpercentCoin);
    not_vaccinated_persons=new SkipList(MaxExpNum,NpercentCoin);
}

VirusList::VirusListNode::~VirusListNode()
{
    delete BF;
    delete vaccinated_persons;
    delete not_vaccinated_persons;
}

VirusList::VirusListNode *VirusList::GetNode(std::string name)
{
    VirusListNode *cur=first;
    while (cur!=NULL)//look for "name"
    {
        if (!(cur->virusName.compare(name)))
            return cur;
        cur=cur->next;
    }
    return NULL;//if "name" not found
}

VirusList::VirusList(unsigned long bloomsize, int NpercentCoin,
                     unsigned long max_expected_population)
:bloomSize(bloomsize), NpercentCoin(NpercentCoin),
max_expected_population(max_expected_population)
{
    first=NULL;
}

VirusList::~VirusList()
{
    VirusListNode *cur=first;
    
    while (cur!=NULL)
    {
        first=cur->next;
        delete cur;
        cur=first;
    }
}

void VirusList::InsertVirus(std::string name)
{
    VirusListNode *temp=first;
    first=new VirusListNode(name,bloomSize,max_expected_population,16,NpercentCoin);
    first->next=temp;
}

BloomFilter *VirusList::GetBloomFilter(std::string name)
{
    VirusListNode *VLnode=GetNode(name);
    if (VLnode!=NULL)
        return VLnode->BF;
    
    return NULL;
}

SkipList *VirusList::GetVaccinatedSkipList(std::string name)
{
    VirusListNode *VLnode=GetNode(name);
    if (VLnode!=NULL)
        return VLnode->vaccinated_persons;
    
    return NULL;
}

SkipList *VirusList::GetNonVaccinatedSkipList(std::string name)
{
    VirusListNode *VLnode=GetNode(name);
    if (VLnode!=NULL)
        return VLnode->not_vaccinated_persons;
    
    return NULL;
}

void VirusList::PrintVaccStat(int citizenID)
{
    VirusListNode *cur=first;
    int D,M,Y;
    
    while (cur!=NULL)//Check all viruses
    {
        citizenANDdate CD=cur->vaccinated_persons->Search(citizenID);
        if (CD.c==NULL)//if citizen was found
        {
            CD=cur->not_vaccinated_persons->Search(citizenID);
            if (CD.c!=NULL)
            {
                cout<<cur->virusName<<" NO"<<endl;
            }
        }
        else
        {
            CD.d.getYMD(Y,M,D);
            cout<<cur->virusName<<" YES "<<D<<"-"<<M<<"-"<<Y<<endl;
        }
        cur=cur->next;
    }
    return;
}
