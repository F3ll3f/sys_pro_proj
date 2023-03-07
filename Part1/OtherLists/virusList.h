#ifndef FILE_VIRUSLIST_H_INCLUDED
#define FILE_VIRUSLIST_H_INCLUDED

#include <iostream>
#include <string>
#include "skiplist.h"
#include "bloomfilter.h"

class VirusList{/*A list of the viruses. For each virus, it also keeps the
                 BloomFilter and the 2 SkipLists associated with this virus.*/
private:
    struct VirusListNode{
        std::string virusName;

        BloomFilter *BF;
        SkipList *vaccinated_persons;
        SkipList *not_vaccinated_persons;
        
        VirusListNode *next;
        
        VirusListNode(std::string virusName, unsigned long bloomSize, unsigned long MaxExpNum,
                      int hfN=16, int NpercentCoin=50);//hfn is the number of hash functions
        ~VirusListNode();
    };
    
    unsigned long bloomSize; //bloomSize of the BloomFilters
    int NpercentCoin; //Percentage in "coin flip" for the SkipLists
    unsigned long max_expected_population;//An estimate of the maximum expected
    //population that will be inserted in each SkipList. It is merely used to
    //define a maximum possible height for the SkipLists
    VirusListNode *first;
    
    VirusListNode *GetNode(std::string name);//Returns Null if virus does not exist
public:
    VirusList(unsigned long bloomsize, int NpercentCoin, unsigned long max_expected_population);
    ~VirusList();
    
    void InsertVirus(std::string name);
    
    BloomFilter *GetBloomFilter(std::string name);//Returns Null if virus does not exist
    SkipList *GetVaccinatedSkipList(std::string name);//Returns Null if virus does not exist
    SkipList *GetNonVaccinatedSkipList(std::string name);//Returns Null if virus does not exist
    
    void PrintVaccStat(int citizenID);//Prints vaccination status for the citizen for all viruses 
};

#endif
