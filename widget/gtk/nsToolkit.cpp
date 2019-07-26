






#include "nscore.h"  
#include "nsGTKToolkit.h"

nsGTKToolkit* nsGTKToolkit::gToolkit = nullptr;






nsGTKToolkit::nsGTKToolkit()
  : mFocusTimestamp(0)  
{
}





nsGTKToolkit* nsGTKToolkit::GetToolkit()
{
    if (!gToolkit) {
        gToolkit = new nsGTKToolkit();
    }
 
    return gToolkit;
}
