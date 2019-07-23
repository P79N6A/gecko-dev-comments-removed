


































#ifndef INTCNT_H
#define INTCNT_H

class IntCount
{
public:
    IntCount();
    ~IntCount();
    int countAdd(int index, int increment=1);
    int countGet(int index);
    int getSize();
    int getCount(int pos);
    int getIndex(int pos);

private:
    IntCount(const IntCount&); 

    int    numInts;
    struct IntPair{int idx; int cnt;} *iPair;
};

#endif
