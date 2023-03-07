#include <iomanip>
#include "statList.h"

using namespace std;

double CalculatePercentage(int a,int b)//Calculates a/(a+b). Returns 0 if a+b=0.
{
    int sum=a+b;
    if (sum==0)
        return 0;
    
    return ((double) a)/(a+b);
}

StatList::StatCountryNode::StatCountryNode(string country,int VacAge0to20,int VacAge20to40,
                  int VacAge40to60,int VacAge60plus,int nonVacAge0to20,int nonVacAge20to40,
                  int nonVacAge40to60,int nonVacAge60plus)
:country(country),
VacAge0to20(VacAge0to20),VacAge20to40(VacAge20to40),VacAge40to60(VacAge40to60),
VacAge60plus(VacAge60plus),nonVacAge0to20(nonVacAge0to20),nonVacAge20to40(nonVacAge20to40),
nonVacAge40to60(nonVacAge40to60),nonVacAge60plus(nonVacAge60plus)
{
    next=NULL;
}

StatList::StatList()
{
    First=NULL;
}

StatList::~StatList()
{
    StatCountryNode *cur=First;
    while (cur!=NULL)
    {
        First=cur->next;
        delete cur;
        cur=First;
    }
}

void StatList::AddOneToAge(string country,int age,bool IsVaccinated)
{
    StatCountryNode *cur=First;
    while (cur!=NULL)//Search for the country
    {
        if (!cur->country.compare(country))
        { //Add one to the suitable group
            if (IsVaccinated)
            {
                if (age<20)
                    cur->VacAge0to20++;
                else if (age<40)
                    cur->VacAge20to40++;
                else if (age<60)
                    cur->VacAge40to60++;
                else
                    cur->VacAge60plus++;
            }
            else
            {
                if (age<20)
                    cur->nonVacAge0to20++;
                else if (age<40)
                    cur->nonVacAge20to40++;
                else if (age<60)
                    cur->nonVacAge40to60++;
                else
                    cur->nonVacAge60plus++;
            }
            return;
        }
        cur=cur->next;
    }
    
    //If country not found
    //Create a new StatCountryNode with value 1 at the suitable age group
    cur=First;
    if (IsVaccinated)
    {
        if (age<20)
            First=new StatCountryNode(country,1);
        else if (age<40)
            First=new StatCountryNode(country,0,1);
        else if (age<60)
            First=new StatCountryNode(country,0,0,1);
        else
            First=new StatCountryNode(country,0,0,0,1);
    }
    else
    {
        if (age<20)
            First=new StatCountryNode(country,0,0,0,0,1);
        else if (age<40)
            First=new StatCountryNode(country,0,0,0,0,0,1);
        else if (age<60)
            First=new StatCountryNode(country,0,0,0,0,0,0,1);
        else
            First=new StatCountryNode(country,0,0,0,0,0,0,0,1);
    }
    First->next=cur;
    
    return;
}

void StatList::AddOneToVacAge(string country,int age)
{
    AddOneToAge(country,age,true);
    return;
}

void StatList::AddOneToNonVacAge(string country,int age)
{
    AddOneToAge(country,age,false);
    return;
}

void StatList::PrintVacToNonVacByAge()
{
    StatCountryNode *cur=First;
    double percentage;
    
    while (cur!=NULL)//For each country in the list
    {
        cout<<cur->country<<endl;
        cout<<std::fixed;

        percentage=CalculatePercentage(cur->VacAge0to20,cur->nonVacAge0to20);
        cout<<"0-20 "<<cur->VacAge0to20<<" "<<setprecision(2)<<100*percentage<<"%"<<endl;
        
        percentage=CalculatePercentage(cur->VacAge20to40,cur->nonVacAge20to40);
        cout<<"20-40 "<<cur->VacAge20to40<<" "<<setprecision(2)<<100*percentage<<"%"<<endl;
        
        percentage=CalculatePercentage(cur->VacAge40to60,cur->nonVacAge40to60);
        cout<<"40-60 "<<cur->VacAge40to60<<" "<<setprecision(2)<<100*percentage<<"%"<<endl;
        
        percentage=CalculatePercentage(cur->VacAge60plus,cur->nonVacAge60plus);
        cout<<"60+ "<<cur->VacAge60plus<<" "<<setprecision(2)<<100*percentage<<"%"<<endl;
        
        cur=cur->next;
        cout<<endl;
    }
    return;
}

void StatList::PrintVacToNonVac()
{
    StatCountryNode *cur=First;
    int VacAllAges, nonVacAllAges;
    double percentage;
    while (cur!=NULL)//For each country in the list
    {
        //Sum all age groups
        VacAllAges=cur->VacAge0to20+cur->VacAge20to40+
        cur->VacAge40to60+cur->VacAge60plus;
        
        nonVacAllAges=cur->nonVacAge0to20+cur->nonVacAge20to40+
        cur->nonVacAge40to60+cur->nonVacAge60plus;
        
        percentage=CalculatePercentage(VacAllAges,nonVacAllAges);
        
        cout<<std::fixed;
        cout<<cur->country<<" "<<VacAllAges<<" "<<setprecision(2)<<100*percentage<<"%"<<endl;

        cur=cur->next;
    }
    return;
}
