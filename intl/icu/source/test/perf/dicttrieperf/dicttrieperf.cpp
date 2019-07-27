























#include <stdio.h>
#include <stdlib.h>
#include "unicode/bytestrie.h"
#include "unicode/bytestriebuilder.h"
#include "unicode/localpointer.h"
#include "unicode/ucharstrie.h"
#include "unicode/ucharstriebuilder.h"
#include "unicode/uperf.h"
#include "unicode/utext.h"
#include "charstr.h"
#include "package.h"
#include "toolutil.h"
#include "ucbuf.h"  
#include "uoptions.h"
#include "uvectr32.h"
#include "cmemory.h" 


class DictionaryTriePerfTest : public UPerfTest {
public:
    DictionaryTriePerfTest(int32_t argc, const char *argv[], UErrorCode &status)
            : UPerfTest(argc, argv, NULL, 0, "", status), numTextLines(0) {
        if(hasFile()) {
            getLines(status);
            for(int32_t i=0; i<numLines; ++i) {
                
                if(lines[i].name[0]>=0x41) {
                    ++numTextLines;
                    
                    int32_t len=lines[i].len;
                    UChar c;
                    while(len>0 && ((c=lines[i].name[len-1])==0xa || c==0xd)) {
                        --len;
                    }
                    lines[i].len=len;
                }
            }
        }
    }

    virtual UPerfFunction *runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

    const char *getSourceDir() const { return sourceDir; }

    UBool hasFile() const { return ucharBuf!=NULL; }
    const ULine *getCachedLines() const { return lines; }
    int32_t getNumLines() const { return numLines; }
    int32_t numTextLines;  
};




class PackageLookup : public UPerfFunction {
protected:
    PackageLookup(const DictionaryTriePerfTest &perf) {
        IcuToolErrorCode errorCode("PackageLookup()");
        CharString filename(perf.getSourceDir(), errorCode);
        int32_t filenameLength=filename.length();
        if(filenameLength>0 && filename[filenameLength-1]!=U_FILE_SEP_CHAR &&
                               filename[filenameLength-1]!=U_FILE_ALT_SEP_CHAR) {
            filename.append(U_FILE_SEP_CHAR, errorCode);
        }
        filename.append(U_ICUDATA_NAME, errorCode);
        filename.append(".dat", errorCode);
        pkg.readPackage(filename.data());
    }

public:
    virtual ~PackageLookup() {}

    

    virtual long getOperationsPerIteration() {
        return pkg.getItemCount();
    }

    

protected:
    Package pkg;
};

struct TOCEntry {
    int32_t nameOffset, dataOffset;
};


static int32_t simpleBinarySearch(const char *s, const char *names, const TOCEntry *toc, int32_t count) {
    int32_t start=0;
    int32_t limit=count;
    int32_t lastNumber=limit;
    for(;;) {
        int32_t number=(start+limit)/2;
        if(lastNumber==number) {  
            return -1;  
        }
        lastNumber=number;
        int32_t cmp=strcmp(s, names+toc[number].nameOffset);
        if(cmp<0) {
            limit=number;
        } else if(cmp>0) {
            start=number;
        } else {  
            return number;
        }
    }
}

class BinarySearchPackageLookup : public PackageLookup {
public:
    BinarySearchPackageLookup(const DictionaryTriePerfTest &perf)
            : PackageLookup(perf) {
        IcuToolErrorCode errorCode("BinarySearchPackageLookup()");
        int32_t count=pkg.getItemCount();
        toc=new TOCEntry[count];
        for(int32_t i=0; i<count; ++i) {
            toc[i].nameOffset=itemNames.length();
            toc[i].dataOffset=i;  
            
            
            const char *name=pkg.getItem(i)->name;
            itemNames.append("icudt46l/", errorCode);
            itemNames.append(name, strlen(name)+1, errorCode);
        }
        printf("size of item names: %6ld\n", (long)itemNames.length());
        printf("size of TOC:        %6ld\n", (long)(count*8));
        printf("total index size:   %6ld\n", (long)(itemNames.length()+count*8));
    }
    virtual ~BinarySearchPackageLookup() {
        delete[] toc;
    }

