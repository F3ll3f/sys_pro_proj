#ifndef FILE_STATLIST_H_INCLUDED
#define FILE_STATLIST_H_INCLUDED

#include <iostream>
#include <string>

class StatList{//Stores stats for each country
private:
    struct StatCountryNode{//Stats for a country by age
        std::string country;
        int VacAge0to20;//Store number of vaccinated people in a specific date range(or in
        int VacAge20to40;//general) for each age group
        int VacAge40to60;
        int VacAge60plus;
        int nonVacAge0to20;//Store number of people who were not vaccinated in a specific date
        int nonVacAge20to40;//range(or in general) for each age group. In case of a date range
        int nonVacAge40to60;//this includes both the non-vaccinated people and the vaccinated
        int nonVacAge60plus;//people in a date that is not in the date range mentioned above.
        //Note: Vaccinated and non-vaccinated people in the rest of the file are referred in
        //the same sense as here
        StatCountryNode *next;
        
        StatCountryNode(std::string country,int VacAge0to20=0,int VacAge20to40=0,
                        int VacAge40to60=0,int VacAge60plus=0,int nonVacAge0to20=0,
                        int nonVacAge20to40=0,int nonVacAge40to60=0,int nonVacAge60plus=0);
    };
    StatCountryNode *First;
    
    void AddOneToAge(std::string country,int age,bool IsVaccinated);/*Adds 1 to specific
    Vaccinated(IsVaccinated=True) or nonVaccinated(IsVaccinated=False) age group and country*/
public:
    StatList();
    ~StatList();
        
    void AddOneToVacAge(std::string country,int age);/*Adds 1 to vaccinated people(in a specific
    date range or in general) for a specific age group and a country*/
    void AddOneToNonVacAge(std::string country,int age);/*Adds 1 to non-vaccinated people (in a
    specific date range or in general) for a specific age group and a country*/
    void PrintVacToNonVacByAge();/*Print percentage of vaccinated people(in a specific date
    range or in general) for each age group*/
    void PrintVacToNonVac();/*Print percentage of vaccinated people(in a specific date range or
    in general)*/
};

#endif
