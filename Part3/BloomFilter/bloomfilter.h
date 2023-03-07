#ifndef FILE_BLOOMFILTER_H_INCLUDED
#define FILE_BLOOMFILTER_H_INCLUDED


#include <iostream>
#include <string>

class BloomFilter{
private:
    unsigned char *array;
    const unsigned long M; //Number of bits in "array"
    const int K; //Number of hash functions
public:
    BloomFilter(unsigned long bloomSize, int K=16);
    ~BloomFilter();
    
    void Insert(std::string element);
    bool Search(std::string element);
    char *GetBytesOfBloomFilter(); //Convert the BloomFilter to a char array
    void UpdateBloomFilter(unsigned char *updatedArray);//Replace the array of the
                                            //BloomFilter with an updated one
};

#endif
