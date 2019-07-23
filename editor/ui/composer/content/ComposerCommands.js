










































var gComposerJSCommandControllerID = 0;



function SetupHTMLEditorCommands()
{
  var commandTable = GetComposerCommandTable();
  if (!commandTable)
    return;
  
  
  SetupTextEditorCommands();

  

  commandTable.registerCommand("cmd_renderedHTMLEnabler", nsDummyHTMLCommand);

  commandTable.registerCommand("cmd_grid",  nsGridCommand);

  commandTable.registerCommand("cmd_listProperties",  nsListPropertiesCommand);
  commandTable.registerCommand("cmd_pageProperties",  nsPagePropertiesCommand);
  commandTable.registerCommand("cmd_colorProperties", nsColorPropertiesCommand);
  commandTable.registerCommand("cmd_advancedProperties", nsAdvancedPropertiesCommand);
  commandTable.registerCommand("cmd_objectProperties",   nsObjectPropertiesCommand);
  commandTable.registerCommand("cmd_removeNamedAnchors", nsRemoveNamedAnchorsCommand);
  commandTable.registerCommand("cmd_editLink",        nsEditLinkCommand);
  
  commandTable.registerCommand("cmd_form",          nsFormCommand);
  commandTable.registerCommand("cmd_inputtag",      nsInputTagCommand);
  commandTable.registerCommand("cmd_inputimage",    nsInputImageCommand);
  commandTable.registerCommand("cmd_textarea",      nsTextAreaCommand);
  commandTable.registerCommand("cmd_select",        nsSelectCommand);
  commandTable.registerCommand("cmd_button",        nsButtonCommand);
  commandTable.registerCommand("cmd_label",         nsLabelCommand);
  commandTable.registerCommand("cmd_fieldset",      nsFieldSetCommand);
  commandTable.registerCommand("cmd_isindex",       nsIsIndexCommand);
  commandTable.registerCommand("cmd_image",         nsImageCommand);
  commandTable.registerCommand("cmd_hline",         nsHLineCommand);
  commandTable.registerCommand("cmd_link",          nsLinkCommand);
  commandTable.registerCommand("cmd_anchor",        nsAnchorCommand);
  commandTable.registerCommand("cmd_insertHTMLWithDialog", nsInsertHTMLWithDialogCommand);
  commandTable.registerCommand("cmd_insertBreak",   nsInsertBreakCommand);
  commandTable.registerCommand("cmd_insertBreakAll",nsInsertBreakAllCommand);

  commandTable.registerCommand("cmd_table",              nsInsertOrEditTableCommand);
  commandTable.registerCommand("cmd_editTable",          nsEditTableCommand);
  commandTable.registerCommand("cmd_SelectTable",        nsSelectTableCommand);
  commandTable.registerCommand("cmd_SelectRow",          nsSelectTableRowCommand);
  commandTable.registerCommand("cmd_SelectColumn",       nsSelectTableColumnCommand);
  commandTable.registerCommand("cmd_SelectCell",         nsSelectTableCellCommand);
  commandTable.registerCommand("cmd_SelectAllCells",     nsSelectAllTableCellsCommand);
  commandTable.registerCommand("cmd_InsertTable",        nsInsertTableCommand);
  commandTable.registerCommand("cmd_InsertRowAbove",     nsInsertTableRowAboveCommand);
  commandTable.registerCommand("cmd_InsertRowBelow",     nsInsertTableRowBelowCommand);
  commandTable.registerCommand("cmd_InsertColumnBefore", nsInsertTableColumnBeforeCommand);
  commandTable.registerCommand("cmd_InsertColumnAfter",  nsInsertTableColumnAfterCommand);
  commandTable.registerCommand("cmd_InsertCellBefore",   nsInsertTableCellBeforeCommand);
  commandTable.registerCommand("cmd_InsertCellAfter",    nsInsertTableCellAfterCommand);
  commandTable.registerCommand("cmd_DeleteTable",        nsDeleteTableCommand);
  commandTable.registerCommand("cmd_DeleteRow",          nsDeleteTableRowCommand);
  commandTable.registerCommand("cmd_DeleteColumn",       nsDeleteTableColumnCommand);
  commandTable.registerCommand("cmd_DeleteCell",         nsDeleteTableCellCommand);
  commandTable.registerCommand("cmd_DeleteCellContents", nsDeleteTableCellContentsCommand);
  commandTable.registerCommand("cmd_JoinTableCells",     nsJoinTableCellsCommand);
  commandTable.registerCommand("cmd_SplitTableCell",     nsSplitTableCellCommand);
  commandTable.registerCommand("cmd_TableOrCellColor",   nsTableOrCellColorCommand);
  commandTable.registerCommand("cmd_NormalizeTable",     nsNormalizeTableCommand);
  commandTable.registerCommand("cmd_smiley",             nsSetSmiley);
  commandTable.registerCommand("cmd_ConvertToTable",     nsConvertToTable);
}

function SetupTextEditorCommands()
{
  var commandTable = GetComposerCommandTable();
  if (!commandTable)
    return;
  
  
  
  commandTable.registerCommand("cmd_find",       nsFindCommand);
  commandTable.registerCommand("cmd_findNext",   nsFindAgainCommand);
  commandTable.registerCommand("cmd_findPrev",   nsFindAgainCommand);
  commandTable.registerCommand("cmd_rewrap",     nsRewrapCommand);
  commandTable.registerCommand("cmd_spelling",   nsSpellingCommand);
  commandTable.registerCommand("cmd_validate",   nsValidateCommand);
  commandTable.registerCommand("cmd_checkLinks", nsCheckLinksCommand);
  commandTable.registerCommand("cmd_insertChars", nsInsertCharsCommand);
}

function SetupComposerWindowCommands()
{
  
  if (gComposerWindowControllerID)
    return;

  
  
  
  
  
  
  

  var windowControllers = window.controllers;

  if (!windowControllers) return;

  var commandTable;
  var composerController;
  var editorController;
  try {
    composerController = Components.classes["@mozilla.org/embedcomp/base-command-controller;1"].createInstance();

    editorController = composerController.QueryInterface(Components.interfaces.nsIControllerContext);
    editorController.init(null); 

    
    var interfaceRequestor = composerController.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    commandTable = interfaceRequestor.getInterface(Components.interfaces.nsIControllerCommandTable);
  }
  catch (e)
  {
    dump("Failed to create composerController\n");
    return;
  }


  if (!commandTable)
  {
    dump("Failed to get interface for nsIControllerCommandManager\n");
    return;
  }

  
  commandTable.registerCommand("cmd_open",           nsOpenCommand);
  commandTable.registerCommand("cmd_save",           nsSaveCommand);
  commandTable.registerCommand("cmd_saveAs",         nsSaveAsCommand);
  commandTable.registerCommand("cmd_exportToText",   nsExportToTextCommand);
  commandTable.registerCommand("cmd_saveAndChangeEncoding",  nsSaveAndChangeEncodingCommand);
  commandTable.registerCommand("cmd_publish",        nsPublishCommand);
  commandTable.registerCommand("cmd_publishAs",      nsPublishAsCommand);
  commandTable.registerCommand("cmd_publishSettings",nsPublishSettingsCommand);
  commandTable.registerCommand("cmd_revert",         nsRevertCommand);
  commandTable.registerCommand("cmd_openRemote",     nsOpenRemoteCommand);
  commandTable.registerCommand("cmd_preview",        nsPreviewCommand);
  commandTable.registerCommand("cmd_editSendPage",   nsSendPageCommand);
  commandTable.registerCommand("cmd_print",          nsPrintCommand);
  commandTable.registerCommand("cmd_printSetup",     nsPrintSetupCommand);
  commandTable.registerCommand("cmd_quit",           nsQuitCommand);
  commandTable.registerCommand("cmd_close",          nsCloseCommand);
  commandTable.registerCommand("cmd_preferences",    nsPreferencesCommand);

  
  if (GetCurrentEditorType() == "html")
  {
    commandTable.registerCommand("cmd_NormalMode",         nsNormalModeCommand);
    commandTable.registerCommand("cmd_AllTagsMode",        nsAllTagsModeCommand);
    commandTable.registerCommand("cmd_HTMLSourceMode",     nsHTMLSourceModeCommand);
    commandTable.registerCommand("cmd_PreviewMode",        nsPreviewModeCommand);
    commandTable.registerCommand("cmd_FinishHTMLSource",   nsFinishHTMLSource);
    commandTable.registerCommand("cmd_CancelHTMLSource",   nsCancelHTMLSource);
    commandTable.registerCommand("cmd_updateStructToolbar", nsUpdateStructToolbarCommand);
  }

  windowControllers.insertControllerAt(0, editorController);

  
  gComposerWindowControllerID = windowControllers.getControllerId(editorController);
}


function GetComposerCommandTable()
{
  var controller;
  if (gComposerJSCommandControllerID)
  {
    try { 
      controller = window.content.controllers.getControllerById(gComposerJSCommandControllerID);
    } catch (e) {}
  }
  if (!controller)
  {
    
    controller = Components.classes["@mozilla.org/embedcomp/base-command-controller;1"].createInstance();

    var editorController = controller.QueryInterface(Components.interfaces.nsIControllerContext);
    editorController.init(null);
    editorController.setCommandContext(GetCurrentEditorElement());
    window.content.controllers.insertControllerAt(0, controller);
  
    
    gComposerJSCommandControllerID = window.content.controllers.getControllerId(controller);
  }

  if (controller)
  {
    var interfaceRequestor = controller.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    return interfaceRequestor.getInterface(Components.interfaces.nsIControllerCommandTable);
  }
  return null;
}


