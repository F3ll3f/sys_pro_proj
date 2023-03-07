#include "helpfulStringFunctions.h"
#include <cstring>
#include <cstdlib>
#include <sstream>

using namespace std;

bool onlyLetters(string str)
{
    unsigned int i;
    char c;
    
    for (i=0; i<str.length(); i++) //Check charcters one by one
    {
        c=str[i];
        if (!((c>='a' && c<='z') || (c>='A' && c<='Z')))//if found character
            return false; //that is not letter, return false
    }
    return true;
}

bool onlyLetNumPlusMostOneDash(string str)
{
    unsigned int i;
    char c;
    int count_dashes=0;
    
    for (i=0; i<str.length(); i++)//Check charcters one by one
    {
        c=str[i];
        if ( !( (c>='a' && c<='z') || (c>='A' && c<='Z') || c>='0' || c<='9') )
        {
            if (c=='-') //count dashes
                count_dashes++;
            else
                return false;
        }
    }
    if (count_dashes>1)
        return false;
    
    return true;
}

bool isNum(std::string str)
{
    unsigned long i;
    char c;
    
    for (i=0; i<str.length(); i++) //check characters
    {
        c=str[i];
        if (c<'0' || c>'9') //if a character that is not a number is found
            return false;
    }
    
    return true;
}

bool isAge(std::string str)
{
    unsigned int i,age=0;
    char c;
    
    for (i=0; i<str.length(); i++) //check characters
    {
        c=str[i];
        if (c<'0' || c>'9') //if a character that is not a number is found
            return false;
        age=10*age+(c-'0'); //calculate age
    }
    if (age<=0 || age>120) //if age is not in [1,120], return false
        return false;
    
    return true;
}

int count_words(string str)//count number of words in a string
{
    int count=0;
    string ignoreWord;
    istringstream is;
    is.str(str);
    while (!is.eof())
    {
        is>>ignoreWord; //ignore next word
        if (!is.fail()) //if a word is found
            count++; //increase the count
    }
    return count;
}

string CreateName(string name,long num)
{
    ostringstream oss;
    if (!name.empty())
        oss<<name<<num;
    else
        oss<<num;
    return oss.str();
}

int compare_strings(const void *first,const void *second)
{
    return strcmp( *(const char **) first, *(const char **) second);
}

char *myIntToStr(int num)
{
    if (num>=0) //If positive
        return myUnLongToStr((unsigned long) num);
    else //If negative
    {
        unsigned long i;
        char *temp=myUnLongToStr((unsigned long) (-num)); //Get the string of the absolute value
        char *mystr=new char[strlen(temp)+2];
        mystr[0]='-';   //Add the sign
        for (i=1;i<=(strlen(temp)+1);i++) //and copy the rest number
        {
            mystr[i]=temp[i-1];
        }
        delete[] temp;
        return mystr;
    }
    return NULL;
}

char *myLongToStr(long num) //Similar to he above function for long
{
    if (num>=0)
        return myUnLongToStr((unsigned long) num);
    else
    {
        unsigned long i;
        char *temp=myUnLongToStr((unsigned long) (-num));
        char *mystr=new char[strlen(temp)+2];
        mystr[0]='-';
        for (i=1;i<=(strlen(temp)+1);i++)
        {
            mystr[i]=temp[i-1];
        }
        delete[] temp;
        return mystr;
    }
    return NULL;
}

char *myUnLongToStr(unsigned long num)
{
    unsigned long i,count=0,num_copy=num;
    
    while (num_copy!=0) //Count the digits
    {
        count++;
        num_copy/=10;
    }
    
    if (num==0)
        count=1;
    char *mystr=new char[count+1];
    mystr[count]=0; //'\0'
    for (i=0;i<count;i++) //Convert each digit to the respective ascii code
    {
        mystr[count-1-i]=(char) ('0'+(num%10));
        num/=10;
    }
    return mystr;
}

unsigned long myStrToUnLong(const char *NUM)
{
    unsigned long number=0;
    while ((*NUM)!=0) //Convert each ascii code to the respective digit
    {
        number*=10;
        number+=(unsigned long) ((*NUM)-'0');
        NUM++;
    }
    return number;
}
