



#ifndef INTCNT_H
#define INTCNT_H

class IntCount
{
public:
    IntCount();
    ~IntCount();
    void clear();
    int countAdd(int index, int increment=1);
    int countGet(int index);
    int getSize();
    int getCount(int pos);
    int getIndex(int pos);

    IntCount(const IntCount&old)
    {
      numInts = old.numInts;
      if (numInts > 0) {
        iPair = new IntPair[numInts];
        for (int i = 0; i < numInts; i++) {
          iPair[i] = old.iPair[i];
        }
      } else {
        iPair = nullptr;
      }
    }
private:

    int    numInts;
    struct IntPair{int idx; int cnt;} *iPair;
};

#endif
