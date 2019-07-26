




#include "txList.h"

  
 






txList::txList() {
   firstItem  = 0;
   lastItem   = 0;
   itemCount  = 0;
} 





txList::~txList() {
    clear();
} 

nsresult txList::add(void* objPtr)
{
    return insertBefore(objPtr, 0);
} 




int32_t List::getLength() {
   return itemCount;
} 









nsresult txList::insertAfter(void* objPtr, ListItem* refItem)
{
    
    if (!refItem)
        return insertBefore(objPtr, firstItem);
    return insertBefore(objPtr, refItem->nextItem);
} 








nsresult txList::insertBefore(void* objPtr, ListItem* refItem)
{
    ListItem* item = new ListItem;
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    item->objPtr = objPtr;
    item->nextItem = 0;
    item->prevItem = 0;

    
    if (!refItem) {
        
        if (lastItem) {
            lastItem->nextItem = item;
            item->prevItem = lastItem;
        }
        lastItem = item;
        if (!firstItem)
            firstItem = item;
    }
    else {
        
        item->nextItem = refItem;
        item->prevItem = refItem->prevItem;
        refItem->prevItem = item;

        if (item->prevItem)
            item->prevItem->nextItem = item;
        else
            firstItem = item;
    }

    
    ++itemCount;
    
    return NS_OK;
} 

txList::ListItem* txList::remove(ListItem* item) {

    if (!item)
        return item;

    
    if (item->prevItem) {
        item->prevItem->nextItem = item->nextItem;
    }
    
    if (item->nextItem) {
        item->nextItem->prevItem = item->prevItem;
    }

    
    if (item == firstItem)
        firstItem = item->nextItem;
    if (item == lastItem)
        lastItem = item->prevItem;

    
    --itemCount;
    return item;
} 

void txList::clear()
{
    ListItem* item = firstItem;
    while (item) {
        ListItem* tItem = item;
        item = item->nextItem;
        delete tItem;
    }
    firstItem  = 0;
    lastItem   = 0;
    itemCount  = 0;
}

  
 







txListIterator::txListIterator(txList* list) {
   this->list   = list;
   currentItem  = 0;
   atEndOfList  = false;
} 







nsresult txListIterator::addAfter(void* objPtr)
{
    if (currentItem || !atEndOfList)
        return list->insertAfter(objPtr, currentItem);
    return list->insertBefore(objPtr, 0);

} 







nsresult txListIterator::addBefore(void* objPtr)
{
    if (currentItem || atEndOfList)
        return list->insertBefore(objPtr, currentItem);
    return list->insertAfter(objPtr, 0);

} 






bool txListIterator::hasNext() {
    bool hasNext = false;
    if (currentItem)
        hasNext = (currentItem->nextItem != 0);
    else if (!atEndOfList)
        hasNext = (list->firstItem != 0);

    return hasNext;
} 




void* txListIterator::next() {

    void* obj = 0;
    if (currentItem)
        currentItem = currentItem->nextItem;
    else if (!atEndOfList)
        currentItem = list->firstItem;

    if (currentItem)
        obj = currentItem->objPtr;
    else
        atEndOfList = true;

    return obj;
} 




void* txListIterator::previous() {

    void* obj = 0;

    if (currentItem)
        currentItem = currentItem->prevItem;
    else if (atEndOfList)
        currentItem = list->lastItem;
    
    if (currentItem)
        obj = currentItem->objPtr;

    atEndOfList = false;

    return obj;
} 




void* txListIterator::current() {

    if (currentItem)
        return currentItem->objPtr;

    return 0;
} 





void* txListIterator::remove() {

    void* obj = 0;
    if (currentItem) {
        obj = currentItem->objPtr;
        txList::ListItem* item = currentItem;
        previous(); 
        list->remove(item);
        delete item;
    }
    return obj;
} 




void txListIterator::reset() {
   atEndOfList = false;
   currentItem = 0;
} 




void txListIterator::resetToEnd() {
   atEndOfList = true;
   currentItem = 0;
} 
