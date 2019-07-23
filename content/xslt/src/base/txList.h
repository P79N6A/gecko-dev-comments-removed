





































#ifndef TRANSFRMX_LIST_H
#define TRANSFRMX_LIST_H

#include "txCore.h"

class txListIterator;




class txList : public TxObject {

friend class txListIterator;

public:

    


    txList();

    


    virtual ~txList();

    




    void* get(int index);

    


    PRInt32 getLength();

    


    inline PRBool isEmpty()
    {
        return itemCount == 0;
    }

    


    nsresult insert(int index, void* objPtr);

    


    nsresult add(void* objPtr);

    


    void* remove(void* objPtr);
    
    


    void clear();

protected:

    struct ListItem {
        ListItem* nextItem;
        ListItem* prevItem;
        void* objPtr;
    };

    ListItem* getFirstItem();
    ListItem* getLastItem();

    


    ListItem* remove(ListItem* sItem);

private:
      txList(const txList& aOther); 

      ListItem* firstItem;
      ListItem* lastItem;
      PRInt32 itemCount;

      nsresult insertAfter(void* objPtr, ListItem* sItem);
      nsresult insertBefore(void* objPtr, ListItem* sItem);
};






class txListIterator {

public:
    



    txListIterator(txList* list);

    


    ~txListIterator();

    





    nsresult addAfter(void* objPtr);

    





    nsresult addBefore(void* objPtr);

    




    MBool  hasNext();

    




    MBool  hasPrevious();

    


    void* next();

    


    void* previous();
    
    


    void* current();
    
    


    void* advance(int i);

    



    void* remove();

    


    void reset();

    


    void resetToEnd();

private:

   
   txList::ListItem* currentItem;

   
   txList* list;

   
   MBool atEndOfList;
};

typedef txList List;

#endif
