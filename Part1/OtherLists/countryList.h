#ifndef FILE_COUNTRYLIST_H_INCLUDED
#define FILE_COUNTRYLIST_H_INCLUDED

#include <iostream>
#include <string>
#include "date_citizen_country.h"

class CountryList{//Just stores the countries
private:
    struct CountryListNode{
        country *pcountry;
        CountryListNode *next;
        
        CountryListNode(country *pcountry);
        ~CountryListNode();
    };
    CountryListNode *First;
public:
    CountryList();
    ~CountryList();
        
    void AddCountry(country *c);
    country *FindCountry(std::string name);//Return NULL if not found
};

#endif
