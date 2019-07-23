




































#ifndef nsComposerController_h__
#define nsComposerController_h__


class nsIControllerCommandTable;






#define NS_EDITORDOCSTATECONTROLLER_CID \
 { 0x50e95301, 0x17a8, 0x11d4, { 0x9f, 0x7e, 0xdd, 0x53, 0x0d, 0x5f, 0x05, 0x7c } }


#define NS_HTMLEDITORCONTROLLER_CID \
 { 0x62db0002, 0xdbb6, 0x43f4, { 0x8f, 0xb7, 0x9d, 0x25, 0x38, 0xbc, 0x57, 0x47 } }


class nsComposerController
{
public:
  static nsresult RegisterEditorDocStateCommands(nsIControllerCommandTable* inCommandTable);
  static nsresult RegisterHTMLEditorCommands(nsIControllerCommandTable* inCommandTable);
};

#endif 