    virtual void call(UErrorCode * ) {
        int32_t count=pkg.getItemCount();
        const char *itemNameChars=itemNames.data();
        const char *name=itemNameChars;
        for(int32_t i=0; i<count; ++i) {
            if(simpleBinarySearch(name, itemNameChars, toc, count)<0) {
                fprintf(stderr, "item not found: %s\n", name);
            }
            name=strchr(name, 0)+1;
        }
    }

protected:
    CharString itemNames;
    
    
    
    
    TOCEntry *toc;
};

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif



static int32_t strcmpAfterPrefix(const char *s1, const char *s2, int32_t &prefixLength) {
    int32_t pl=prefixLength;
    s1+=pl;
    s2+=pl;
    int32_t cmp=0;
    for(;;) {
        int32_t c1=(uint8_t)*s1++;
        int32_t c2=(uint8_t)*s2++;
        cmp=c1-c2;
        if(cmp!=0 || c1==0) {  
            break;
        }
        ++pl;  
    }
    prefixLength=pl;
    return cmp;
}

static int32_t prefixBinarySearch(const char *s, const char *names, const TOCEntry *toc, int32_t count) {
    if(count==0) {
        return -1;
    }
    int32_t start=0;
    int32_t limit=count;
    
    
    
    int32_t startPrefixLength=0;
    int32_t limitPrefixLength=0;
    
    
    
    
    if(0==strcmpAfterPrefix(s, names+toc[0].nameOffset, startPrefixLength)) {
        return 0;
    }
    ++start;
    --limit;
    if(0==strcmpAfterPrefix(s, names+toc[limit].nameOffset, limitPrefixLength)) {
        return limit;
    }
    while(start<limit) {
        int32_t i=(start+limit)/2;
        int32_t prefixLength=MIN(startPrefixLength, limitPrefixLength);
        int32_t cmp=strcmpAfterPrefix(s, names+toc[i].nameOffset, prefixLength);
        if(cmp<0) {
            limit=i;
            limitPrefixLength=prefixLength;
        } else if(cmp==0) {
            return i;
        } else {
            start=i+1;
            startPrefixLength=prefixLength;
        }
    }
    return -1;
}

class PrefixBinarySearchPackageLookup : public BinarySearchPackageLookup {
public:
    PrefixBinarySearchPackageLookup(const DictionaryTriePerfTest &perf)
            : BinarySearchPackageLookup(perf) {}

    virtual void call(UErrorCode * ) {
        int32_t count=pkg.getItemCount();
        const char *itemNameChars=itemNames.data();
        const char *name=itemNameChars;
        for(int32_t i=0; i<count; ++i) {
            if(prefixBinarySearch(name, itemNameChars, toc, count)<0) {
                fprintf(stderr, "item not found: %s\n", name);
            }
            name=strchr(name, 0)+1;
        }
    }
};

static int32_t bytesTrieLookup(const char *s, const char *nameTrieBytes) {
    BytesTrie trie(nameTrieBytes);
    if(USTRINGTRIE_HAS_VALUE(trie.next(s, -1))) {
        return trie.getValue();
    } else {
        return -1;
    }
}

