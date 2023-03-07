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
