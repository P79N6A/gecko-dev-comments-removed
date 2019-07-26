








#ifndef SkPostParts_DEFINED
#define SkPostParts_DEFINED

#include "SkDisplayInput.h"

class SkPost;

class SkDataInput: public SkInput {
    DECLARE_MEMBER_INFO(DataInput);
    SkDataInput();
    bool add();
    virtual void dirty();
    virtual SkDisplayable* getParent() const;
    virtual void onEndElement(SkAnimateMaker& );
    virtual bool setParent(SkDisplayable* );
protected:
    SkPost* fParent;
    typedef SkInput INHERITED;
    friend class SkPost;
};

#endif 
