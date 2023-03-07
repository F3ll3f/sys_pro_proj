#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "vaccapp.h"
#include "helpfulStringFunctions.h"
#include "date_citizen_country.h"

using namespace std;

int main(int argc, char *argv[])
{
    unsigned long bloomsize;
    string citizenRecordsFile;
    
    if (argc!=5)
    {
        cout<<"Wrong format."<<endl;
        return -1;
    }
    if (!strcmp(argv[1],"-c") && !strcmp(argv[3],"-b"))
    {
        if (isNum(argv[4]))
            bloomsize=atol(argv[4]);
        else //Not a number
        {
            cout<<"Wrong format."<<endl;
            return -3;
        }
        citizenRecordsFile=argv[2];
    }
    else if (!strcmp(argv[3],"-c") && !strcmp(argv[1],"-b"))
    {
        if (isNum(argv[2]))
            bloomsize=atol(argv[2]);
        else //Not a number
        {
            cout<<"Wrong format."<<endl;
            return -3;
        }
        citizenRecordsFile=argv[4];
    }
    else
    {
        cout<<"Wrong format."<<endl;
        return -2;
    }
    
    VaccApp VA(bloomsize,citizenRecordsFile,50,3000000000);
    //Creates an object which handles all the operations needed
    //3 billions is an expected maximum number of citizens, used simply
    //for defining a maximum possible height in SkipLists
    //50% means the "coin" in SkipLists if fair
    
    cout<<"Type command: ";
    string answer;
    getline(cin,answer);//Get a command
    cout<<endl;
    int count=count_words(answer);
    
    istringstream is;
    string command;
    
    is.str(answer);//Set answer as the content of "is" stream
    is.clear();//Clear flags
    is>>command;
    
    string cID, vName, d1, d2, country, fName, lName, y_or_n,Age;
    string var1,var2;
    int age;
    while (command.compare("/exit"))
    {//Find command and check if format is correct. Then call corresponding method.
        if (!command.compare("/vaccineStatusBloom"))
        {
            if (count!=3)
                cout<<"Wrong format for command /vaccineStatusBloom"<<endl;
            else
            {
                is>>cID;
                is>>vName;
                if (onlyLetNumPlusMostOneDash(vName) && isNum(cID))
                    VA.vaccStatBloom(vName,cID);
                else
                    cout<<"Wrong format for command /vaccineStatusBloom"<<endl;
            }
        }
        else if (!command.compare("/vaccineStatus"))
        {
            if (count<2 || count>3)
                cout<<"Wrong format for command /vaccineStatus"<<endl;
            else if (count==3)
            {
                is>>cID;
                is>>vName;
                if (onlyLetNumPlusMostOneDash(vName) && isNum(cID))
                    VA.vaccStatVirus(vName,cID);
                else
                    cout<<"Wrong format for command /vaccineStatus"<<endl;
            }
            else
            {
                is>>cID;
                if (isNum(cID))
                    VA.vaccStat(cID);
                else
                    cout<<"Wrong format for command /vaccineStatus"<<endl;
            }
        }
        else if (!command.compare("/populationStatus") || !command.compare("/popStatusByAge"))
        {
            if (count<2 || count>5)
            {
                if (!command.compare("/populationStatus"))
                    cout<<"Wrong format for command /populationStatus"<<endl;
                else
                    cout<<"Wrong format for command /popStatusByAge"<<endl;
            }
            else if (count==2)
            {
                is>>vName;
                if (onlyLetNumPlusMostOneDash(vName))
                {
                    if (!command.compare("/populationStatus"))
                        VA.populationStat(vName);
                    else
                        VA.populationStatByAge(vName);
                }
                else
                {
                    if (!command.compare("/populationStatus"))
                        cout<<"Wrong format for command /populationStatus"<<endl;
                    else
                        cout<<"Wrong format for command /popStatusByAge"<<endl;
                }
            }
            else if (count==3)
            {
                is>>var1;
                is>>var2;
                date date1(var2);
                if (date1.isZero())
                {
                    if (onlyLetters(var1) && onlyLetNumPlusMostOneDash(var2))
                    {
                        if (!command.compare("/populationStatus"))
                            VA.populationStat(var1,var2);
                        else
                            VA.populationStatByAge(var1,var2);
                    }
                    else
                    {
                        if (!command.compare("/populationStatus"))
                            cout<<"Wrong format for command /populationStatus"<<endl;
                        else
                            cout<<"Wrong format for command /popStatusByAge"<<endl;
                    }
                }
                else
                    cout<<"ERROR"<<endl;
            }
            else if (count==4)
            {
                is>>vName;
                is>>d1;
                is>>d2;
                date date1(d1);
                date date2(d2);
                if (date2.isZero())
                {
                    if (!command.compare("/populationStatus"))
                        cout<<"Wrong format for command /populationStatus"<<endl;
                    else
                        cout<<"Wrong format for command /popStatusByAge"<<endl;
                }
                else if (date1.isZero())
                    cout<<"ERROR"<<endl;
                else
                {
                    if (onlyLetNumPlusMostOneDash(vName))
                    {
                        if (!command.compare("/populationStatus"))
                            VA.populationStat(vName,date1,date2);
                        else
                            VA.populationStatByAge(vName,date1,date2);
                    }
                    else
                    {
                        if (!command.compare("/populationStatus"))
                            cout<<"Wrong format for command /populationStatus"<<endl;
                        else
                            cout<<"Wrong format for command /popStatusByAge"<<endl;
                    }
                }
            }
            else if (count==5)
            {
                is>>country;
                is>>vName;
                is>>d1;
                is>>d2;
                date date1(d1);
                date date2(d2);
                if (date1.isZero() || date2.isZero())
                {
                    if (!command.compare("/populationStatus"))
                        cout<<"Wrong format for command /populationStatus"<<endl;
                    else
                        cout<<"Wrong format for command /popStatusByAge"<<endl;
                }
                else
                {
                    if (onlyLetters(country) && onlyLetNumPlusMostOneDash(vName))
                    {
                        if (!command.compare("/populationStatus"))
                            VA.populationStat(country,vName,date1,date2);
                        else
                            VA.populationStatByAge(country,vName,date1,date2);
                    }
                    else
                    {
                        if (!command.compare("/populationStatus"))
                            cout<<"Wrong format for command /populationStatus"<<endl;
                        else
                            cout<<"Wrong format for command /popStatusByAge"<<endl;
                    }
                }
            }
        }
        else if (!command.compare("/insertCitizenRecord"))
        {
            if (count<8 || count>9)
                cout<<"Wrong format for command /insertCitizenRecord"<<endl;
            else
            {
                is>>cID;
                is>>fName;
                is>>lName;
                is>>country;
                is>>Age;
                is>>vName;
                is>>y_or_n;
                
                if (count==8)
                {
                    if (!y_or_n.compare("NO") && isNum(cID) && onlyLetters(fName) &&
                        onlyLetters(lName) && onlyLetters(country) && isAge(Age) &&
                        onlyLetNumPlusMostOneDash(vName))
                    {
                        age=atoi(Age.c_str());
                        if (!VA.insertCitizenRec(cID,fName,lName,country,age,vName))
                        {
                            cout<<"ERROR: WRONG DATA OF CITIZEN "<<cID<<
                            " OR THERE IS ALREADY A RECORD OF HIM FOR VIRUS "
                            <<vName<<endl;
                        }
                    }
                    else
                        cout<<"Wrong format for command /insertCitizenRecord"<<endl;
                }
                else
                {
                    is>>d1;
                    date date1(d1);
                    if (date1.isZero() || y_or_n.compare("YES") || !isNum(cID) ||
                        !onlyLetters(fName) || !onlyLetters(lName) ||
                        !onlyLetters(country) || !isAge(Age) ||
                        !onlyLetNumPlusMostOneDash(vName) )
                        cout<<"Wrong format for command /insertCitizenRecord"<<endl;
                    else
                    {
                        age=atoi(Age.c_str());
                        citizenANDdate cd(VA.insertCitizenRec
                                          (cID,fName,lName,country,age,vName,d1));
                        if (cd.c==NULL)//incosistent citizen data
                            cout<<"ERROR: WRONG DATA OF CITIZEN "<<cID<<endl;
                        else if (!cd.d.isZero())//Already vaccinated
                        {
                            int D,M,Y;
                            cd.d.getYMD(Y,M,D);
                            cout<<"ERROR: CITIZEN "<<cID<<" ALREADY VACCINATED ON "
                            <<D<<"-"<<M<<"-"<<Y<<endl;
                        }
                    }
                }
            }
        }
        else if (!command.compare("/vaccinateNow"))
        {
            if (count!=7)
                cout<<"Wrong format for command /vaccinateNow"<<endl;
            else
            {
                is>>cID;
                is>>fName;
                is>>lName;
                is>>country;
                is>>Age;
                is>>vName;
                
                if (!isNum(cID) || !onlyLetters(fName) || !onlyLetters(lName) || !onlyLetters(country) || !isAge(Age) || !onlyLetNumPlusMostOneDash(vName))
                    cout<<"Wrong format for command /vaccinateNow"<<endl;
                else
                {
                    age=atoi(Age.c_str());
                    citizenANDdate CD(VA.vaccNow(cID,fName,lName,country,age,vName));
                    if (CD.c==NULL)
                        cout<<"ERROR: WRONG DATA OF CITIZEN "<<cID<<endl;
                    else if (!CD.d.isZero())
                    {
                        int DAY; int MONTH; int YEAR;
                        CD.d.getYMD(YEAR,MONTH,DAY);
                        
                        cout<<"ERROR: CITIZEN "<<cID<<" ALREADY VACCINATED ON "
                        <<DAY<<"-"<<MONTH<<"-"<<YEAR<<endl;
                    }
                }
            }
        }
        else if (!command.compare("/list-nonVaccinated-Persons"))
        {
            if (count!=2)
                cout<<"Wrong format for command /list-nonVaccinated-Persons"<<endl;
            else
            {
                is>>vName;
                if (onlyLetNumPlusMostOneDash(vName))
                    VA.listnonVaccCitizens(vName);
                else
                    cout<<"Wrong format for command /list-nonVaccinated-Persons"<<endl;
            }
        }
        else
        {
            cout<<"Invalid command!"<<endl;
        }
        
        cout<<"Type command: ";
        getline(cin,answer); //Get a new command
        cout<<endl;
        is.str(answer);
        count=count_words(answer);
        is.clear();
        is>>command;
    }
    
    cout<<"Program ended!"<<endl;
    
    return 0;
}
