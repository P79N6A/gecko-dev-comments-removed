



















#include "unicode/utypes.h"
#include "unicont.h"
#include "cmemory.h" 







struct BMPBitHash : public UObject {
    int64_t keys[0x800];  
    uint16_t values[0x800];
    uint16_t reverse[0x400];
    uint16_t count;
    const int32_t prime=1301;  

    BMPBitHash() : count(0) {
        
        uprv_memset(values, 0xff, sizeof(values));
    }

    



    uint16_t map(int64_t key) {
        int32_t hash=(int32_t)(key>>55)&0x1ff;
        hash^=(int32_t)(key>>44)&0x7ff;
        hash^=(int32_t)(key>>33)&0x7ff;
        hash^=(int32_t)(key>>22)&0x7ff;
        hash^=(int32_t)(key>>11)&0x7ff;
        hash^=(int32_t)key&0x7ff;
        for(;;) {
            if(values[hash]==0xffff) {
                
                keys[hash]=key;
                reverse[count]=hash;
                return values[hash]=count++;
            } else if(keys[hash]==key) {
                
                return values[hash];
            } else {
                
                hash=(hash+prime)&0x7ff;
            }
        }
    }

    uint16_t countKeys() const { return count; }

    



    void invert(int64_t *k) const {
        uint16_t i;

        for(i=0; i<count; ++i) {
            k[i]=keys[reverse[i]];
        }
    }
};

class BitSet : public UObject, public UnicodeContainable {
public:
    BitSet(const UnicodeSet &set, UErrorCode &errorCode) : bits(shortBits), restSet(set.clone()) {
        if(U_FAILURE(errorCode)) {
            return;
        }
        BMPBitHash *bitHash=new BMPBitHash;
        if(bitHash==NULL || restSet==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        UnicodeSetIterator iter(set);
        int64_t b;
        UChar32 start, end;
        int32_t prevIndex, i, j;

        b=0;  
        prevIndex=-1;
        for(;;) {
            if(iter.nextRange() && !iter.isString()) {
                start=iter.getCodepoint();
                end=iter.getCodepointEnd();
            } else {
                start=0x10000;
            }
            i=start>>6;
            if(prevIndex!=i) {
                
                if(prevIndex<0) {
                    prevIndex=0;
                } else {
                    index[prevIndex++]=bitHash->map(b);
                }
                
                if(prevIndex<i) {
                    uint16_t zero=bitHash->map(0);
                    do {
                        index[prevIndex++]=zero;
                    } while(prevIndex<i);
                }
                b=0;
            }
            if(start>0xffff) {
                break;
            }
            b|=~((INT64_C(1)<<(start&0x3f))-1);
            j=end>>6;
            if(i<j) {
                
                index[i++]=bitHash->map(b);
                
                if(i<j) {
                    uint16_t all=bitHash->map(INT64_C(0xffffffffffffffff));
                    do {
                        index[i++]=all;
                    } while(i<j);
                }
                b=INT64_C(0xffffffffffffffff);
            }
            
            b&=(INT64_C(1)<<(end&0x3f))-1;
            prevIndex=j;
        }

        if(bitHash->countKeys()>UPRV_LENGTHOF(shortBits)) {
            bits=(int64_t *)uprv_malloc(bitHash->countKeys()*8);
        }
        if(bits!=NULL) {
            bitHash->invert(bits);
        } else {
            bits=shortBits;
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        latin1Set[0]=(uint32_t)bits[0];
        latin1Set[1]=(uint32_t)(bits[0]>>32);
        latin1Set[2]=(uint32_t)bits[1];
        latin1Set[3]=(uint32_t)(bits[1]>>32);
        latin1Set[4]=(uint32_t)bits[2];
        latin1Set[5]=(uint32_t)(bits[2]>>32);
        latin1Set[6]=(uint32_t)bits[3];
        latin1Set[7]=(uint32_t)(bits[3]>>32);

        restSet.remove(0, 0xffff);
    }

    ~BitSet() {
        if(bits!=shortBits) {
            uprv_free(bits);
        }
        delete restSet;
    }

    UBool contains(UChar32 c) const {
        if((uint32_t)c<=0xff) {
            return (UBool)((latin1Set[c>>5]&((uint32_t)1<<(c&0x1f)))!=0);
        } else if((uint32_t)c<0xffff) {
            return (UBool)((bits[c>>6]&(INT64_C(1)<<(c&0x3f)))!=0);
        } else {
            return restSet->contains(c);
        }
    }

private:
    uint16_t index[0x400];
    int64_t shortBits[32];
    int64_t *bits;

    uint32_t latin1Bits[8];

    UnicodeSet *restSet;
};
