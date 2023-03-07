#include <ctime>
#include "date_citizen.h"
#include "helpfulStringFunctions.h"

using namespace std;

date::date()
:year(0),month(0),day(0)
{}

date::date(int year,int month,int day)
:year(year),month(month),day(day)
{}

date::date(const string &d)
{
    bool validDATE=true;//validDATE becomes false if a check shows that d is not a date
    
    unsigned int pos=0;
    
    if (d.length()<8 || d.length()>10)
        validDATE=false;
    
    if (validDATE)
    {
        //check day
        if (d[1]=='-' && (d[0]<='9') && (d[0]>='1'))//1 digit for day
        {
            day=d[0]-'0';
            pos=2;
        }
        else if (d[2]=='-' && (
                 ( (d[1]<='9') && (d[1]>='0')  && (d[0]<='2') && (d[0]>='0')) ||
                 ( (d[1]<='1') && (d[1]>='0')  && (d[0]=='3') )
                            )          )//2 digits for day
        {
            day=10*(d[0]-'0')+(d[1]-'0');
            pos=3;
        }
        else
        {
            validDATE=false;
        }
        if (day==0)
            validDATE=false;
    }
    if (validDATE)
    {
        //check month
        if (d[pos+1]=='-' && (d[pos]<='9') && (d[pos]>='1'))//1 digit for month
        {
            month=d[pos]-'0';
            pos+=2;
        }
        else if (d[pos+2]=='-' && (
                ( (d[pos+1]<='9') && (d[pos+1]>='1')  && (d[pos]=='0') ) ||
                ( (d[pos+1]<='2') && (d[pos+1]>='0')  && (d[pos]=='1') )
                                   )        )//2 digits for month
        {
            month=10*(d[pos]-'0')+(d[pos+1]-'0');
            pos+=3;
        }
        else
        {
            validDATE=false;
        }
    }
    if (validDATE && d.length()==(pos+4))
    {
        //check year
        if (d[pos]>='1' && d[pos]<='2' && (d[pos+1]<='9') && (d[pos+1]>='0')
            && (d[pos+2]<='9')&& (d[pos+2]>='0') && (d[pos+3]<='9') && (d[pos+3]>='0'))
        {//4 digits for year
            year=1000*(d[pos]-'0')+100*(d[pos+1]-'0')+10*(d[pos+2]-'0')+(d[pos+3]-'0');
        }
        else
        {
            validDATE=false;
        }
    }
    else
        validDATE=false;
    
    if (!validDATE)//not valid date
    {
        year=0; month=0; day=0;
    }
}

bool date::isZero()
{
    return ((year==0)&&(month==0)&&(day==0));
}


void date::getYMD(int &year,int &month,int &day)
{
    year=this->year;
    month=this->month;
    day=this->day;
    return;
}

string date::getDate()
{
    string emptyStr="";
    string fullDate=CreateName(emptyStr,day);
    fullDate.append("-");
    fullDate=CreateName(fullDate,month);
    fullDate.append("-");
    return CreateName(fullDate,year);
}

int date::compare(const date &d)
{
    return 10000*(year-d.year)+100*(month-d.month)+(day-d.day);
}

citizen::citizen(long citizenID,string fname,string lname,int age,string *pcountry)
:citizenID(citizenID),fname(fname),lname(lname),age(age),pcountry(pcountry)
{}

bool citizen::isTheSame(long citizenID, string fname, string lname, int age,
                        string cname)
{
    return ((this->citizenID==citizenID) && !(this->fname.compare(fname)) &&
            !(this->lname.compare(lname)) && (this->age==age)
            && !(this->pcountry->compare(cname)));
}

void citizen::print()
{
    cout<<citizenID<<" "<<fname<<" "<<lname<<" "<<*pcountry<<" "<<age<<endl;
    return;
}


citizenANDdate::citizenANDdate(citizen *c)
:c(c)
{}

citizenANDdate::citizenANDdate(citizen *c,const date &d)
:c(c),d(d)
{}
