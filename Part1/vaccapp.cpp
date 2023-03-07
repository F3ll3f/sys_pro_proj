#include <iostream>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "vaccapp.h"
#include "helpfulStringFunctions.h"

using namespace std;

date get_today()//get toaday's date
{
    time_t T=time(NULL);
    struct tm *TM=localtime(&T);
    date d(1900+TM->tm_year,1+TM->tm_mon,TM->tm_mday);
    return d;
}

citizen *VaccApp::CreateNewCitizen(int ID,std::string fName,std::string lName,
                          std::string cName,int age)
{
    country *pcountry=CList.FindCountry(cName);

    if (pcountry==NULL)//Check if country exists
    { //If not, create one
        pcountry=new country(cName);
        CList.AddCountry(pcountry);
    }
    //Create new citizen
    citizen *pcitizen=new citizen(ID,fName,lName,age,pcountry);
    CitizenList.Insert(pcitizen);
    return pcitizen;
}


VaccApp::VaccApp(unsigned long bloomsize,string citizenRecordsFile, int NpercentCoin,
                 unsigned long max_expected_population)
:VList(bloomsize, NpercentCoin, max_expected_population),
CitizenList(max_expected_population,NpercentCoin)
{
    ifstream infile(citizenRecordsFile.c_str());//Open the file
    string line;
    istringstream is;
    int count;
    string cID, vName, d1, d2, country, fName, lName, y_or_n,Age;
    int age,ID;
    citizen *pcitizen;
    SkipList *SL;
    
    srand((unsigned int) time(NULL));//Initialize in order to use it in other methods
    
    while (getline(infile,line))//Read one line each time
    {
        count=count_words(line);
        if (count<7 || count>8)
            cout<<"ERROR IN RECORD "<<line<<endl;
        else
        {
            is.str(line);//line is the content of the stream
            is.clear();//clear flags
            //read from the line
            is>>cID;
            is>>fName;
            is>>lName;
            is>>country;
            is>>Age;
            is>>vName;
            is>>y_or_n;
            if (!y_or_n.compare("YES") && (count==8) && isNum(cID) &&
                onlyLetters(fName) && onlyLetters(lName) && onlyLetters(country) &&
                isAge(Age) && onlyLetNumPlusMostOneDash(vName) )
            {
                is>>d1;
                date date1(d1);
                ID=atoi(cID.c_str());
                age=atoi(Age.c_str());

                SL=VList.GetNonVaccinatedSkipList(vName);
                //Check if virus is already in the virus list
                if (SL==NULL)//if not
                {
                    VList.InsertVirus(vName);//add him
                    pcitizen=NULL;
                }
                else//If it is
                    pcitizen=VList.GetNonVaccinatedSkipList(vName)->Search(ID).c;

                //Check if the citizen is in the non Vaccinated SkipList
                if (pcitizen!=NULL)
                    cout<<"ERROR IN RECORD "<<line<<endl;
                else
                {
                    //Check if the citizen is in the citizen SkipList
                    pcitizen=CitizenList.Search(ID).c;
                    if (pcitizen==NULL) //If he is not there
                    { //Create new citizen and insert him to the Vaccinated Skiplist
                        
                        pcitizen=CreateNewCitizen(ID,fName,lName,country,age);
                        citizenANDdate CD(pcitizen,date1);
                        //Insert him
                        VList.GetVaccinatedSkipList(vName)->Insert(CD);
                        VList.GetBloomFilter(vName)->Insert(cID);
                    }
                    else if (pcitizen->isTheSame(ID,fName,lName,age,country))
                        //If he is there and personal info is correct
                    {   //Insert him in the Vaccinated SkipList
                        citizenANDdate CD(pcitizen,date1);
                        //Check first if he is already vaccinated
                        if (!VList.GetVaccinatedSkipList(vName)->Insert(CD))
                            cout<<"ERROR IN RECORD "<<line<<endl;
                        else
                            VList.GetBloomFilter(vName)->Insert(cID);
                    }
                    else //If personal info is incorrect
                    {
                        cout<<"ERROR IN RECORD "<<line<<endl;
                    }
                }
            }
            else if (!y_or_n.compare("NO"))
            {
                if (count==8 || !isNum(cID) || !onlyLetters(fName) ||
                    !onlyLetters(lName) || !onlyLetters(country) || !isAge(Age) ||
                    !onlyLetNumPlusMostOneDash(vName) )
                    cout<<"ERROR IN RECORD "<<line<<endl;
                else
                {
                    age=atoi(Age.c_str());
                    //Try to insert the record
                    if (!insertCitizenRec(cID,fName,lName,country,age,vName))
                        cout<<"ERROR IN RECORD "<<line<<endl;
                }
            }
            else
                cout<<"ERROR IN RECORD "<<line<<endl;
        }
    }
}