function goUpdateCommandState(command)
{
  try
  {
    var controller = top.document.commandDispatcher.getControllerForCommand(command);
    if (!(controller instanceof Components.interfaces.nsICommandController))
      return;

    var params = newCommandParams();
    if (!params) return;

    controller.getCommandStateWithParams(command, params);

    switch (command)
    {
      case "cmd_bold":
      case "cmd_italic":
      case "cmd_underline":
      case "cmd_var":
      case "cmd_samp":
      case "cmd_code":
      case "cmd_acronym":
      case "cmd_abbr":
      case "cmd_cite":
      case "cmd_strong":
      case "cmd_em":
      case "cmd_superscript":
      case "cmd_subscript":
      case "cmd_strikethrough":
      case "cmd_tt":
      case "cmd_nobreak":
      case "cmd_ul":
      case "cmd_ol":
        pokeStyleUI(command, params.getBooleanValue("state_all"));
        break;

      case "cmd_paragraphState":
      case "cmd_align":
      case "cmd_highlight":
      case "cmd_backgroundColor":
      case "cmd_fontColor":
      case "cmd_fontFace":
      case "cmd_fontSize":
      case "cmd_absPos":
        pokeMultiStateUI(command, params);
        break;

      case "cmd_decreaseZIndex":
      case "cmd_increaseZIndex":
      case "cmd_indent":
      case "cmd_outdent":
      case "cmd_increaseFont":
      case "cmd_decreaseFont":
      case "cmd_removeStyles":
      case "cmd_smiley":
        break;

      default: dump("no update for command: " +command+"\n");
    }
  }
  catch (e) { dump("An error occurred updating the "+command+" command: \n"+e+"\n"); }
}

function goUpdateComposerMenuItems(commandset)
{
  

  for (var i = 0; i < commandset.childNodes.length; i++)
  {
    var commandNode = commandset.childNodes[i];
    var commandID = commandNode.id;
    if (commandID)
    {
      goUpdateCommand(commandID);  
      if (commandNode.hasAttribute("state"))
        goUpdateCommandState(commandID);
    }
  }
}


function goDoCommandParams(command, params)
{
  try
  {
    var controller = top.document.commandDispatcher.getControllerForCommand(command);
    if (controller && controller.isCommandEnabled(command))
    {
      if (controller instanceof Components.interfaces.nsICommandController)
      {
        controller.doCommandWithParams(command, params);

        
        if (params)
          controller.getCommandStateWithParams(command, params);
      }
      else
      {
        controller.doCommand(command);
      }
      ResetStructToolbar();
    }
  }
  catch (e)
  {
    dump("An error occurred executing the "+command+" command\n");
  }
}

function pokeStyleUI(uiID, aDesiredState)
{
 try {
  var commandNode = top.document.getElementById(uiID);
  if (!commandNode)
    return;

  var uiState = ("true" == commandNode.getAttribute("state"));
  if (aDesiredState != uiState)
  {
    var newState;
    if (aDesiredState)
      newState = "true";
    else
      newState = "false";
    commandNode.setAttribute("state", newState);
  }
 } catch(e) { dump("poking UI for "+uiID+" failed: "+e+"\n"); }
}

function doStyleUICommand(cmdStr)
{
  try
  {
    var cmdParams = newCommandParams();
    goDoCommandParams(cmdStr, cmdParams);
    if (cmdParams)
      pokeStyleUI(cmdStr, cmdParams.getBooleanValue("state_all"));

    ResetStructToolbar();
  } catch(e) {}
}

function pokeMultiStateUI(uiID, cmdParams)
{
  try
  {
    var commandNode = document.getElementById(uiID);
    if (!commandNode)
      return;

    var isMixed = cmdParams.getBooleanValue("state_mixed");
    var desiredAttrib;
    if (isMixed)
      desiredAttrib = "mixed";
    else {
      var valuetype = cmdParams.getValueType("state_attribute");
      if (valuetype == Components.interfaces.nsICommandParams.eStringType) {
        desiredAttrib = cmdParams.getCStringValue("state_attribute");      
      } else {
        desiredAttrib = cmdParams.getStringValue("state_attribute");      
      }

    }

    var uiState = commandNode.getAttribute("state");
    if (desiredAttrib != uiState)
    {
      commandNode.setAttribute("state", desiredAttrib);
    }
  } catch(e) {}
}

function doStatefulCommand(commandID, newState)
{
  var commandNode = document.getElementById(commandID);
  if (commandNode)
      commandNode.setAttribute("state", newState);
  gContentWindow.focus();   

  try
  {
    var cmdParams = newCommandParams();
    if (!cmdParams) return;

    cmdParams.setStringValue("state_attribute", newState);
    goDoCommandParams(commandID, cmdParams);

    pokeMultiStateUI(commandID, cmdParams);

    ResetStructToolbar();
  } catch(e) { dump("error thrown in doStatefulCommand: "+e+"\n"); }
}


function PrintObject(obj)
{
  dump("-----" + obj + "------\n");
  var names = "";
  for (var i in obj)
  {
    if (i == "value")
      names += i + ": " + obj.value + "\n";
    else if (i == "id")
      names += i + ": " + obj.id + "\n";
    else
      names += i + "\n";
  }
  
  dump(names + "-----------\n");
}


function PrintNodeID(id)
{
  PrintObject(document.getElementById(id));
}


var nsDummyHTMLCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    dump("Hey, who's calling the dummy command?\n");
  }

};


var nsOpenCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;    
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, GetString("OpenHTMLFile"), nsIFilePicker.modeOpen);

    SetFilePickerDirectory(fp, "html");

    
    
    fp.appendFilters(nsIFilePicker.filterHTML);
    fp.appendFilters(nsIFilePicker.filterText);
    fp.appendFilters(nsIFilePicker.filterAll);

    
    try {
      fp.show();
      
    }
    catch (ex) {
      dump("filePicker.chooseInputFile threw an exception\n");
    }
  
    



    if (fp.file && fp.file.path.length > 0) {
      SaveFilePickerDirectory(fp, "html");
      editPage(fp.fileURL.spec, window, false);
    }
  }
};



var nsUpdateStructToolbarCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    UpdateStructToolbar();
    return true;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},
  doCommand: function(aCommand)  {}
}



var nsSaveCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    
    
    
    try {
      var docUrl = GetDocumentUrl();
      return IsDocumentEditable() &&
        (IsDocumentModified() || IsHTMLSourceChanged() ||
         IsUrlAboutBlank(docUrl) || GetScheme(docUrl) != "file");
    } catch (e) {return false;}
  },
  
  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var result = false;
    var editor = GetCurrentEditor();
    if (editor)
    {
      FinishHTMLSource();
      result = SaveDocument(IsUrlAboutBlank(GetDocumentUrl()), false, editor.contentsMIMEType);
      window.content.focus();
    }
    return result;
  }
}

var nsSaveAsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var editor = GetCurrentEditor();
    if (editor)
    {
      FinishHTMLSource();
      var result = SaveDocument(true, false, editor.contentsMIMEType);
      window.content.focus();
      return result;
    }
    return false;
  }
}

var nsExportToTextCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    if (GetCurrentEditor())
    {
      FinishHTMLSource();
      var result = SaveDocument(true, true, "text/plain");
      window.content.focus();
      return result;
    }
    return false;
  }
}

var nsSaveAndChangeEncodingCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {    
    FinishHTMLSource();
    window.ok = false;
    window.exportToText = false;
    var oldTitle = GetDocumentTitle();
    window.openDialog("chrome://editor/content/EditorSaveAsCharset.xul","_blank", "chrome,close,titlebar,modal,resizable=yes");

    if (GetDocumentTitle() != oldTitle)
      UpdateWindowTitle();

    if (window.ok)
    {
      if (window.exportToText)
      {
        window.ok = SaveDocument(true, true, "text/plain");
      }
      else
      {
        var editor = GetCurrentEditor();
        window.ok = SaveDocument(true, false, editor ? editor.contentsMIMEType : null);
      }
    }

    window.content.focus();
    return window.ok;
  }
};

var nsPublishCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    if (IsDocumentEditable())
    {
      
      
      
      try {
        var docUrl = GetDocumentUrl();
        return IsDocumentModified() || IsHTMLSourceChanged()
               || IsUrlAboutBlank(docUrl) || GetScheme(docUrl) == "file";
      } catch (e) {return false;}
    }
    return false;
  },
  
  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    if (GetCurrentEditor())
    {
      var docUrl = GetDocumentUrl();
      var filename = GetFilename(docUrl);
      var publishData;
      var showPublishDialog = false;

      
      try {
        var prefs = GetPrefs();
        if (prefs)
          showPublishDialog = prefs.getBoolPref("editor.always_show_publish_dialog");
      } catch(e) {}

      if (!showPublishDialog && filename)
      {
        
        publishData = CreatePublishDataFromUrl(docUrl);

        
        
        
      }

      if (showPublishDialog || !publishData)
      {
        
        publishData = {};
        window.ok = false;
        var oldTitle = GetDocumentTitle();
        window.openDialog("chrome://editor/content/EditorPublish.xul","_blank", 
                          "chrome,close,titlebar,modal", "", "", publishData);
        if (GetDocumentTitle() != oldTitle)
          UpdateWindowTitle();

        window.content.focus();
        if (!window.ok)
          return false;
      }
      if (publishData)
      {
        FinishHTMLSource();
        return Publish(publishData);
      }
    }
    return false;
  }
}

var nsPublishAsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },
  
  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    if (GetCurrentEditor())
    {
      FinishHTMLSource();

      window.ok = false;
      var publishData = {};
      var oldTitle = GetDocumentTitle();
      window.openDialog("chrome://editor/content/EditorPublish.xul","_blank", 
                        "chrome,close,titlebar,modal", "", "", publishData);
      if (GetDocumentTitle() != oldTitle)
        UpdateWindowTitle();

      window.content.focus();
      if (window.ok)
        return Publish(publishData);
    }
    return false;
  }
}




function GetExtensionBasedOnMimeType(aMIMEType)
{
  try {
    var mimeService = null;
    mimeService = Components.classes["@mozilla.org/mime;1"].getService();
    mimeService = mimeService.QueryInterface(Components.interfaces.nsIMIMEService);

    var fileExtension = mimeService.getPrimaryExtension(aMIMEType, null);

    
    
    if (fileExtension == "htm")
      fileExtension = "html";

    return fileExtension;
  }
  catch (e) {}
  return "";
}

