








#include "threadtest.h"
#include "unicode/unistr.h"
#include "stdio.h"

class StringThreadTest: public AbstractThreadTest {
public:
                    StringThreadTest();
    virtual        ~StringThreadTest();
    virtual void    check();
    virtual void    runOnce();
            void    makeStringCopies(int recursionCount);

private:
    UnicodeString   *fCleanStrings;
    UnicodeString   *fSourceStrings;
};

StringThreadTest::StringThreadTest() {
    
    
    
    
    fCleanStrings     = new UnicodeString[5];
    fSourceStrings    = new UnicodeString[5];

    fCleanStrings[0]  = "When sorrows come, they come not single spies, but in batallions.";
    fSourceStrings[0] = "When sorrows come, they come not single spies, but in batallions.";
    fCleanStrings[1]  = "Away, you scullion! You rampallion! You fustilarion! I'll tickle your catastrophe!";
    fSourceStrings[1] = "Away, you scullion! You rampallion! You fustilarion! I'll tickle your catastrophe!"; 
    fCleanStrings[2]  = "hot";
    fSourceStrings[2] = "hot"; 
    fCleanStrings[3]  = "";
    fSourceStrings[3] = ""; 
    fCleanStrings[4]  = "Tomorrow, and tomorrow, and tomorrow,\n"
                        "Creeps in this petty pace from day to day\n"
                        "To the last syllable of recorded time;\n"
                        "And all our yesterdays have lighted fools \n"
                        "The way to dusty death. Out, out brief candle!\n"
                        "Life's but a walking shadow, a poor player\n"
                        "That struts and frets his hour upon the stage\n"
                        "And then is heard no more. It is a tale\n"
                        "Told by and idiot, full of sound and fury,\n"
                        "Signifying nothing.\n";
    fSourceStrings[4] = "Tomorrow, and tomorrow, and tomorrow,\n"
                        "Creeps in this petty pace from day to day\n"
                        "To the last syllable of recorded time;\n"
                        "And all our yesterdays have lighted fools \n"
                        "The way to dusty death. Out, out brief candle!\n"
                        "Life's but a walking shadow, a poor player\n"
                        "That struts and frets his hour upon the stage\n"
                        "And then is heard no more. It is a tale\n"
                        "Told by and idiot, full of sound and fury,\n"
                        "Signifying nothing.\n";
};


StringThreadTest::~StringThreadTest() {
    delete [] fCleanStrings;
    delete [] fSourceStrings;
}


void   StringThreadTest::runOnce() {
    makeStringCopies(25);
}

void   StringThreadTest::makeStringCopies(int recursionCount) {
    UnicodeString firstGeneration[5];
    UnicodeString secondGeneration[5];
    UnicodeString thirdGeneration[5];
    UnicodeString fourthGeneration[5];

    
    
    int i;
    for (i=0; i<5; i++) {
         firstGeneration[i]   = fSourceStrings[i];
         secondGeneration[i]  = firstGeneration[i];
         thirdGeneration[i]   = UnicodeString(secondGeneration[i]);
 
         fourthGeneration[i]  = UnicodeString();
         fourthGeneration[i]  = thirdGeneration[i];
    }


    
    
    if (recursionCount > 0) {
        makeStringCopies(recursionCount-1);
    }


    
    for (i=0; i<5; i++) {
        if (firstGeneration[i] !=  fSourceStrings[i]   ||
            firstGeneration[i] !=  secondGeneration[i] ||
            firstGeneration[i] !=  thirdGeneration[i]  ||
            firstGeneration[i] !=  fourthGeneration[i])
        {
            fprintf(stderr, "Error, strings don't compare equal.\n");
        }
    }

};
  

void   StringThreadTest::check() {
    
    
    
    
    
    
    int i;

    for (i=0; i<5; i++) {
        if (fSourceStrings[i].fFlags & UnicodeString::kRefCounted) {
            const UChar *buf = fSourceStrings[i].getBuffer();
            uint32_t refCount = fSourceStrings[i].refCount();
            if (refCount != 1) {
                fprintf(stderr, "\nFailure.  SourceString Ref Count was %d, should be 1.\n", refCount);
            }
        }
    }
};
  





AbstractThreadTest  *createStringTest() {
    return new StringThreadTest();
};