VaccApp::~VaccApp()
{
    CitizenList.DestroyAll();
}

void VaccApp::vaccStatBloom(string vName, string citizenID)
{
    if(VList.GetBloomFilter(vName)==NULL || !VList.GetBloomFilter(vName)->Search(citizenID))
        cout<<"NOT VACCINATED"<<endl;
    else
        cout<<"MAYBE"<<endl;
    return;
}

void VaccApp::vaccStatVirus(string vName, string citizenID)
{
    int ID=atoi(citizenID.c_str());
    SkipList *SL=VList.GetVaccinatedSkipList(vName);
    
    if (SL==NULL)//if virus is not in the virus list
    {
        cout<<"NOT VACCINATED"<<endl;
        return;
    }
    
    citizenANDdate CD=SL->Search(ID);
    if (CD.c==NULL)//if ID is not found
        cout<<"NOT VACCINATED"<<endl;
    else//if found
    {
        int D,M,Y;
        CD.d.getYMD(Y,M,D);
        cout<<"VACCINATED ON "<<D<<"-"<<M<<"-"<<Y<<endl;
    }
    return;
}

void VaccApp::vaccStat(string citizenID)
{
    int ID=atoi(citizenID.c_str());
    VList.PrintVaccStat(ID);
    return;
}

void VaccApp::populationStat(string cName,string vName,date d1,date d2)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    //Update StatList with vaccinated people in [d1,d2]
    SkipL->UpdateStatList(StatL,cName,d1,d2);
    //Update StatList with people not vaccinated in the above interval or not vaccinated at all
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL,cName);
    StatL.PrintVacToNonVac();
    return;
}

void VaccApp::populationStat(string cName,string vName)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    //Update StatList with vaccinated people
    SkipL->UpdateStatList(StatL,cName);
    //Update StatList with not-vaccinated people
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL,cName);
    StatL.PrintVacToNonVac();
    return;
}

void VaccApp::populationStat(string vName,date d1,date d2)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    
    SkipL->UpdateStatList(StatL,d1,d2);
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL);
    StatL.PrintVacToNonVac();
    return;
}

void VaccApp::populationStat(string vName)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }

    SkipL->UpdateStatList(StatL);
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL);
    StatL.PrintVacToNonVac();
    return;
}

void VaccApp::populationStatByAge(string cName,string vName,date d1,date d2)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    //Update with vaccinated people in [d1,d2]
    SkipL->UpdateStatList(StatL,cName,d1,d2);
    //Update StatList with people not vaccinated in the above interval or not vaccinated at all
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL,cName);
    StatL.PrintVacToNonVacByAge();
    return;
}

void VaccApp::populationStatByAge(string cName,string vName)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    
    //Update StatList with vaccinated people
    SkipL->UpdateStatList(StatL,cName);
    //Update StatList with not-vaccinated people
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL,cName);
    StatL.PrintVacToNonVacByAge();
    return;
}

void VaccApp::populationStatByAge(string vName,date d1,date d2)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    
    SkipL->UpdateStatList(StatL,d1,d2);
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL);
    StatL.PrintVacToNonVacByAge();
    return;
}

void VaccApp::populationStatByAge(string vName)
{
    StatList StatL;
    SkipList *SkipL=VList.GetVaccinatedSkipList(vName);
    if (SkipL==NULL)//if there is no record for this virus
    {
        cout<<"There is no record for this virus"<<endl;
        return;
    }
    
    SkipL->UpdateStatList(StatL);
    VList.GetNonVaccinatedSkipList(vName)->UpdateStatList(StatL);
    StatL.PrintVacToNonVacByAge();
    return;
}