function GetSuggestedFileName(aDocumentURLString, aMIMEType)
{
  var extension = GetExtensionBasedOnMimeType(aMIMEType);
  if (extension)
    extension = "." + extension;

  
  if (aDocumentURLString.length >= 0 && !IsUrlAboutBlank(aDocumentURLString))
  {
    var docURI = null;
    try {

      var ioService = GetIOService();
      docURI = ioService.newURI(aDocumentURLString, GetCurrentEditor().documentCharacterSet, null);
      docURI = docURI.QueryInterface(Components.interfaces.nsIURL);

      
      var url = docURI.fileBaseName;
      if (url)
        return url+extension;
    } catch(e) {}
  } 

  
  var title = GetDocumentTitle();
  
  return GenerateValidFilename(title, extension) || GetString("untitled") + extension;
}


function PromptForSaveLocation(aDoSaveAsText, aEditorType, aMIMEType, aDocumentURLString)
{
  var dialogResult = {};
  dialogResult.filepickerClick = nsIFilePicker.returnCancel;
  dialogResult.resultingURI = "";
  dialogResult.resultingLocalFile = null;

  var fp = null;
  try {
    fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  } catch (e) {}
  if (!fp) return dialogResult;

  
  var promptString;
  if (aDoSaveAsText || aEditorType == "text")
    promptString = GetString("ExportToText");
  else
    promptString = GetString("SaveDocumentAs")

  fp.init(window, promptString, nsIFilePicker.modeSave);

  
  if (aDoSaveAsText)
    fp.appendFilters(nsIFilePicker.filterText);
  else
    fp.appendFilters(nsIFilePicker.filterHTML);
  fp.appendFilters(nsIFilePicker.filterAll);

  
  var suggestedFileName = GetSuggestedFileName(aDocumentURLString, aMIMEType);
  if (suggestedFileName)
    fp.defaultString = suggestedFileName;

  
  
  try {
    var ioService = GetIOService();
    var fileHandler = GetFileProtocolHandler();
    
    var isLocalFile = true;
    try {
      var docURI = ioService.newURI(aDocumentURLString, GetCurrentEditor().documentCharacterSet, null);
      isLocalFile = docURI.schemeIs("file");
    }
    catch (e) {}

    var parentLocation = null;
    if (isLocalFile)
    {
      var fileLocation = fileHandler.getFileFromURLSpec(aDocumentURLString); 
      parentLocation = fileLocation.parent;
    }
    if (parentLocation)
    {
      
      if ("gFilePickerDirectory" in window)
        gFilePickerDirectory = fp.displayDirectory;

      fp.displayDirectory = parentLocation;
    }
    else
    {
      
      SetFilePickerDirectory(fp, aEditorType);
    }
  }
  catch(e) {}

  dialogResult.filepickerClick = fp.show();
  if (dialogResult.filepickerClick != nsIFilePicker.returnCancel)
  {
    
    dialogResult.resultingURIString = fileHandler.getURLSpecFromFile(fp.file);
    dialogResult.resultingLocalFile = fp.file;
    SaveFilePickerDirectory(fp, aEditorType);
  }
  else if ("gFilePickerDirectory" in window && gFilePickerDirectory)
    fp.displayDirectory = gFilePickerDirectory; 

  return dialogResult;
}


function PromptAndSetTitleIfNone()
{
  if (GetDocumentTitle()) 
    return true;

  var promptService = GetPromptService();
  if (!promptService) return false;

  var result = {value:null};
  var captionStr = GetString("DocumentTitle");
  var msgStr = GetString("NeedDocTitle") + '\n' + GetString("DocTitleHelp");
  var confirmed = promptService.prompt(window, captionStr, msgStr, result, null, {value:0});
  if (confirmed)
    SetDocumentTitle(TrimString(result.value));

  return confirmed;
}

var gPersistObj;








      

const webPersist = Components.interfaces.nsIWebBrowserPersist;
function OutputFileWithPersistAPI(editorDoc, aDestinationLocation, aRelatedFilesParentDir, aMimeType)
{
  gPersistObj = null;
  var editor = GetCurrentEditor();
  try {
    var imeEditor = editor.QueryInterface(Components.interfaces.nsIEditorIMESupport);
    imeEditor.forceCompositionEnd();
    } catch (e) {}

  var isLocalFile = false;
  try {
    var tmp1 = aDestinationLocation.QueryInterface(Components.interfaces.nsIFile);
    isLocalFile = true;
  } 
  catch (e) {
    try {
      var tmp = aDestinationLocation.QueryInterface(Components.interfaces.nsIURI);
      isLocalFile = tmp.schemeIs("file");
    }
    catch (e) {}
  }

  try {
    
    var persistObj = Components.classes["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"].createInstance(webPersist);
    persistObj.progressListener = gEditorOutputProgressListener;
    
    var wrapColumn = GetWrapColumn();
    var outputFlags = GetOutputFlags(aMimeType, wrapColumn);

    
    
    if (!isLocalFile) 
    {
      outputFlags |= webPersist.ENCODE_FLAGS_CR_LINEBREAKS | webPersist.ENCODE_FLAGS_LF_LINEBREAKS;

      
      
      
      persistObj.persistFlags = persistObj.persistFlags | webPersist.PERSIST_FLAGS_SERIALIZE_OUTPUT;
    }

    
    
    
    persistObj.persistFlags = persistObj.persistFlags 
                            | webPersist.PERSIST_FLAGS_NO_BASE_TAG_MODIFICATIONS
                            | webPersist.PERSIST_FLAGS_REPLACE_EXISTING_FILES
                            | webPersist.PERSIST_FLAGS_DONT_FIXUP_LINKS
                            | webPersist.PERSIST_FLAGS_DONT_CHANGE_FILENAMES
                            | webPersist.PERSIST_FLAGS_FIXUP_ORIGINAL_DOM;
    persistObj.saveDocument(editorDoc, aDestinationLocation, aRelatedFilesParentDir, 
                            aMimeType, outputFlags, wrapColumn);
    gPersistObj = persistObj;
  }
  catch(e) { dump("caught an error, bail\n"); return false; }

  return true;
}


function GetOutputFlags(aMimeType, aWrapColumn)
{
  var outputFlags = 0;
  var editor = GetCurrentEditor();
  var outputEntity = (editor && editor.documentCharacterSet == "ISO-8859-1")
    ? webPersist.ENCODE_FLAGS_ENCODE_LATIN1_ENTITIES
    : webPersist.ENCODE_FLAGS_ENCODE_BASIC_ENTITIES;
  if (aMimeType == "text/plain")
  {
    
    outputFlags |= webPersist.ENCODE_FLAGS_FORMATTED;
  }
  else
  {
    try {
      
      var prefs = GetPrefs();
      if (prefs.getBoolPref("editor.prettyprint"))
        outputFlags |= webPersist.ENCODE_FLAGS_FORMATTED;

      
      var encodeEntity = prefs.getCharPref("editor.encode_entity");
      switch (encodeEntity) {
        case "basic"  : outputEntity = webPersist.ENCODE_FLAGS_ENCODE_BASIC_ENTITIES; break;
        case "latin1" : outputEntity = webPersist.ENCODE_FLAGS_ENCODE_LATIN1_ENTITIES; break;
        case "html"   : outputEntity = webPersist.ENCODE_FLAGS_ENCODE_HTML_ENTITIES; break;
        case "none"   : outputEntity = 0; break;
      }
    }
    catch (e) {}
  }
  outputFlags |= outputEntity;

  if (aWrapColumn > 0)
    outputFlags |= webPersist.ENCODE_FLAGS_WRAP;

  return outputFlags;
}


const nsIWebBrowserPersist = Components.interfaces.nsIWebBrowserPersist;
function GetWrapColumn()
{
  try {
    return GetCurrentEditor().wrapWidth;
  } catch (e) {}
  return 0;
}

function GetPromptService()
{
  var promptService;
  try {
    promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
    promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService);
  }
  catch (e) {}
  return promptService;
}

const gShowDebugOutputStateChange = false;
const gShowDebugOutputProgress = false;
const gShowDebugOutputStatusChange = false;

const gShowDebugOutputLocationChange = false;
const gShowDebugOutputSecurityChange = false;

const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
const nsIChannel = Components.interfaces.nsIChannel;

const kErrorBindingAborted = 2152398850;
const kErrorBindingRedirected = 2152398851;
const kFileNotFound = 2152857618;

