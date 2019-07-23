




































#ifndef CLIST_H
#define CLIST_H

#include <stddef.h>








struct CList;

#define OBJECT_PTR_FROM_CLIST(className, listElement) \
            ((char*)listElement - offsetof(className, m_link))
                

struct CList {
    CList *next;
    CList *prev;

    CList() {
        next = prev = this;
    }

    ~CList() {
        Remove();
    }
        
    
    
    
    void Append(CList &element) {
        element.next = this;
        element.prev = prev;
        prev->next = &element;
        prev = &element;
    }

    
    
    
    void Add(CList &element) {
        element.next = next;
        element.prev = this;
        next->prev = &element;
        next = &element;
    }

    
    
    
    void AppendToList(CList &list) {
        list.Append(*this);
    }

    
    
    
    void AddToList(CList &list) {
        list.Add(*this);
    }

    
    
    
    void Remove(void) {
        prev->next = next;
        next->prev = prev;
 
        next = prev = this;
    }

    
    
    
    bool IsEmpty(void) {
        return (next == this);
    }
};    

#endif 

