




































#ifndef nsEditorController_h__
#define nsEditorController_h__

#define NS_EDITORCONTROLLER_CID \
{ 0x26fb965c, 0x9de6, 0x11d3, { 0xbc, 0xcc, 0x0, 0x60, 0xb0, 0xfc, 0x76, 0xbd } }

class nsIControllerCommandTable;





class nsEditorController 
{
public:
  static nsresult RegisterEditorCommands(nsIControllerCommandTable* inCommandTable);
};

#endif 

