#ifndef FILE_HELPFULSTRINGFUNCTIONS_H_INCLUDED
#define FILE_HELPFULSTRINGFUNCTIONS_H_INCLUDED

#include <iostream>
#include <string>

bool onlyLetters(std::string str);//Returns true if str contains only letters
bool onlyLetNumPlusMostOneDash(std::string str);//Returns true if str contains only letters and at most one dash
bool isNum(std::string str);//Returns true if str is a number
bool isAge(std::string str);//Returns true if str is an integer in [1,120]
int count_words(std::string str);//Count words in Str
std::string CreateName(std::string name,long num);//Append a number to a string
int compare_strings(const void *first,const void *second);
char *myIntToStr(int num);//Converts num to char* in dynamically allocated memory
char *myLongToStr(long num);//Converts num to char* in dynamically allocated memory
char *myUnLongToStr(unsigned long num);//Converts num to char* in dynamically allocated memory
unsigned long myStrToUnLong(const char *NUM);//Converts NUM to unsigned long

#endif