citizenANDdate VaccApp::insertCitizenRec(string citizenID,string fName,string lName,
                               string cName,int age,string vName, date d)
{
    int ID=atoi(citizenID.c_str());
    citizen *pcitizen;
    date zeroD(0,0,0);//Zero date
    SkipList *NonVSL=VList.GetNonVaccinatedSkipList(vName);
    
    if (NonVSL==NULL)//if there no record yet for this virus
    {
        VList.InsertVirus(vName);//add him
        pcitizen=NULL;
    }
    else
        pcitizen=NonVSL->Search(ID).c;
    
    if (pcitizen!=NULL)//If citizen is in the non-Vaccinated SkipList
    {
        if (pcitizen->isTheSame(ID,fName,lName,age,cName))
        {//If his personal info is correct
            NonVSL->Delete(ID);
            citizenANDdate CD(pcitizen,d);
            VList.GetVaccinatedSkipList(vName)->Insert(CD);
            VList.GetBloomFilter(vName)->Insert(citizenID);
        }
        else //If his personal info is incosistent
        {
            citizenANDdate cd(NULL,zeroD);
            return cd;
        }
    }
    else //If citizen is not in the non-Vaccinated SkipList
    {
        //Look for the citizen in the citizen-SkipList
        pcitizen=CitizenList.Search(ID).c;
        if (pcitizen==NULL)
        {//If there is no previous record of the citizen
            pcitizen=CreateNewCitizen(ID,fName,lName,cName,age);
            citizenANDdate CD(pcitizen,d);
            VList.GetVaccinatedSkipList(vName)->Insert(CD);
            VList.GetBloomFilter(vName)->Insert(citizenID);
        }
        else if (!pcitizen->isTheSame(ID,fName,lName,age,cName))
        {//If his personal info is incosistent with a record of the citizen
            citizenANDdate cd(NULL,zeroD);
            return cd;
        }
        else
        {//If there is a consistent record of the citizen in citizenSkipList
            citizenANDdate CD(pcitizen,d);
            SkipList *VSL=VList.GetVaccinatedSkipList(vName);
            //Insert him unless he is already vaccinated
            if (!VSL->Insert(CD))
            {
                date prevD=VSL->Search(ID).d;
                citizenANDdate cdprev(pcitizen,prevD);
                return cdprev;
            }
            VList.GetBloomFilter(vName)->Insert(citizenID);
        }
    }
    citizenANDdate cd(pcitizen,zeroD);
    return cd;
}

bool VaccApp::insertCitizenRec(string citizenID,string fName,string lName,
                               string cName,int age,string vName)
{
    int ID=atoi(citizenID.c_str());
    citizen *pcitizen;
    SkipList *VSL=VList.GetVaccinatedSkipList(vName);
    
    if (VSL==NULL)//if there no record yet for this virus
    {
        VList.InsertVirus(vName);//add him
        pcitizen=NULL;
    }
    else
        pcitizen=VSL->Search(ID).c;
    
    if (pcitizen!=NULL)//If the citizen has already vaccinated
        return false;
    else
    {
        //Find citizen in the citizen Skiplist
        pcitizen=CitizenList.Search(ID).c;
        if (pcitizen==NULL)
        {//If there is no record of the citizen
            pcitizen=CreateNewCitizen(ID,fName,lName,cName,age);
            VList.GetNonVaccinatedSkipList(vName)->Insert(pcitizen);
        }
        else if (pcitizen->isTheSame(ID,fName,lName,age,cName))
        {//If there is a consistent record of the citizen in citizenSkipList
            if (!VList.GetNonVaccinatedSkipList(vName)->Insert(pcitizen))
                return false;
        }
        else//If his personal info is incosistent with a record of the citizen
            return false;
    }
    return true;
}

citizenANDdate VaccApp::vaccNow(string citizenID,string fName,string lName, string cName,
                      int age,string vName)
{
    date d=get_today();
    return insertCitizenRec(citizenID,fName,lName,cName,age,vName,d);
}

void VaccApp::listnonVaccCitizens(string vName)
{
    SkipList *NonVSL=VList.GetNonVaccinatedSkipList(vName);
    if (NonVSL==NULL)
    {
        cout<<"-"<<endl;
        return;
    }
    
    NonVSL->PrintList();
    return;
}
