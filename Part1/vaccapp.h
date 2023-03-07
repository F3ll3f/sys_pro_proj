#ifndef FILE_VACCAPP_H_INCLUDED
#define FILE_VACCAPP_H_INCLUDED

#include <iostream>
#include <string>
#include "bloomfilter.h"
#include "skiplist.h"
#include "virusList.h"
#include "countryList.h"
#include "date_citizen_country.h"



class VaccApp{//An object which contains all structures and methods that the vaccineMonitor needs
private:
    CountryList CList;//The list of the countries
    VirusList VList;//VList includes 2 SkipLists and 1 Bloomfilter for each virus
    SkipList CitizenList;//A SkipList with all the citizens
    
    citizen *CreateNewCitizen(int ID,std::string fName,std::string lName,
                              std::string cName,int age); //Create a new citizen
                                                //and add him to the CitizenList
public:
    VaccApp(unsigned long bloomsize, std::string citizenRecordsFile, int NpercentCoin,
            unsigned long max_expected_population);
    ~VaccApp();
    
    void vaccStatBloom(std::string vName, std::string citizenID);
    void vaccStatVirus(std::string vName, std::string citizenID);
    void vaccStat(std::string citizenID);
    
    void populationStat(std::string cName,std::string vName,date d1,date d2);
    void populationStat(std::string cName,std::string vName);
    void populationStat(std::string vName,date d1,date d2);
    void populationStat(std::string vName);
    
    void populationStatByAge(std::string cName,std::string vName,date d1,date d2);
    void populationStatByAge(std::string cName,std::string vName);
    void populationStatByAge(std::string vName,date d1,date d2);
    void populationStatByAge(std::string vName);
    
    citizenANDdate insertCitizenRec(std::string citizenID,std::string fName,
    std::string lName,std::string cName,int age,std::string vName, date d); //YES
    /*If citizen's personal details are incosistent with a previous record, it fails
     and data member "c" in citizenANDdate points to NULL. Else, it points to the citizen.
     If the citizen is already vaccinated, it returns the previous date of
     vaccination in member "d" in citizenANDdate. Else, the date is 0-0-0 */
    
    bool insertCitizenRec(std::string citizenID,std::string fName,std::string lName,
                          std::string cName,int age,std::string vName);//NO
    /*Returns true if insert was succesful. Else,  it returns false if citizen's
     personal details are incosistent with a previous record or the citizen is
     already in the vaccinated or the non-vaccinated Skiplist for this virus*/
    
    citizenANDdate vaccNow(std::string citizenID,std::string fName,std::string lName,
                  std::string cName,int age,std::string vName);
    //Simply calls the first insertCitizenRec for today's date
    
    void listnonVaccCitizens(std::string vName);
};

#endif
