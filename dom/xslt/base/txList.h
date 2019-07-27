




#ifndef TRANSFRMX_LIST_H
#define TRANSFRMX_LIST_H

#include "txCore.h"

class txListIterator;




class txList : public txObject {

friend class txListIterator;

public:

    


    txList();

    


    ~txList();

    


    int32_t getLength();

    


    inline bool isEmpty()
    {
        return itemCount == 0;
    }

    


    nsresult add(void* objPtr);

    


    void clear();

protected:

    struct ListItem {
        ListItem* nextItem;
        ListItem* prevItem;
        void* objPtr;
    };

    


    ListItem* remove(ListItem* sItem);

private:
      txList(const txList& aOther); 

      ListItem* firstItem;
      ListItem* lastItem;
      int32_t itemCount;

      nsresult insertAfter(void* objPtr, ListItem* sItem);
      nsresult insertBefore(void* objPtr, ListItem* sItem);
};






class txListIterator {

public:
    



    explicit txListIterator(txList* list);

    





    nsresult addAfter(void* objPtr);

    





    nsresult addBefore(void* objPtr);

    




    bool  hasNext();

    


    void* next();

    


    void* previous();
    
    


    void* current();
    
    



    void* remove();

    


    void reset();

    


    void resetToEnd();

private:

   
   txList::ListItem* currentItem;

   
   txList* list;

   
   bool atEndOfList;
};

typedef txList List;

#endif
