



































#include "nsRegion.h"

void xxxGFXNeverCalled()
{
    nsRegion a;
    nsRect r;
    nsRegion b(r);
    nsRegion c(a);
    c.And(a,b);
    c.And(a,r);
    c.And(r,a);
    c.Or(a,b);
    c.Or(a,r);
    c.Sub(a,b);
    c.Sub(r,a);
    c.IsEmpty();
    c.GetBounds();
    c.GetNumRects();
    c.MoveBy(0,0);
    c.MoveBy(nsPoint());
    c.SetEmpty();
    c.SimplifyInward(0);
    c.SimplifyOutward(0);
    
    
    nsRegionRectIterator i(a);
    i.Next();
    i.Prev();
    i.Reset();
}
