




































#ifndef ipcIDList_h__
#define ipcIDList_h__

#include "nsID.h"
#include "ipcList.h"





class ipcIDNode
{
public:
    ipcIDNode(const nsID &id)
        : mID(id)
        { }

    const nsID &Value() const { return mID; }

    PRBool Equals(const nsID &id) const { return mID.Equals(id); }

    class ipcIDNode *mNext;
private:
    nsID mID;
};





class ipcIDList : public ipcList<ipcIDNode>
{
public:
    typedef ipcList<ipcIDNode> Super;

    void Prepend(const nsID &id)
    {
        Super::Prepend(new ipcIDNode(id));
    }

    void Append(const nsID &id)
    {
        Super::Append(new ipcIDNode(id));
    }

    const ipcIDNode *Find(const nsID &id) const
    {
        return FindNode(mHead, id);
    }

    void FindAndDelete(const nsID &id)
    {
        ipcIDNode *node = FindNodeBefore(mHead, id);
        if (node)
            DeleteAfter(node);
        else
            DeleteFirst();
    }

private:
    static NS_HIDDEN_(ipcIDNode *) FindNode      (ipcIDNode *head, const nsID &id);
    static NS_HIDDEN_(ipcIDNode *) FindNodeBefore(ipcIDNode *head, const nsID &id);
};

#endif 