class BytesTriePackageLookup : public PackageLookup {
public:
    BytesTriePackageLookup(const DictionaryTriePerfTest &perf)
            : PackageLookup(perf) {
        IcuToolErrorCode errorCode("BinarySearchPackageLookup()");
        builder=new BytesTrieBuilder(errorCode);
        int32_t count=pkg.getItemCount();
        for(int32_t i=0; i<count; ++i) {
            
            
            
            
            const char *name=pkg.getItem(i)->name;
            int32_t offset=itemNames.length();
            itemNames.append("icudt46l/", errorCode);
            itemNames.append(name, -1, errorCode);
            
            
            
            StringPiece fullName(itemNames.toStringPiece());
            fullName.remove_prefix(offset);
            builder->add(fullName, i, errorCode);
            
            itemNames.append(0, errorCode);
        }
        int32_t length=builder->buildStringPiece(USTRINGTRIE_BUILD_SMALL, errorCode).length();
        printf("size of BytesTrie:   %6ld\n", (long)length);
        
        printf("size of dataOffsets:%6ld\n", (long)((count+1)*4));
        printf("total index size:   %6ld\n", (long)(length+(count+1)*4));
    }
    virtual ~BytesTriePackageLookup() {
        delete builder;
    }

    virtual void call(UErrorCode *pErrorCode) {
        int32_t count=pkg.getItemCount();
        const char *nameTrieBytes=builder->buildStringPiece(USTRINGTRIE_BUILD_SMALL, *pErrorCode).data();
        const char *name=itemNames.data();
        for(int32_t i=0; i<count; ++i) {
            if(bytesTrieLookup(name, nameTrieBytes)<0) {
                fprintf(stderr, "item not found: %s\n", name);
            }
            name=strchr(name, 0)+1;
        }
    }

protected:
    BytesTrieBuilder *builder;
    CharString itemNames;
};





class DictLookup : public UPerfFunction {
public:
    DictLookup(const DictionaryTriePerfTest &perfTest) : perf(perfTest) {}

    virtual long getOperationsPerIteration() {
        return perf.numTextLines;
    }

protected:
    const DictionaryTriePerfTest &perf;
};







static int32_t
ucharsTrieMatches(UCharsTrie &trie,
                  UText *text, int32_t textLimit,
                  int32_t *lengths, int &count, int limit ) {
    UChar32 c=utext_next32(text);
    
    
    
    if(c<0) {
        return 0;
    }
    
    
    UStringTrieResult result=trie.first(c);
    int32_t numChars=1;
    count=0;
    for(;;) {
        if(USTRINGTRIE_HAS_VALUE(result)) {
            if(count<limit) {
                
                lengths[count++]=numChars;  
            }
            if(result==USTRINGTRIE_FINAL_VALUE) {
                break;
            }
        } else if(result==USTRINGTRIE_NO_MATCH) {
            break;
        }
        if(numChars>=textLimit) {
            
            break;
        }
        UChar32 c=utext_next32(text);
        
        
        
        if(c<0) {
            break;
        }
        ++numChars;
        
        
        result=trie.next(c);
    }
#if 0
    
    
    
    if(index!=lastMatch) {
        utext_setNativeIndex(text, lastMatch);
    }
    return lastMatch-start;
    
    
#endif
    return numChars;
}

class UCharsTrieDictLookup : public DictLookup {
public:
    UCharsTrieDictLookup(const DictionaryTriePerfTest &perfTest)
            : DictLookup(perfTest), trie(NULL) {
        IcuToolErrorCode errorCode("UCharsTrieDictLookup()");
        builder=new UCharsTrieBuilder(errorCode);
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            
            if(lines[i].name[0]<0x41) {
                continue;
            }
            builder->add(UnicodeString(FALSE, lines[i].name, lines[i].len), 0, errorCode);
        }
        UnicodeString trieUChars;
        int32_t length=builder->buildUnicodeString(USTRINGTRIE_BUILD_SMALL, trieUChars, errorCode).length();
        printf("size of UCharsTrie:          %6ld bytes\n", (long)length*2);
        trie=builder->build(USTRINGTRIE_BUILD_SMALL, errorCode);
    }

    virtual ~UCharsTrieDictLookup() {
        delete builder;
        delete trie;
    }

protected:
    UCharsTrieBuilder *builder;
    UCharsTrie *trie;
};

