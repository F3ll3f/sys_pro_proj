#ifndef FILE_DATE_CITIZEN_H_INCLUDED
#define FILE_DATE_CITIZEN_H_INCLUDED

#include <iostream>
#include <string>


class date{
private:
    int year;
    int month;
    int day;
public:
    date();//Creates date 0-0-0.
    date(int year,int month,int day);
    date(const std::string &d);//Creates a date from a string date(D-M-Y).
                               //Returns 0-0-0 date, if date is not valid.
    
    bool isZero(); //Returns true only if date is 0-0-0
    void getYMD(int &year,int &month,int &day);
    std::string getDate();
    int compare(const date &d);// Returns 0 if "this" date is d,
                               // <0 "this" date is earlier than d, >0 else
};


struct citizen{
    long citizenID;
    std::string fname;
    std::string lname;
    int age;
    std::string *pcountry;
    
    citizen(long citizenID, std::string fname, std::string lname, int age,
            std::string *pcountry);
    bool isTheSame(long citizenID, std::string fname, std::string lname,
                   int age, std::string cname);/*Returns true if ID,
    fname,lname, age, country are the same. Else, it returns false.*/
    void print();
};

struct citizenANDdate{//An object that simply combines citizen and date data
    citizen *c;
    date d;
    
    citizenANDdate(citizen *c);//Date becomes 0-0-0(Used if date doesn't matter)
    citizenANDdate(citizen *c,const date &d);
};
#endif
