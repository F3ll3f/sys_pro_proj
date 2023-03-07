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
    VirusListNode *iterator;//Is is used for iteration of the VirusList
    int count; //Number of viruses
    VirusListNode *first;
    
    VirusListNode *GetNode(std::string name);//Returns Null if virus does not exist
public:
    VirusList(unsigned long bloomsize, int NpercentCoin, unsigned long max_expected_population);
    ~VirusList();
    
    void InsertVirus(std::string name);
    
    BloomFilter *GetBloomFilter(std::string name);//Returns Null if virus does not exist
    SkipList *GetVaccinatedSkipList(std::string name);//Returns Null if virus does not exist
    SkipList *GetNonVaccinatedSkipList(std::string name);//Returns Null if virus does not exist
    
    int GetCount();//Return number of viruses
    void PrintVaccStat(long citizenID);//Prints vaccination status for the citizen for all viruses
    
    std::string GetFirstBloomFilter(BloomFilter *&Bf);//Get first BloomFilter in the VirusList
                    //and return the virus name or empty string if VirusList is empty
    std::string GetNextBloomFilter(BloomFilter *&Bf);/*Use it for iteration over the VirusList.
    First call GetFirstBloomFilter. After that, each call of GetNextBloomFilter will return
    next virus and the respective BloomFilter until there are no more viruses. Then, it returns
    the empty string. Between these two methods use only public Get* methods of this class. */
    
    std::string GetFirstSkipList(SkipList *&Sl,bool Vaccinated);//Work the same way as
    std::string GetNextSkipList(SkipList *&Sl,bool Vaccinated);/* GetFirstBloomFilter and
    GetNextBloomFilter. If Vaccinated==true, works for the vaccinated skiplists, otherwise for
    the non-vaccinated skiplists. Between these two methods use only public Get* methods of this class. */
};

#endif