var gEditorOutputProgressListener =
{
  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    var editor = GetCurrentEditor();

    
    var requestSpec;
    try {
      var channel = aRequest.QueryInterface(nsIChannel);
      requestSpec = StripUsernamePasswordFromURI(channel.URI);
    } catch (e) {
      if ( gShowDebugOutputStateChange)
        dump("***** onStateChange; NO REQUEST CHANNEL\n");
    }

    var pubSpec;
    if (gPublishData)
      pubSpec = gPublishData.publishUrl + gPublishData.docDir + gPublishData.filename;

    if (gShowDebugOutputStateChange)
    {
      dump("\n***** onStateChange request: " + requestSpec + "\n");
      dump("      state flags: ");

      if (aStateFlags & nsIWebProgressListener.STATE_START)
        dump(" STATE_START, ");
      if (aStateFlags & nsIWebProgressListener.STATE_STOP)
        dump(" STATE_STOP, ");
      if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK)
        dump(" STATE_IS_NETWORK ");

      dump("\n * requestSpec="+requestSpec+", pubSpec="+pubSpec+", aStatus="+aStatus+"\n");

      DumpDebugStatus(aStatus);
    }
    
    if (!gProgressDialog)
      return;

    
    
    if ((aStateFlags & nsIWebProgressListener.STATE_START)
         && gPersistObj && requestSpec
         && (gPersistObj.currentState != gPersistObj.PERSIST_STATE_FINISHED))
    {
      try {
        
        gProgressDialog.SetProgressStatus(GetFilename(requestSpec), "busy");
      } catch(e) {}
    }

    
    if (aStateFlags & nsIWebProgressListener.STATE_STOP)
    {
      
      try {
        
        var httpChannel = aRequest.QueryInterface(Components.interfaces.nsIHttpChannel);
        var httpResponse = httpChannel.responseStatus;
        if (httpResponse < 200 || httpResponse >= 300)
          aStatus = httpResponse;   
        else if (aStatus == kErrorBindingAborted)
          aStatus = 0;

        if (gShowDebugOutputStateChange)
          dump("http response is: "+httpResponse+"\n");
      } 
      catch(e) 
      {
        if (aStatus == kErrorBindingAborted)
          aStatus = 0;
      }

      
      var abortPublishing = (aStatus != 0 && aStatus != kFileNotFound);

      
      
      
      
      if (aStatus != 0 
           || (requestSpec && requestSpec.indexOf(GetScheme(gPublishData.publishUrl)) == 0))
      {
        try {
          gProgressDialog.SetProgressFinished(GetFilename(requestSpec), aStatus);
        } catch(e) {}
      }


      if (abortPublishing)
      {
        
        gPersistObj.cancelSave();

        
        gCommandAfterPublishing = null;

        
        if (gRestoreDocumentSource)
        {
          try {
            editor.rebuildDocumentFromSource(gRestoreDocumentSource);

            
            
            editor.transactionManager.clear();
          }
          catch (e) {}
        }

        
        
        gProgressDialog.SetProgressFinished(null, 0);

        
        return;
      }

      
      
      
      
      

      
      
      

      
      
      
      
      if (!requestSpec && GetScheme(gPublishData.publishUrl) == "file"
          && (!gPersistObj || gPersistObj.currentState == nsIWebBrowserPersist.PERSIST_STATE_FINISHED))
      {
        aStateFlags |= nsIWebProgressListener.STATE_IS_NETWORK;
        if (!gPersistObj)
        {          
          gPersistObj =
          {
            result : aStatus,
            currentState : nsIWebBrowserPersist.PERSIST_STATE_FINISHED
          }
        }
      }

      
      if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK
          && gPersistObj.currentState == nsIWebBrowserPersist.PERSIST_STATE_FINISHED)
      {
        if (GetScheme(gPublishData.publishUrl) == "file")
        {
          
          
          gProgressDialog.SetProgressFinished(gPublishData.filename, gPersistObj.result);
        }

        if (gPersistObj.result == 0)
        {
          
          try {
            
            
            var docUrl = GetDocUrlFromPublishData(gPublishData);
            SetDocumentURI(GetIOService().newURI(docUrl, editor.documentCharacterSet, null));

            UpdateWindowTitle();

            
            editor.resetModificationCount();

            
            SetSaveAndPublishUI(urlstring);

          } catch (e) {}

          
          if (gPublishData)
          {
            if (gPublishData.savePublishData)
            {
              
              
              gPublishData.saveDirs = true;
              SavePublishDataToPrefs(gPublishData);
            }
            else
              SavePassword(gPublishData);
          }

          
          
          gProgressDialog.RequestCloseDialog();
        }
        else
        {
          
          
          
          gProgressDialog.SetProgressFinished(null, 0);
        }
      }
    }
  },

  onProgressChange : function(aWebProgress, aRequest, aCurSelfProgress,
                              aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {
    if (!gPersistObj)
      return;

    if (gShowDebugOutputProgress)
    {
      dump("\n onProgressChange: gPersistObj.result="+gPersistObj.result+"\n");
      try {
      var channel = aRequest.QueryInterface(nsIChannel);
      dump("***** onProgressChange request: " + channel.URI.spec + "\n");
      }
      catch (e) {}
      dump("*****       self:  "+aCurSelfProgress+" / "+aMaxSelfProgress+"\n");
      dump("*****       total: "+aCurTotalProgress+" / "+aMaxTotalProgress+"\n\n");

      if (gPersistObj.currentState == gPersistObj.PERSIST_STATE_READY)
        dump(" Persister is ready to save data\n\n");
      else if (gPersistObj.currentState == gPersistObj.PERSIST_STATE_SAVING)
        dump(" Persister is saving data.\n\n");
      else if (gPersistObj.currentState == gPersistObj.PERSIST_STATE_FINISHED)
        dump(" PERSISTER HAS FINISHED SAVING DATA\n\n\n");
    }
  },

  onLocationChange : function(aWebProgress, aRequest, aLocation)
  {
    if (gShowDebugOutputLocationChange)
    {
      dump("***** onLocationChange: "+aLocation.spec+"\n");
      try {
        var channel = aRequest.QueryInterface(nsIChannel);
        dump("*****          request: " + channel.URI.spec + "\n");
      }
      catch(e) {}
    }
  },

  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {
    if (gShowDebugOutputStatusChange)
    {
      dump("***** onStatusChange: "+aMessage+"\n");
      try {
        var channel = aRequest.QueryInterface(nsIChannel);
        dump("*****        request: " + channel.URI.spec + "\n");
      }
      catch (e) { dump("          couldn't get request\n"); }
      
      DumpDebugStatus(aStatus);

      if (gPersistObj)
      {
        if(gPersistObj.currentState == gPersistObj.PERSIST_STATE_READY)
          dump(" Persister is ready to save data\n\n");
        else if(gPersistObj.currentState == gPersistObj.PERSIST_STATE_SAVING)
          dump(" Persister is saving data.\n\n");
        else if(gPersistObj.currentState == gPersistObj.PERSIST_STATE_FINISHED)
          dump(" PERSISTER HAS FINISHED SAVING DATA\n\n\n");
      }
    }
  },

  onSecurityChange : function(aWebProgress, aRequest, state)
  {
    if (gShowDebugOutputSecurityChange)
    {
      try {
        var channel = aRequest.QueryInterface(nsIChannel);
        dump("***** onSecurityChange request: " + channel.URI.spec + "\n");
      } catch (e) {}
    }
  },

  QueryInterface : function(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener)
    || aIID.equals(Components.interfaces.nsISupports)
    || aIID.equals(Components.interfaces.nsISupportsWeakReference)
    || aIID.equals(Components.interfaces.nsIPrompt)
    || aIID.equals(Components.interfaces.nsIAuthPrompt))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },


  alert : function(dlgTitle, text)
  {
    AlertWithTitle(dlgTitle, text, gProgressDialog ? gProgressDialog : window);
  },
  alertCheck : function(dialogTitle, text, checkBoxLabel, checkObj)
  {
    AlertWithTitle(dialogTitle, text);
  },
  confirm : function(dlgTitle, text)
  {
    return ConfirmWithTitle(dlgTitle, text, null, null);
  },
  confirmCheck : function(dlgTitle, text, checkBoxLabel, checkObj)
  {
    var promptServ = GetPromptService();
    if (!promptServ)
      return;

    promptServ.confirmEx(window, dlgTitle, text, nsIPromptService.STD_OK_CANCEL_BUTTONS,
                         "", "", "", checkBoxLabel, checkObj);
  },
  confirmEx : function(dlgTitle, text, btnFlags, btn0Title, btn1Title, btn2Title, checkBoxLabel, checkVal)
  {
    var promptServ = GetPromptService();
    if (!promptServ)
     return 0;

    return promptServ.confirmEx(window, dlgTitle, text, btnFlags,
                        btn0Title, btn1Title, btn2Title,
                        checkBoxLabel, checkVal);
  },
  prompt : function(dlgTitle, text, inoutText, checkBoxLabel, checkObj)
  {
    var promptServ = GetPromptService();
    if (!promptServ)
     return false;

    return promptServ.prompt(window, dlgTitle, text, inoutText, checkBoxLabel, checkObj);
  },
  promptPassword : function(dlgTitle, text, pwObj, checkBoxLabel, savePWObj)
  {

    var promptServ = GetPromptService();
    if (!promptServ)
     return false;

    var ret = false;
    try {
      
      
      
      if (gPublishData)
        savePWObj.value = gPublishData.savePassword;

      ret = promptServ.promptPassword(gProgressDialog ? gProgressDialog : window,
                                      dlgTitle, text, pwObj, checkBoxLabel, savePWObj);

      if (!ret)
        setTimeout(CancelPublishing, 0);

      if (ret && gPublishData)
        UpdateUsernamePasswordFromPrompt(gPublishData, gPublishData.username, pwObj.value, savePWObj.value);
    } catch(e) {}

    return ret;
  },
  promptUsernameAndPassword : function(dlgTitle, text, userObj, pwObj, checkBoxLabel, savePWObj)
  {
    var ret = PromptUsernameAndPassword(dlgTitle, text, savePWObj.value, userObj, pwObj);
    if (!ret)
      setTimeout(CancelPublishing, 0);

    return ret;
  },
  select : function(dlgTitle, text, count, selectList, outSelection)
  {
    var promptServ = GetPromptService();
    if (!promptServ)
      return false;

    return promptServ.select(window, dlgTitle, text, count, selectList, outSelection);
  },


  prompt : function(dlgTitle, text, pwrealm, savePW, defaultText, result)
  {
    var promptServ = GetPromptService();
    if (!promptServ)
      return false;

    var savePWObj = {value:savePW};
    var ret = promptServ.prompt(gProgressDialog ? gProgressDialog : window,
                                dlgTitle, text, defaultText, pwrealm, savePWObj);
    if (!ret)
      setTimeout(CancelPublishing, 0);
    return ret;
  },

  promptUsernameAndPassword : function(dlgTitle, text, pwrealm, savePW, userObj, pwObj)
  {
    var ret = PromptUsernameAndPassword(dlgTitle, text, savePW, userObj, pwObj);
    if (!ret)
      setTimeout(CancelPublishing, 0);
    return ret;
  },

  promptPassword : function(dlgTitle, text, pwrealm, savePW, pwObj)
  {
    var ret = false;
    try {
      var promptServ = GetPromptService();
      if (!promptServ)
        return false;

      
      
      
      
      var savePWObj = {value:savePW};
      
      if (gPublishData)
        savePWObj.value = gPublishData.savePassword;

      ret = promptServ.promptPassword(gProgressDialog ? gProgressDialog : window,
                                      dlgTitle, text, pwObj, GetString("SavePassword"), savePWObj);

      if (!ret)
        setTimeout(CancelPublishing, 0);

      if (ret && gPublishData)
        UpdateUsernamePasswordFromPrompt(gPublishData, gPublishData.username, pwObj.value, savePWObj.value);
    } catch(e) {}

    return ret;
  }
}

