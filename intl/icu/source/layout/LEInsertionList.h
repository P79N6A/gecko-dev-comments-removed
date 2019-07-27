






#ifndef __LEINSERTIONLIST_H
#define __LEINSERTIONLIST_H

#include "LETypes.h"

U_NAMESPACE_BEGIN

struct InsertionRecord;

#ifndef U_HIDE_INTERNAL_API






class U_LAYOUT_API LEInsertionCallback
{
public:
    












    virtual le_bool applyInsertion(le_int32 atPosition, le_int32 count, LEGlyphID newGlyphs[]) = 0;
    
    


     virtual ~LEInsertionCallback();
};
















class LEInsertionList : public UObject
{
public:
    







    LEInsertionList(le_bool rightToLeft);

    


    ~LEInsertionList();

    












    LEGlyphID *insert(le_int32 position, le_int32 count, LEErrorCode &success);

    






    le_int32 getGrowAmount();

    










    le_bool applyInsertions(LEInsertionCallback *callback);

    





    void reset();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:

    




    InsertionRecord *head;

    




    InsertionRecord *tail;

    




    le_int32 growAmount;

    







    le_bool  append;
};
#endif  

U_NAMESPACE_END
#endif

