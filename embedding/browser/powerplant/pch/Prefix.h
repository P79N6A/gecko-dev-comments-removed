






































#define PP_Target_Carbon			1
#define PP_Suppress_Notes_221		1
#define PP_MenuUtils_Option			PP_MenuUtils_AppearanceOnly
#define PP_Uses_Aqua_MenuBar		1


#include <Carbon/Carbon.h>


#include <LAction.h>
#include <LUndoer.h>
#include <UTETextAction.h>
#include <UTEViewTextAction.h>

#include <LModelDirector.h>
#include <LModelObject.h>
#include <LModelProperty.h>
#include <UAppleEventsMgr.h>
#include <UExtractFromAEDesc.h>

#include <LArray.h>
#include <LArrayIterator.h>
#include <LComparator.h>
#include <LRunArray.h>
#include <LVariableArray.h>
#include <TArray.h>
#include <TArrayIterator.h>

#include <LApplication.h>
#include <LCommander.h>
#include <LDocApplication.h>
#include <LDocument.h>
#include <LSingleDoc.h>

#include <LAttachable.h>
#include <LAttachment.h>
#include <LBroadcaster.h>
#include <LDragAndDrop.h>
#include <LDragTask.h>
#include <LEventDispatcher.h>
#include <LListener.h>
#include <LPeriodical.h>
#include <LSharable.h>

#include <LDataStream.h>
#include <LFile.h>
#include <LFileStream.h>
#include <LHandleStream.h>
#include <LStream.h>

#include <LButton.h>
#include <LCaption.h>
#include <LCicnButton.h>
#include <LControl.h>
#include <LDialogBox.h>
#include <LEditField.h>
#include <LFocusBox.h>
#include <LGrafPortView.h>
#include <LListBox.h>
#include <LOffscreenView.h>
#include <LPane.h>
#include <LPicture.h>
#include <LPlaceHolder.h>
#include <LPrintout.h>
#include <LRadioGroupView.h>
#include <LScroller.h>
#include <LStdControl.h>
#include <LTabGroupView.h>
#include <LTableView.h>
#include <LTextEditView.h>
#include <LView.h>
#include <LWindow.h>
#include <UGWorld.h>
#include <UQuickTime.h>

#include <PP_Constants.h>
#include <PP_KeyCodes.h>
#include <PP_Macros.h>
#include <PP_Messages.h>
#include <PP_Prefix.h>
#include <PP_Resources.h>
#include <PP_Types.h>

#include <LClipboard.h>
#include <LFileTypeList.h>
#include <LMenu.h>
#include <LMenuBar.h>
#include <LRadioGroup.h>
#include <LString.h>
#include <LTabGroup.h>
#include <UDesktop.h>

#include <UAttachments.h>
#include <UCursor.h>
#include <UDebugging.h>
#include <UDrawingState.h>
#include <UDrawingUtils.h>
#include <UEnvironment.h>
#include <UException.h>
#include <UKeyFilters.h>
#include <UMemoryMgr.h>
#include <UModalDialogs.h>
#include <UPrinting.h>
#include <UReanimator.h>
#include <URegions.h>
#include <URegistrar.h>
#include <UScrap.h>
#include <UScreenPort.h>
#include <UTextEdit.h>
#include <UTextTraits.h>
#include <UWindows.h>


#include "mozilla-config.h"


#include "PPEmbedConfig.h"


#define POWERPLANT_IS_FRAMEWORK