function PromptUsernameAndPassword(dlgTitle, text, savePW, userObj, pwObj)
{
  
  
  if (!gPublishData)
    return false

  var ret = false;
  try {
    var promptServ = GetPromptService();
    if (!promptServ)
      return false;

    var savePWObj = {value:savePW};

    
    if (gPublishData)
    {
      
      
      savePWObj.value = gPublishData.savePassword;
      if (!userObj.value)
        userObj.value = gPublishData.username;
    }

    ret = promptServ.promptUsernameAndPassword(gProgressDialog ? gProgressDialog : window, 
                                               dlgTitle, text, userObj, pwObj, 
                                               GetString("SavePassword"), savePWObj);
    if (ret && gPublishData)
      UpdateUsernamePasswordFromPrompt(gPublishData, userObj.value, pwObj.value, savePWObj.value);

  } catch (e) {}

  return ret;
}

function DumpDebugStatus(aStatus)
{
  

  if (aStatus == kErrorBindingAborted)
    dump("***** status is NS_BINDING_ABORTED\n");
  else if (aStatus == kErrorBindingRedirected)
    dump("***** status is NS_BINDING_REDIRECTED\n");
  else if (aStatus == 2152398859) 
    dump("***** status is ALREADY_CONNECTED\n");
  else if (aStatus == 2152398860) 
    dump("***** status is NOT_CONNECTED\n");
  else if (aStatus == 2152398861) 
    dump("***** status is CONNECTION_REFUSED\n");
  else if (aStatus == 2152398862) 
    dump("***** status is NET_TIMEOUT\n");
  else if (aStatus == 2152398863) 
    dump("***** status is IN_PROGRESS\n");
  else if (aStatus == 2152398864) 
    dump("***** status is OFFLINE\n");
  else if (aStatus == 2152398865) 
    dump("***** status is NO_CONTENT\n");
  else if (aStatus == 2152398866) 
    dump("***** status is UNKNOWN_PROTOCOL\n");
  else if (aStatus == 2152398867) 
    dump("***** status is PORT_ACCESS_NOT_ALLOWED\n");
  else if (aStatus == 2152398868) 
    dump("***** status is NET_RESET\n");
  else if (aStatus == 2152398869) 
    dump("***** status is FTP_LOGIN\n");
  else if (aStatus == 2152398870) 
    dump("***** status is FTP_CWD\n");
  else if (aStatus == 2152398871) 
    dump("***** status is FTP_PASV\n");
  else if (aStatus == 2152398872) 
    dump("***** status is FTP_PWD\n");
  else if (aStatus == 2152857601)
    dump("***** status is UNRECOGNIZED_PATH\n");
  else if (aStatus == 2152857602)
    dump("***** status is UNRESOLABLE SYMLINK\n");
  else if (aStatus == 2152857604)
    dump("***** status is UNKNOWN_TYPE\n");
  else if (aStatus == 2152857605)
    dump("***** status is DESTINATION_NOT_DIR\n");
  else if (aStatus == 2152857606)
    dump("***** status is TARGET_DOES_NOT_EXIST\n");
  else if (aStatus == 2152857608)
    dump("***** status is ALREADY_EXISTS\n");
  else if (aStatus == 2152857609)
    dump("***** status is INVALID_PATH\n");
  else if (aStatus == 2152857610)
    dump("***** status is DISK_FULL\n");
  else if (aStatus == 2152857612)
    dump("***** status is NOT_DIRECTORY\n");
  else if (aStatus == 2152857613)
    dump("***** status is IS_DIRECTORY\n");
  else if (aStatus == 2152857614)
    dump("***** status is IS_LOCKED\n");
  else if (aStatus == 2152857615)
    dump("***** status is TOO_BIG\n");
  else if (aStatus == 2152857616)
    dump("***** status is NO_DEVICE_SPACE\n");
  else if (aStatus == 2152857617)
    dump("***** status is NAME_TOO_LONG\n");
  else if (aStatus == 2152857618) 
    dump("***** status is FILE_NOT_FOUND\n");
  else if (aStatus == 2152857619)
    dump("***** status is READ_ONLY\n");
  else if (aStatus == 2152857620)
    dump("***** status is DIR_NOT_EMPTY\n");
  else if (aStatus == 2152857621)
    dump("***** status is ACCESS_DENIED\n");
  else if (aStatus == 2152398878)
    dump("***** status is ? (No connection or time out?)\n");
  else
    dump("***** status is " + aStatus + "\n");
}


function UpdateUsernamePasswordFromPrompt(publishData, username, password, savePassword)
{
  if (!publishData)
    return;
  
  
  
  
  
  publishData.savePublishData = (gPublishData.username != username || gPublishData.password != password)
                                && (savePassword || !publishData.notInSiteData);

  publishData.username = username;
  publishData.password = password;
  publishData.savePassword = savePassword;
}

const kSupportedTextMimeTypes =
[
  "text/plain",
  "text/css",
  "text/rdf",
  "text/xsl",
  "text/javascript",
  "text/ecmascript",
  "application/javascript",
  "application/ecmascript",
  "application/x-javascript",
  "text/xul",
  "application/vnd.mozilla.xul+xml"
];

function IsSupportedTextMimeType(aMimeType)
{
  for (var i = 0; i < kSupportedTextMimeTypes.length; i++)
  {
    if (kSupportedTextMimeTypes[i] == aMimeType)
      return true;
  }
  return false;
}


function SaveDocument(aSaveAs, aSaveCopy, aMimeType)
{
  var editor = GetCurrentEditor();
  if (!aMimeType || aMimeType == "" || !editor)
    throw NS_ERROR_NOT_INITIALIZED;

  var editorDoc = editor.document;
  if (!editorDoc)
    throw NS_ERROR_NOT_INITIALIZED;

  
  var editorType = GetCurrentEditorType();
  if (editorType != "text" && editorType != "html" 
      && editorType != "htmlmail" && editorType != "textmail")
    throw NS_ERROR_NOT_IMPLEMENTED;

  var saveAsTextFile = IsSupportedTextMimeType(aMimeType);

  
  if (aMimeType != "text/html" && !saveAsTextFile)
    throw NS_ERROR_NOT_IMPLEMENTED;

  if (saveAsTextFile)
    aMimeType = "text/plain";

  var urlstring = GetDocumentUrl();
  var mustShowFileDialog = (aSaveAs || IsUrlAboutBlank(urlstring) || (urlstring == ""));

  
  if (!mustShowFileDialog && GetScheme(urlstring) != "file")
    mustShowFileDialog = true;

  var replacing = !aSaveAs;
  var titleChanged = false;
  var doUpdateURI = false;
  var tempLocalFile = null;

  if (mustShowFileDialog)
  {
	  try {
	    
	    if (!saveAsTextFile && (editorType == "html"))
	    {
	      var userContinuing = PromptAndSetTitleIfNone(); 
	      if (!userContinuing)
	        return false;
	    }

	    var dialogResult = PromptForSaveLocation(saveAsTextFile, editorType, aMimeType, urlstring);
	    if (dialogResult.filepickerClick == nsIFilePicker.returnCancel)
	      return false;

	    replacing = (dialogResult.filepickerClick == nsIFilePicker.returnReplace);
	    urlstring = dialogResult.resultingURIString;
	    tempLocalFile = dialogResult.resultingLocalFile;
 
      
      if (!aSaveCopy)
        doUpdateURI = true;
   } catch (e) {  return false; }
  } 

  var success = true;
  var ioService;
  try {
    
    
    var docURI;
    if (!tempLocalFile)
    {
      ioService = GetIOService();
      docURI = ioService.newURI(urlstring, editor.documentCharacterSet, null);
      
      if (docURI.schemeIs("file"))
      {
        var fileHandler = GetFileProtocolHandler();
        tempLocalFile = fileHandler.getFileFromURLSpec(urlstring).QueryInterface(Components.interfaces.nsILocalFile);
      }
    }

    
    var relatedFilesDir = null;
    
    
    var saveAssociatedFiles = false;
    try {
      var prefs = GetPrefs();
      saveAssociatedFiles = prefs.getBoolPref("editor.save_associated_files");
    } catch (e) {}

    
    
    if (saveAssociatedFiles && aSaveAs)
    {
      try {
        if (tempLocalFile)
        {
          
          
          
          var oldLocation = GetDocumentUrl();
          var oldLocationLastSlash = oldLocation.lastIndexOf("\/");
          if (oldLocationLastSlash != -1)
            oldLocation = oldLocation.slice(0, oldLocationLastSlash);

          var relatedFilesDirStr = urlstring;
          var newLocationLastSlash = relatedFilesDirStr.lastIndexOf("\/");
          if (newLocationLastSlash != -1)
            relatedFilesDirStr = relatedFilesDirStr.slice(0, newLocationLastSlash);
          if (oldLocation == relatedFilesDirStr || IsUrlAboutBlank(oldLocation))
            relatedFilesDir = null;
          else
            relatedFilesDir = tempLocalFile.parent;
        }
        else
        {
          var lastSlash = urlstring.lastIndexOf("\/");
          if (lastSlash != -1)
          {
            var relatedFilesDirString = urlstring.slice(0, lastSlash + 1);  
            ioService = GetIOService();
            relatedFilesDir = ioService.newURI(relatedFilesDirString, editor.documentCharacterSet, null);
          }
        }
      } catch(e) { relatedFilesDir = null; }
    }

    var destinationLocation;
    if (tempLocalFile)
      destinationLocation = tempLocalFile;
    else
      destinationLocation = docURI;

    success = OutputFileWithPersistAPI(editorDoc, destinationLocation, relatedFilesDir, aMimeType);
  }
  catch (e)
  {
    success = false;
  }

  if (success)
  {
    try {
      if (doUpdateURI)
      {
         
        if (tempLocalFile)
          docURI = GetFileProtocolHandler().newFileURI(tempLocalFile);

        
        SetDocumentURI(docURI);
      }

      
      
      
      UpdateWindowTitle();

      if (!aSaveCopy)
        editor.resetModificationCount();
      

      
      SetSaveAndPublishUI(urlstring);
    } catch (e) {}
  }
  else
  {
    var saveDocStr = GetString("SaveDocument");
    var failedStr = GetString("SaveFileFailed");
    AlertWithTitle(saveDocStr, failedStr);
  }
  return success;
}