class UCharsTrieDictMatches : public UCharsTrieDictLookup {
public:
    UCharsTrieDictMatches(const DictionaryTriePerfTest &perfTest)
            : UCharsTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode *pErrorCode) {
        UText text=UTEXT_INITIALIZER;
        int32_t lengths[20];
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            
            if(lines[i].name[0]<0x41) {
                continue;
            }
            utext_openUChars(&text, lines[i].name, lines[i].len, pErrorCode);
            int32_t count=0;
            ucharsTrieMatches(*trie, &text, lines[i].len,
                              lengths, count, UPRV_LENGTHOF(lengths));
            if(count==0 || lengths[count-1]!=lines[i].len) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

class UCharsTrieDictContains : public UCharsTrieDictLookup {
public:
    UCharsTrieDictContains(const DictionaryTriePerfTest &perfTest)
            : UCharsTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode * ) {
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            
            if(lines[i].name[0]<0x41) {
                continue;
            }
            if(!USTRINGTRIE_HAS_VALUE(trie->reset().next(lines[i].name, lines[i].len))) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

static inline int32_t thaiCharToByte(UChar32 c) {
    if(0xe00<=c && c<=0xefe) {
        return c&0xff;
    } else if(c==0x2e) {
        return 0xff;
    } else {
        return -1;
    }
}

static UBool thaiWordToBytes(const UChar *s, int32_t length,
                             CharString &str, UErrorCode &errorCode) {
    for(int32_t i=0; i<length; ++i) {
        UChar c=s[i];
        int32_t b=thaiCharToByte(c);
        if(b>=0) {
            str.append((char)b, errorCode);
        } else {
            fprintf(stderr, "thaiWordToBytes(): unable to encode U+%04X as a byte\n", c);
            return FALSE;
        }
    }
    return TRUE;
}

class BytesTrieDictLookup : public DictLookup {
public:
    BytesTrieDictLookup(const DictionaryTriePerfTest &perfTest)
            : DictLookup(perfTest), trie(NULL), noDict(FALSE) {
        IcuToolErrorCode errorCode("BytesTrieDictLookup()");
        builder=new BytesTrieBuilder(errorCode);
        CharString str;
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            
            if(lines[i].name[0]<0x41) {
                continue;
            }
            if(!thaiWordToBytes(lines[i].name, lines[i].len, str.clear(), errorCode)) {
                fprintf(stderr, "thaiWordToBytes(): failed for word %ld (0-based)\n", (long)i);
                noDict=TRUE;
                break;
            }
            builder->add(str.toStringPiece(), 0, errorCode);
        }
        if(!noDict) {
            int32_t length=builder->buildStringPiece(USTRINGTRIE_BUILD_SMALL, errorCode).length();
            printf("size of BytesTrie:           %6ld bytes\n", (long)length);
            trie=builder->build(USTRINGTRIE_BUILD_SMALL, errorCode);
        }
    }

    virtual ~BytesTrieDictLookup() {
        delete builder;
        delete trie;
    }

protected:
    BytesTrieBuilder *builder;
    BytesTrie *trie;
    UBool noDict;
};

static int32_t
bytesTrieMatches(BytesTrie &trie,
                 UText *text, int32_t textLimit,
                 int32_t *lengths, int &count, int limit ) {
    UChar32 c=utext_next32(text);
    if(c<0) {
        return 0;
    }
    UStringTrieResult result=trie.first(thaiCharToByte(c));
    int32_t numChars=1;
    count=0;
    for(;;) {
        if(USTRINGTRIE_HAS_VALUE(result)) {
            if(count<limit) {
                
                lengths[count++]=numChars;  
            }
            if(result==USTRINGTRIE_FINAL_VALUE) {
                break;
            }
        } else if(result==USTRINGTRIE_NO_MATCH) {
            break;
        }
        if(numChars>=textLimit) {
            break;
        }
        UChar32 c=utext_next32(text);
        if(c<0) {
            break;
        }
        ++numChars;
        result=trie.next(thaiCharToByte(c));
    }
    return numChars;
}

class BytesTrieDictMatches : public BytesTrieDictLookup {
public:
    BytesTrieDictMatches(const DictionaryTriePerfTest &perfTest)
            : BytesTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode *pErrorCode) {
        if(noDict) {
            return;
        }
        UText text=UTEXT_INITIALIZER;
        int32_t lengths[20];
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            
            if(lines[i].name[0]<0x41) {
                continue;
            }
            utext_openUChars(&text, lines[i].name, lines[i].len, pErrorCode);
            int32_t count=0;
            bytesTrieMatches(*trie, &text, lines[i].len,
                             lengths, count, UPRV_LENGTHOF(lengths));
            if(count==0 || lengths[count-1]!=lines[i].len) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

class BytesTrieDictContains : public BytesTrieDictLookup {
public:
    BytesTrieDictContains(const DictionaryTriePerfTest &perfTest)
            : BytesTrieDictLookup(perfTest) {}

    virtual void call(UErrorCode * ) {
        if(noDict) {
            return;
        }
        const ULine *lines=perf.getCachedLines();
        int32_t numLines=perf.getNumLines();
        for(int32_t i=0; i<numLines; ++i) {
            const UChar *line=lines[i].name;
            
            if(line[0]<0x41) {
                continue;
            }
            UStringTrieResult result=trie->first(thaiCharToByte(line[0]));
            int32_t lineLength=lines[i].len;
            for(int32_t j=1; j<lineLength; ++j) {
                if(!USTRINGTRIE_HAS_NEXT(result)) {
                    fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
                    break;
                }
                result=trie->next(thaiCharToByte(line[j]));
            }
            if(!USTRINGTRIE_HAS_VALUE(result)) {
                fprintf(stderr, "word %ld (0-based) not found\n", (long)i);
            }
        }
    }
};

UPerfFunction *DictionaryTriePerfTest::runIndexedTest(int32_t index, UBool exec,
                                                      const char *&name, char * ) {
    if(hasFile()) {
        switch(index) {
        case 0:
            name="ucharstriematches";
            if(exec) {
                return new UCharsTrieDictMatches(*this);
            }
            break;
        case 1:
            name="ucharstriecontains";
            if(exec) {
                return new UCharsTrieDictContains(*this);
            }
            break;
        case 2:
            name="bytestriematches";
            if(exec) {
                return new BytesTrieDictMatches(*this);
            }
            break;
        case 3:
            name="bytestriecontains";
            if(exec) {
                return new BytesTrieDictContains(*this);
            }
            break;
        default:
            name="";
            break;
        }
    } else {
        if(index==0 && exec) {
            puts("Running BytesTrie perf tests on the .dat package file from the --sourcedir.\n"
                 "For UCharsTrie perf tests on a dictionary text file, specify the -f or --file-name.\n");
        }
        switch(index) {
        case 0:
            name="simplebinarysearch";
            if(exec) {
                return new BinarySearchPackageLookup(*this);
            }
            break;
        case 1:
            name="prefixbinarysearch";
            if(exec) {
                return new PrefixBinarySearchPackageLookup(*this);
            }
            break;
        case 2:
            name="bytestrie";
            if(exec) {
                return new BytesTriePackageLookup(*this);
            }
            break;
        default:
            name="";
            break;
        }
    }
    return NULL;
}

int main(int argc, const char *argv[]) {
    IcuToolErrorCode errorCode("dicttrieperf main()");
    DictionaryTriePerfTest test(argc, argv, errorCode);
    if(errorCode.isFailure()) {
        fprintf(stderr, "DictionaryTriePerfTest() failed: %s\n", errorCode.errorName());
        test.usage();
        return errorCode.reset();
    }
    if(!test.run()) {
        fprintf(stderr, "FAILED: Tests could not be run, please check the arguments.\n");
        return -1;
    }
    return 0;
}
