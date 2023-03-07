#include "bloomfilter.h"

using namespace std;

unsigned long djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

unsigned long sdbm(const char *str) {
    unsigned long hash = 0;
    int c;

    while ((c = *str++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

unsigned long hash_i(const char *str, unsigned int i) {
    return djb2(str) + i*sdbm(str) + i*i;
}

BloomFilter::BloomFilter(unsigned long bloomSize, int K):
M(bloomSize*8),K(K)
{
    unsigned long i;
    array=new unsigned char[bloomSize];
    
    for (i=0; i<bloomSize; i++)
    {
        array[i]=0;
    }
}

BloomFilter::~BloomFilter()
{
    delete[] array;
}

void BloomFilter::Insert(string element)
{
    int i;
    unsigned long n,byte_ind;
    unsigned char bit_ind;
    for (i=1; i<=K; i++)//for each hash function
    {                   //find the bits to modify
        n=hash_i(element.c_str(),i) %M;
        byte_ind=n/8;
        bit_ind=1<<(7-(n%8));//create mask
        array[byte_ind]|=bit_ind; //modify
    }
    return;
}

bool BloomFilter::Search(string element)
{
    int i;
    unsigned long n,byte_ind;
    unsigned char bit_ind;
    
    for (i=1; i<=K; i++)//for each hash function
    {                   //find the bits nedded to check
        n=hash_i(element.c_str(),i)%M;
        byte_ind=n/8;
        bit_ind=1<<(7-(n%8));//mask
        if ((array[byte_ind]&bit_ind)==0) //if the bit is 0
            return false;
    }
    return true;
}

char *BloomFilter::GetBytesOfBloomFilter()
{
    return ((char *) array);
}

void BloomFilter::UpdateBloomFilter(unsigned char *updatedArray)
{
    unsigned long i;
    for (i=0; i<(M/8); i++) //Replace each byte of the array
    {
        array[i]=updatedArray[i];
    }
    
    return;
}