function SetDocumentURI(uri)
{
  try {
    
    GetCurrentEditorElement().docShell.setCurrentURI(uri);
  } catch (e) { dump("SetDocumentURI:\n"+e +"\n"); }
}



var gPublishData;
var gProgressDialog;
var gCommandAfterPublishing = null;
var gRestoreDocumentSource;

function Publish(publishData)
{
  if (!publishData)
    return false;

  
  
  
  gPublishData = publishData;

  gPublishData.docURI = CreateURIFromPublishData(publishData, true);
  if (!gPublishData.docURI)
  {
    AlertWithTitle(GetString("Publish"), GetString("PublishFailed"));
    return false;
  }

  if (gPublishData.publishOtherFiles)
    gPublishData.otherFilesURI = CreateURIFromPublishData(publishData, false);
  else
    gPublishData.otherFilesURI = null;

  if (gShowDebugOutputStateChange)
  {
    dump("\n *** publishData: PublishUrl="+publishData.publishUrl+", BrowseUrl="+publishData.browseUrl+
      ", Username="+publishData.username+", Dir="+publishData.docDir+
      ", Filename="+publishData.filename+"\n");
    dump(" * gPublishData.docURI.spec w/o pass="+StripPassword(gPublishData.docURI.spec)+", PublishOtherFiles="+gPublishData.publishOtherFiles+"\n");
  }

  
  
  
  
  if (GetScheme(publishData.publishUrl) == "ftp" && !publishData.username)
  {
    var message = GetString("PromptFTPUsernamePassword").replace(/%host%/, GetHost(publishData.publishUrl));
    var savePWobj = {value:publishData.savePassword};
    var userObj = {value:publishData.username};
    var pwObj = {value:publishData.password};
    if (!PromptUsernameAndPassword(GetString("Prompt"), message, savePWobj, userObj, pwObj))
      return false; 

    
    gPublishData.docURI.username = publishData.username;
    gPublishData.docURI.password = publishData.password;

    if (gPublishData.otherFilesURI)
    {
      gPublishData.otherFilesURI.username = publishData.username;
      gPublishData.otherFilesURI.password = publishData.password;
    }
  }

  try {
    
    
    SetDocumentEditable(false);

    
    gProgressDialog =
      window.openDialog("chrome://editor/content/EditorPublishProgress.xul", "_blank",
                        "chrome,dependent,titlebar", gPublishData, gPersistObj);

  } catch (e) {}

  
  
  
  
  return true;
}

function StartPublishing()
{
  var editor = GetCurrentEditor();
  if (editor && gPublishData && gPublishData.docURI && gProgressDialog)
  {
    gRestoreDocumentSource = null;

    
    
    if (gPublishData.otherFilesURI)
    {
      try {
        gRestoreDocumentSource = 
          editor.outputToString(editor.contentsMIMEType, kOutputEncodeW3CEntities);
      } catch (e) {}
    }

    OutputFileWithPersistAPI(editor.document, 
                             gPublishData.docURI, gPublishData.otherFilesURI, 
                             editor.contentsMIMEType);
    return gPersistObj;
  }
  return null;
}

function CancelPublishing()
{
  try {
    gPersistObj.cancelSave();
    gProgressDialog.SetProgressStatusCancel();
  } catch (e) {}

  
  gCommandAfterPublishing = null;

  if (gProgressDialog)
  {
    
    
    gProgressDialog.CloseDialog();
  }
  else
    FinishPublishing();
}

function FinishPublishing()
{
  SetDocumentEditable(true);
  gProgressDialog = null;
  gPublishData = null;
  gRestoreDocumentSource = null;

  if (gCommandAfterPublishing)
  {
    
    var command = gCommandAfterPublishing;
    gCommandAfterPublishing = null;
    goDoCommand(command);
  }
}


function CreateURIFromPublishData(publishData, doDocUri)
{
  if (!publishData || !publishData.publishUrl)
    return null;

  var URI;
  try {
    var spec = publishData.publishUrl;
    if (doDocUri)
      spec += FormatDirForPublishing(publishData.docDir) + publishData.filename; 
    else
      spec += FormatDirForPublishing(publishData.otherDir);

    var ioService = GetIOService();
    URI = ioService.newURI(spec, GetCurrentEditor().documentCharacterSet, null);

    if (publishData.username)
      URI.username = publishData.username;
    if (publishData.password)
      URI.password = publishData.password;
  }
  catch (e) {}

  return URI;
}


function GetDocUrlFromPublishData(publishData)
{
  if (!publishData || !publishData.filename || !publishData.publishUrl)
    return "";

  
  var url;
  var docScheme = GetScheme(GetDocumentUrl());

  
  
  
  if (!GetScheme(publishData.browseUrl))
    url = publishData.publishUrl;
  else
    url = publishData.browseUrl;

  url += FormatDirForPublishing(publishData.docDir) + publishData.filename;

  if (GetScheme(url) == "ftp")
    url = InsertUsernameIntoUrl(url, publishData.username);

  return url;
}

function SetSaveAndPublishUI(urlstring)
{
  
  goUpdateCommand("cmd_save");
  goUpdateCommand("cmd_publish");
}

function SetDocumentEditable(isDocEditable)
{
  var editor = GetCurrentEditor();
  if (editor && editor.document)
  {
    try {
      var flags = editor.flags;
      editor.flags = isDocEditable ?  
            flags &= ~nsIPlaintextEditor.eEditorReadonlyMask :
            flags | nsIPlaintextEditor.eEditorReadonlyMask;
    } catch(e) {}

    
    window.updateCommands("create");
  }  
}




var nsPublishSettingsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    if (GetCurrentEditor())
    {
      

      window.ok = window.openDialog("chrome://editor/content/EditorPublishSettings.xul","_blank", "chrome,close,titlebar,modal", "");
      window.content.focus();
      return window.ok;
    }
    return false;
  }
}


var nsRevertCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() &&
            IsDocumentModified() &&
            !IsUrlAboutBlank(GetDocumentUrl()));
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    var promptService = GetPromptService();
    if (promptService)
    {
      
      var title = GetDocumentTitle();
      if (!title)
        title = GetString("untitled");

      var msg = GetString("AbandonChanges").replace(/%title%/,title);

      var result = promptService.confirmEx(window, GetString("RevertCaption"), msg,
  						      (promptService.BUTTON_TITLE_REVERT * promptService.BUTTON_POS_0) +
  						      (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1),
  						      null, null, null, null, {value:0});

      
      if(result == 0)
      {
        CancelHTMLSource();
        EditorLoadUrl(GetDocumentUrl());
      }
    }
  }
};


var nsCloseCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return GetCurrentEditor() != null;
  },
  
  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    CloseWindow();
  }
};

function CloseWindow()
{
  
  
  if (CheckAndSaveDocument("cmd_close", true)) 
  {
    if (window.InsertCharWindow)
      SwitchInsertCharToAnotherEditorOrClose();

    try {
      var basewin = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIWebNavigation)
                      .QueryInterface(Components.interfaces.nsIDocShellTreeItem)
                      .treeOwner
                      .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIBaseWindow);
      basewin.destroy();
    } catch (e) {}
  }
}


var nsOpenRemoteCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;    
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var params = { browser: null, action: null, url: "" };
    openDialog( "chrome://communicator/content/openLocation.xul", "_blank", "chrome,modal,titlebar", params);
    var win = getTopWin();
    switch (params.action) {
      case "0": 
        win.focus();
        win.loadURI(params.url, null,
                    nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP);
        break;
      case "1": 
        openDialog(getBrowserURL(), "_blank", "all,dialog=no", params.url, null,
                   null, nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP);
        break;
      case "2": 
        editPage(params.url);
        break;
      case "3": 
        win.focus();
        var browser = win.getBrowser();
        browser.selectedTab = browser.addTab(params.url, null, null, false,
                nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP);
        break;
      default:
        window.content.focus();
        break;
    }
  }
};


var nsPreviewCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && 
            IsHTMLEditor() && 
            (DocumentHasBeenSaved() || IsDocumentModified()));
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
	  
    
    if (!CheckAndSaveDocument("cmd_preview", DocumentHasBeenSaved()))
	    return;

    
	  if (DocumentHasBeenSaved())
    {
      var browser;
      try {
        
        var windowManager = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService();
        var windowManagerInterface = windowManager.QueryInterface(Components.interfaces.nsIWindowMediator);
        var enumerator = windowManagerInterface.getEnumerator("navigator:browser");

        var documentURI = GetDocumentUrl();
        while ( enumerator.hasMoreElements() )
        {
          browser = enumerator.getNext().QueryInterface(Components.interfaces.nsIDOMWindowInternal);
          if ( browser && (documentURI == browser.getBrowser().currentURI.spec))
            break;

          browser = null;
        }
      }
      catch (ex) {}

      
      if (!browser)
      {
        browser = window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no", documentURI);
      }
      else
      {
        try {
          browser.BrowserReloadSkipCache();
          browser.focus();
        } catch (ex) {}
      }
    }
  }
};


var nsSendPageCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() &&
            (DocumentHasBeenSaved() || IsDocumentModified()));
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    
    if (!CheckAndSaveDocument("cmd_editSendPage", DocumentHasBeenSaved()))
	    return;

    
    if (DocumentHasBeenSaved())
    {
      
      try
      {
        openComposeWindow(GetDocumentUrl(), GetDocumentTitle());        
      } catch (ex) { dump("Cannot Send Page: " + ex + "\n"); }
    }
  }
};


var nsPrintCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;    
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    FinishHTMLSource();
    try {
      NSPrint();
    } catch (e) {}
  }
};


var nsPrintSetupCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;    
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    FinishHTMLSource();
    NSPrintSetup();
  }
};


var nsQuitCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;    
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {}

  







};


var nsFindCommand =
{
  isCommandEnabled: function(aCommand, editorElement)
  {
    return editorElement.getEditor(editorElement.contentWindow) != null;
  },

  getCommandStateParams: function(aCommand, aParams, editorElement) {},
  doCommandParams: function(aCommand, aParams, editorElement) {},

  doCommand: function(aCommand, editorElement)
  {
    try {
      window.openDialog("chrome://editor/content/EdReplace.xul", "_blank",
                        "chrome,modal,titlebar", editorElement);
    }
    catch(ex) {
      dump("*** Exception: couldn't open Replace Dialog\n");
    }
    
  }
};


var nsFindAgainCommand =
{
  isCommandEnabled: function(aCommand, editorElement)
  {
    
    
    return editorElement.getEditor(editorElement.contentWindow) != null;
  },

  getCommandStateParams: function(aCommand, aParams, editorElement) {},
  doCommandParams: function(aCommand, aParams, editorElement) {},

  doCommand: function(aCommand, editorElement)
  {
    try {
      var findPrev = aCommand == "cmd_findPrev";
      var findInst = editorElement.webBrowserFind;
      var findService = Components.classes["@mozilla.org/find/find_service;1"]
                                  .getService(Components.interfaces.nsIFindService);
      findInst.findBackwards = findService.findBackwards ^ findPrev;
      findInst.findNext();
      
      findInst.findBackwards = findService.findBackwards; 
    }
    catch (ex) {}
  }
};


var nsRewrapCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && !IsInHTMLSourceMode() &&
            GetCurrentEditor() instanceof Components.interfaces.nsIEditorMailSupport);
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    GetCurrentEditor().QueryInterface(Components.interfaces.nsIEditorMailSupport).rewrap(false);
  }
};


var nsSpellingCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && 
            !IsInHTMLSourceMode() && IsSpellCheckerInstalled());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.cancelSendMessage = false;
    try {
      var skipBlockQuotes = (window.document.documentElement.getAttribute("windowtype") == "msgcompose");
      window.openDialog("chrome://editor/content/EdSpellCheck.xul", "_blank",
              "chrome,close,titlebar,modal", false, skipBlockQuotes, true);
    }
    catch(ex) {}
    window.content.focus();
  }
};


var URL2Validate;
var nsValidateCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return GetCurrentEditor() != null;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    
    if (IsDocumentModified() || IsHTMLSourceChanged())
    {
      if (!CheckAndSaveDocument("cmd_validate", false))
        return;

      
      if (!DocumentHasBeenSaved())    
        return;
    }

    URL2Validate = GetDocumentUrl();
    
    var ifile;
    try {
      var fileHandler = GetFileProtocolHandler();
      ifile = fileHandler.getFileFromURLSpec(URL2Validate);
      
    } catch (e) { ifile = null; }
    if (ifile)
    {
      URL2Validate = ifile.path;
      var vwin = window.open("http://validator.w3.org/file-upload.html",
                             "EditorValidate");
      
      vwin.addEventListener("load", this.validateFilePageLoaded, false);
    }
    else
    {
      var vwin2 = window.open("http://validator.w3.org/check?uri="
                              + URL2Validate
                              + "&doctype=Inline",
                              "EditorValidate");
      
    }
  },
  validateFilePageLoaded: function(event)
  {
    event.target.forms[0].uploaded_file.value = URL2Validate;
  }
};

var nsCheckLinksCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdLinkChecker.xul","_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsFormCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdFormProps.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsInputTagCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdInputProps.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsInputImageCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdInputImage.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsTextAreaCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdTextAreaProps.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsSelectCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdSelectProps.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsButtonCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdButtonProps.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsLabelCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var tagName = "label";
    try {
      var editor = GetCurrentEditor();
      
      var labelElement = editor.getSelectedElement(tagName);
      if (!labelElement)
        labelElement = editor.getElementOrParentByTagName(tagName, editor.selection.anchorNode);
      if (!labelElement)
        labelElement = editor.getElementOrParentByTagName(tagName, editor.selection.focusNode);
      if (labelElement) {
        
        window.openDialog("chrome://editor/content/EdLabelProps.xul", "_blank", "chrome,close,titlebar,modal", labelElement);
        window.content.focus();
      } else {
        EditorSetTextProperty(tagName, "", "");
      }
    } catch (e) {}
  }
};


var nsFieldSetCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdFieldSetProps.xul", "_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsIsIndexCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      var editor = GetCurrentEditor();
      var isindexElement = editor.createElementWithDefaults("isindex");
      isindexElement.setAttribute("prompt", editor.outputToString("text/plain", kOutputSelectionOnly));
      editor.insertElementAtSelection(isindexElement, true);
    } catch (e) {}
  }
};


var nsImageCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdImageProps.xul","_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsHLineCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    
    

    var tagName = "hr";
    var editor = GetCurrentEditor();
      
    var hLine;
    try {
      hLine = editor.getSelectedElement(tagName);
    } catch (e) {return;}

    if (hLine)
    {
      
      window.openDialog("chrome://editor/content/EdHLineProps.xul", "_blank", "chrome,close,titlebar,modal");
      window.content.focus();
    } 
    else
    {
      try {
        hLine = editor.createElementWithDefaults(tagName);

        
        var prefs = GetPrefs();
        var align = prefs.getIntPref("editor.hrule.align");
        if (align == 0)
          editor.setAttributeOrEquivalent(hLine, "align", "left", true);
        else if (align == 2)
          editor.setAttributeOrEquivalent(hLine, "align", "right", true);

        
  
        var width = prefs.getIntPref("editor.hrule.width");
        var percent = prefs.getBoolPref("editor.hrule.width_percent");
        if (percent)
          width = width +"%";

        editor.setAttributeOrEquivalent(hLine, "width", width, true);

        var height = prefs.getIntPref("editor.hrule.height");
        editor.setAttributeOrEquivalent(hLine, "size", String(height), true);

        var shading = prefs.getBoolPref("editor.hrule.shading");
        if (shading)
          hLine.removeAttribute("noshade");
        else
          hLine.setAttribute("noshade", "noshade");

        editor.insertElementAtSelection(hLine, true);

      } catch (e) {}
    }
  }
};


var nsLinkCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    
    var element = GetObjectForProperties();
    if (element && element.nodeName.toLowerCase() == "img")
      window.openDialog("chrome://editor/content/EdImageProps.xul","_blank", "chrome,close,titlebar,modal", null, true);
    else
      window.openDialog("chrome://editor/content/EdLinkProps.xul","_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};


var nsAnchorCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdNamedAnchorProps.xul", "_blank", "chrome,close,titlebar,modal", "");
    window.content.focus();
  }
};


var nsInsertHTMLWithDialogCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdInsSrc.xul","_blank", "chrome,close,titlebar,modal,resizable", "");
    window.content.focus();
  }
};


var nsInsertCharsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    EditorFindOrCreateInsertCharWindow();
  }
};


var nsInsertBreakCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentEditor().insertHTML("<br>");
    } catch (e) {}
  }
};


var nsInsertBreakAllCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentEditor().insertHTML("<br clear='all'>");
    } catch (e) {}
  }
};


var nsGridCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdSnapToGrid.xul","_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};



var nsListPropertiesCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdListProps.xul","_blank", "chrome,close,titlebar,modal");
    window.content.focus();
  }
};



var nsPagePropertiesCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var oldTitle = GetDocumentTitle();
    window.openDialog("chrome://editor/content/EdPageProps.xul","_blank", "chrome,close,titlebar,modal", "");

    
    
    if (GetDocumentTitle() != oldTitle)
      UpdateWindowTitle();

    window.content.focus();
  }
};


var nsObjectPropertiesCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    var isEnabled = false;
    if (IsDocumentEditable() && IsEditingRenderedHTML())
    {
      isEnabled = (GetObjectForProperties() != null ||
                   GetCurrentEditor().getSelectedElement("href") != null);
    }
    return isEnabled;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    var element = GetObjectForProperties();
    if (element)
    {
      var name = element.nodeName.toLowerCase();
      switch (name)
      {
        case 'img':
          goDoCommand("cmd_image");
          break;
        case 'hr':
          goDoCommand("cmd_hline");
          break;
        case 'form':
          goDoCommand("cmd_form");
          break;
        case 'input':
          var type = element.getAttribute("type");
          if (type && type.toLowerCase() == "image")
            goDoCommand("cmd_inputimage");
          else
            goDoCommand("cmd_inputtag");
          break;
        case 'textarea':
          goDoCommand("cmd_textarea");
          break;
        case 'select':
          goDoCommand("cmd_select");
          break;
        case 'button':
          goDoCommand("cmd_button");
          break;
        case 'label':
          goDoCommand("cmd_label");
          break;
        case 'fieldset':
          goDoCommand("cmd_fieldset");
          break;
        case 'table':
          EditorInsertOrEditTable(false);
          break;
        case 'td':
        case 'th':
          EditorTableCellProperties();
          break;
        case 'ol':
        case 'ul':
        case 'dl':
        case 'li':
          goDoCommand("cmd_listProperties");
          break;
        case 'a':
          if (element.name)
          {
            goDoCommand("cmd_anchor");
          }
          else if(element.href)
          {
            goDoCommand("cmd_link");
          }
          break;
        default:
          doAdvancedProperties(element);
          break;
      }
    } else {
      
      try {
        element = GetCurrentEditor().getSelectedElement("href");
      } catch (e) {}
      if (element)
        goDoCommand("cmd_link");
    }
    window.content.focus();
  }
};



