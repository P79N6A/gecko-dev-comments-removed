




#ifndef nsEditorController_h__
#define nsEditorController_h__

#include "nscore.h"

#define NS_EDITORCONTROLLER_CID \
{ 0x26fb965c, 0x9de6, 0x11d3, { 0xbc, 0xcc, 0x0, 0x60, 0xb0, 0xfc, 0x76, 0xbd } }

#define NS_EDITINGCONTROLLER_CID \
{ 0x2c5a5cdd, 0xe742, 0x4dfe, { 0x86, 0xb8, 0x06, 0x93, 0x09, 0xbf, 0x6c, 0x91 } }

class nsIControllerCommandTable;





class nsEditorController 
{
public:
  static nsresult RegisterEditorCommands(nsIControllerCommandTable* inCommandTable);
  static nsresult RegisterEditingCommands(nsIControllerCommandTable* inCommandTable);
};

#endif 