var nsSetSmiley =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon)
  {
    var smileyCode = aParams.getStringValue("state_attribute");

    var strSml;
    switch(smileyCode)
    {
        case ":-)": strSml="s1";
        break;
        case ":-(": strSml="s2";
        break;
        case ";-)": strSml="s3";
        break;
        case ":-P":
        case ":-p":
        case ":-b": strSml="s4";
        break;
        case ":-D": strSml="s5";
        break;
        case ":-[": strSml="s6";
        break;
        case ":-/":
        case ":/":
        case ":-\\":
        case ":\\": strSml="s7";
        break;
        case "=-O":
        case "=-o": strSml="s8";
        break;
        case ":-*": strSml="s9";
        break;
        case ">:o":
        case ">:-o": strSml="s10";
        break;
        case "8-)": strSml="s11";
        break;
        case ":-$": strSml="s12";
        break;
        case ":-!": strSml="s13";
        break;
        case "O:-)":
        case "o:-)": strSml="s14";
        break;
        case ":'(": strSml="s15";
        break;
        case ":-X":
        case ":-x": strSml="s16";
        break;
        default:	strSml="";
        break;
    }

    try
    {
      var editor = GetCurrentEditor();
      var selection = editor.selection;
      var extElement = editor.createElementWithDefaults("span");
      extElement.setAttribute("class", "moz-smiley-" + strSml);

      var intElement = editor.createElementWithDefaults("span");
      if (!intElement)
        return;

      
      var smileButMenu = document.getElementById('smileButtonMenu');      
      if (smileButMenu.getAttribute("padwithspace"))
         smileyCode = " " + smileyCode + " ";

      var txtElement =  editor.document.createTextNode(smileyCode);
      if (!txtElement)
        return;

      intElement.appendChild (txtElement);
      extElement.appendChild (intElement);


      editor.insertElementAtSelection(extElement,true);
      window.content.focus();		

    } 
    catch (e) 
    {
        dump("Exception occured in smiley InsertElementAtSelection\n");
    }
  },
  
  doCommand: function(aCommand) {}
};


function doAdvancedProperties(element)
{
  if (element)
  {
    window.openDialog("chrome://editor/content/EdAdvancedEdit.xul", "_blank", "chrome,close,titlebar,modal,resizable=yes", "", element);
    window.content.focus();
  }
}

var nsAdvancedPropertiesCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    try {
      var element = GetCurrentEditor().getSelectedElement("");
      doAdvancedProperties(element);
    } catch (e) {}
  }
};


var nsColorPropertiesCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    window.openDialog("chrome://editor/content/EdColorProps.xul","_blank", "chrome,close,titlebar,modal", ""); 
    UpdateDefaultColors(); 
    window.content.focus();
  }
};


var nsRemoveNamedAnchorsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    EditorRemoveTextProperty("name", "");
    window.content.focus();
  }
};



var nsEditLinkCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      var element = GetCurrentEditor().getSelectedElement("href");
      if (element)
        editPage(element.href, window, false);
    } catch (e) {}
    window.content.focus();
  }
};



var nsNormalModeCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsHTMLEditor() && IsDocumentEditable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    SetEditMode(kDisplayModeNormal);
  }
};

var nsAllTagsModeCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsHTMLEditor());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    SetEditMode(kDisplayModeAllTags);
  }
};

var nsHTMLSourceModeCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsHTMLEditor());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    SetEditMode(kDisplayModeSource);
  }
};

var nsPreviewModeCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsHTMLEditor());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    SetEditMode(kDisplayModePreview);
  }
};


var nsInsertOrEditTableCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return (IsDocumentEditable() && IsEditingRenderedHTML());
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    if (IsInTableCell())
      EditorTableCellProperties();
    else
      EditorInsertOrEditTable(true);
  }
};


var nsEditTableCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    EditorInsertOrEditTable(false);
  }
};


var nsSelectTableCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().selectTable();
    } catch(e) {}
    window.content.focus();
  }
};

var nsSelectTableRowCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().selectTableRow();
    } catch(e) {}
    window.content.focus();
  }
};

var nsSelectTableColumnCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().selectTableColumn();
    } catch(e) {}
    window.content.focus();
  }
};

var nsSelectTableCellCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().selectTableCell();
    } catch(e) {}
    window.content.focus();
  }
};

var nsSelectAllTableCellsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().selectAllTableCells();
    } catch(e) {}
    window.content.focus();
  }
};


var nsInsertTableCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsDocumentEditable() && IsEditingRenderedHTML();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    EditorInsertTable();
  }
};

var nsInsertTableRowAboveCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().insertTableRow(1, false);
    } catch(e) {}
    window.content.focus();
  }
};

var nsInsertTableRowBelowCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().insertTableRow(1, true);
    } catch(e) {}
    window.content.focus();
  }
};

var nsInsertTableColumnBeforeCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().insertTableColumn(1, false);
    } catch(e) {}
    window.content.focus();
  }
};

var nsInsertTableColumnAfterCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().insertTableColumn(1, true);
    } catch(e) {}
    window.content.focus();
  }
};

var nsInsertTableCellBeforeCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().insertTableCell(1, false);
    } catch(e) {}
    window.content.focus();
  }
};

var nsInsertTableCellAfterCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().insertTableCell(1, true);
    } catch(e) {}
    window.content.focus();
  }
};


var nsDeleteTableCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().deleteTable();
    } catch(e) {}
    window.content.focus();
  }
};

var nsDeleteTableRowCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var rows = GetNumberOfContiguousSelectedRows();
    
    if (rows == 0)
      rows = 1;

    try {
      var editor = GetCurrentTableEditor();
      editor.beginTransaction();

      
      while (rows)
      {
        editor.deleteTableRow(rows);
        rows = GetNumberOfContiguousSelectedRows();
      }
    } finally { editor.endTransaction(); }
    window.content.focus();
  }
};

var nsDeleteTableColumnCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    var columns = GetNumberOfContiguousSelectedColumns();
    
    if (columns == 0)
      columns = 1;

    try {
      var editor = GetCurrentTableEditor();
      editor.beginTransaction();

      
      while (columns)
      {
        editor.deleteTableColumn(columns);
        columns = GetNumberOfContiguousSelectedColumns();
      }
    } finally { editor.endTransaction(); }
    window.content.focus();
  }
};

var nsDeleteTableCellCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().deleteTableCell(1);   
    } catch (e) {}
    window.content.focus();
  }
};

var nsDeleteTableCellContentsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTableCell();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().deleteTableCellContents();
    } catch (e) {}
    window.content.focus();
  }
};



var nsNormalizeTableCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    try {
      GetCurrentTableEditor().normalizeTable(null);   
    } catch (e) {}
    window.content.focus();
  }
};


var nsJoinTableCellsCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    if (IsDocumentEditable() && IsEditingRenderedHTML())
    {
      try {
        var editor = GetCurrentTableEditor();
        var tagNameObj = { value: "" };
        var countObj = { value: 0 };
        var cell = editor.getSelectedOrParentTableElement(tagNameObj, countObj);

        
        
        
        
        if( cell && (tagNameObj.value == "td"))
        {
          
          if (countObj.value > 1) return true;

          var colSpan = cell.getAttribute("colspan");

          
          
          if (!colSpan)
            colSpan = Number(1);
          else
            colSpan = Number(colSpan);

          var rowObj = { value: 0 };
          var colObj = { value: 0 };
          editor.getCellIndexes(cell, rowObj, colObj);

          
          
          
          return (colSpan && editor.getCellAt(null, rowObj.value,
                                              colObj.value + colSpan));
        }
      } catch (e) {}
    }
    return false;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    try {
      GetCurrentTableEditor().joinTableCells(false);
    } catch (e) {}
    window.content.focus();
  }
};


var nsSplitTableCellCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    if (IsDocumentEditable() && IsEditingRenderedHTML())
    {
      var tagNameObj = { value: "" };
      var countObj = { value: 0 };
      var cell;
      try {
        cell = GetCurrentTableEditor().getSelectedOrParentTableElement(tagNameObj, countObj);
      } catch (e) {}

      
      
      if ( cell && (tagNameObj.value == "td") && 
           countObj.value <= 1 &&
           IsSelectionInOneCell() )
      {
        var colSpan = cell.getAttribute("colspan");
        var rowSpan = cell.getAttribute("rowspan");
        if (!colSpan) colSpan = 1;
        if (!rowSpan) rowSpan = 1;
        return (colSpan > 1  || rowSpan > 1 ||
                colSpan == 0 || rowSpan == 0);
      }
    }
    return false;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    try {
      GetCurrentTableEditor().splitTableCell();
    } catch (e) {}
    window.content.focus();
  }
};


var nsTableOrCellColorCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return IsInTable();
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    EditorSelectColor("TableOrCell");
  }
};


var nsPreferencesCommand =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    goPreferences('editor', 'chrome://editor/content/pref-composer.xul','editor');
    window.content.focus();
  }
};


var nsFinishHTMLSource =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    FinishHTMLSource();
  }
};

var nsCancelHTMLSource =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    return true;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    
    CancelHTMLSource();
  }
};

var nsConvertToTable =
{
  isCommandEnabled: function(aCommand, dummy)
  {
    if (IsDocumentEditable() && IsEditingRenderedHTML())
    {
      var selection;
      try {
        selection = GetCurrentEditor().selection;
      } catch (e) {}

      if (selection && !selection.isCollapsed)
      {
        
        var element;
        try {
          element = GetCurrentEditor().getSelectedElement("");
        } catch (e) {}
        if (element)
        {
          var name = element.nodeName.toLowerCase();
          if (name == "td" ||
              name == "th" ||
              name == "caption" ||
              name == "table")
            return false;
        }

        
        
        if ( GetParentTableCell(selection.focusNode) !=
             GetParentTableCell(selection.anchorNode) )
          return false
      
        return true;
      }
    }
    return false;
  },

  getCommandStateParams: function(aCommand, aParams, aRefCon) {},
  doCommandParams: function(aCommand, aParams, aRefCon) {},

  doCommand: function(aCommand)
  {
    if (this.isCommandEnabled())
    {
      window.openDialog("chrome://editor/content/EdConvertToTable.xul","_blank", "chrome,close,titlebar,modal")
    }
    window.content.focus();
  }
};

