











































var gComposerWindowControllerID = 0;
var prefAuthorString = "";

const kDisplayModeNormal = 0;
const kDisplayModeAllTags = 1;
const kDisplayModeSource = 2;
const kDisplayModePreview = 3;
const kDisplayModeMenuIDs = ["viewNormalMode", "viewAllTagsMode", "viewSourceMode", "viewPreviewMode"];
const kDisplayModeTabIDS = ["NormalModeButton", "TagModeButton", "SourceModeButton", "PreviewModeButton"];
const kNormalStyleSheet = "chrome://editor/content/EditorContent.css";
const kAllTagsStyleSheet = "chrome://editor/content/EditorAllTags.css";
const kParagraphMarksStyleSheet = "chrome://editor/content/EditorParagraphMarks.css";
const kContentEditableStyleSheet = "resource:/res/contenteditable.css";

const kTextMimeType = "text/plain";
const kHTMLMimeType = "text/html";

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;

var gPreviousNonSourceDisplayMode = 1;
var gEditorDisplayMode = -1;
var gDocWasModified = false;  
var gContentWindow = 0;
var gSourceContentWindow = 0;
var gSourceTextEditor = null;
var gContentWindowDeck;
var gFormatToolbar;
var gFormatToolbarHidden = false;
var gViewFormatToolbar;
var gColorObj = { LastTextColor:"", LastBackgroundColor:"", LastHighlightColor:"",
                  Type:"", SelectedType:"", NoDefault:false, Cancel:false,
                  HighlightColor:"", BackgroundColor:"", PageColor:"",
                  TextColor:"", TableColor:"", CellColor:""
                };
var gDefaultTextColor = "";
var gDefaultBackgroundColor = "";
var gCSSPrefListener;
var gEditorToolbarPrefListener;
var gReturnInParagraphPrefListener;
var gPrefs;
var gLocalFonts = null;

var gLastFocusNode = null;
var gLastFocusNodeWasSelected = false;


var gFontSizeNames = ["xx-small","x-small","small","medium","large","x-large","xx-large"];

const nsIFilePicker = Components.interfaces.nsIFilePicker;

const kEditorToolbarPrefs = "editor.toolbars.showbutton.";
const kUseCssPref         = "editor.use_css";
const kCRInParagraphsPref = "editor.CR_creates_new_p";

function getEngineWebBrowserPrint()
{
  return content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                .getInterface(Components.interfaces.nsIWebBrowserPrint);
}

function ShowHideToolbarSeparators(toolbar) {
  var childNodes = toolbar.childNodes;
  var separator = null;
  var hideSeparator = true;
  for (var i = 0; childNodes[i].localName != "spacer"; i++) {
    if (childNodes[i].localName == "toolbarseparator") {
      if (separator)
        separator.hidden = true;
      separator = childNodes[i];
    } else if (!childNodes[i].hidden) {
      if (separator)
        separator.hidden = hideSeparator;
      separator = null;
      hideSeparator = false;
    }
  }
}

function ShowHideToolbarButtons()
{
  var array = gPrefs.getChildList(kEditorToolbarPrefs, {});
  for (var i in array) {
    var prefName = array[i];
    var id = prefName.substr(kEditorToolbarPrefs.length) + "Button";
    var button = document.getElementById(id);
    if (button)
      button.hidden = !gPrefs.getBoolPref(prefName);
  }
  ShowHideToolbarSeparators(document.getElementById("EditToolbar"));
  ShowHideToolbarSeparators(document.getElementById("FormatToolbar"));
}
  
function nsPrefListener(prefName)
{
  this.startup(prefName);
}


nsPrefListener.prototype =
{
  domain: "",
  startup: function(prefName)
  {
    this.domain = prefName;
    try {
      var pbi = pref.QueryInterface(Components.interfaces.nsIPrefBranch2);
      pbi.addObserver(this.domain, this, false);
    } catch(ex) {
      dump("Failed to observe prefs: " + ex + "\n");
    }
  },
  shutdown: function()
  {
    try {
      var pbi = pref.QueryInterface(Components.interfaces.nsIPrefBranch2);
      pbi.removeObserver(this.domain, this);
    } catch(ex) {
      dump("Failed to remove pref observers: " + ex + "\n");
    }
  },
  observe: function(subject, topic, prefName)
  {
    if (!IsHTMLEditor())
      return;
    
    if (topic != "nsPref:changed") return;
    
    var editor = GetCurrentEditor();
    if (prefName == kUseCssPref)
    {
      var cmd = document.getElementById("cmd_highlight");
      if (cmd) {
        var useCSS = gPrefs.getBoolPref(prefName);

        if (useCSS && editor) {
          var mixedObj = {};
          var state = editor.getHighlightColorState(mixedObj);
          cmd.setAttribute("state", state);
          cmd.collapsed = false;
        }      
        else {
          cmd.setAttribute("state", "transparent");
          cmd.collapsed = true;
        }

        if (editor)
          editor.isCSSEnabled = useCSS;
      }
    }
    else if (prefName.substr(0, kEditorToolbarPrefs.length) == kEditorToolbarPrefs)
    {
      var id = prefName.substr(kEditorToolbarPrefs.length) + "Button";
      var button = document.getElementById(id);
      if (button) {
        button.hidden = !gPrefs.getBoolPref(prefName);
        ShowHideToolbarSeparators(button.parentNode);
      }
    }
    else if (editor && (prefName == kCRInParagraphsPref))
      editor.returnInParagraphCreatesNewParagraph = gPrefs.getBoolPref(prefName);
  }
}

function AfterHighlightColorChange()
{
  if (!IsHTMLEditor())
    return;

  var button = document.getElementById("cmd_highlight");
  if (button) {
    var mixedObj = {};
    try {
      var state = GetCurrentEditor().getHighlightColorState(mixedObj);
      button.setAttribute("state", state);
      onHighlightColorChange();
    } catch (e) {}
  }      
}

function EditorOnLoad()
{
    
    if ( window.arguments && window.arguments[0] ) {
        
        
        document.getElementById( "args" ).setAttribute( "value", window.arguments[0] );
    }

    
    if ("arguments" in window && window.arguments.length > 1 && window.arguments[1]) {
      if (window.arguments[1].indexOf("charset=") != -1) {
        var arrayArgComponents = window.arguments[1].split("=");
        if (arrayArgComponents) {
          
          document.getElementById( "args" ).setAttribute("charset", arrayArgComponents[1]);
        }
      }
    }

    window.tryToClose = EditorCanClose;

    
    EditorStartup();

    
    try {
      gSourceContentWindow = document.getElementById("content-source");
      gSourceContentWindow.makeEditable("text", false);
      gSourceTextEditor = gSourceContentWindow.getEditor(gSourceContentWindow.contentWindow);
      gSourceTextEditor.QueryInterface(Components.interfaces.nsIPlaintextEditor);
      gSourceTextEditor.enableUndo(false);
      gSourceTextEditor.rootElement.style.fontFamily = "-moz-fixed";
      gSourceTextEditor.rootElement.style.whiteSpace = "pre";
      gSourceTextEditor.rootElement.style.margin = 0;
      var controller = Components.classes["@mozilla.org/embedcomp/base-command-controller;1"]
                                 .createInstance(Components.interfaces.nsIControllerContext);
      controller.init(null);
      controller.setCommandContext(gSourceContentWindow);
      gSourceContentWindow.contentWindow.controllers.insertControllerAt(0, controller);
      var commandTable = controller.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                   .getInterface(Components.interfaces.nsIControllerCommandTable);
      commandTable.registerCommand("cmd_find",        nsFindCommand);
      commandTable.registerCommand("cmd_findNext",    nsFindAgainCommand);
      commandTable.registerCommand("cmd_findPrev",    nsFindAgainCommand);
    } catch (e) { dump("makeEditable failed: "+e+"\n"); }
}

const gSourceTextListener =
{
  NotifyDocumentCreated: function NotifyDocumentCreated() {},
  NotifyDocumentWillBeDestroyed: function NotifyDocumentWillBeDestroyed() {},
  NotifyDocumentStateChanged: function NotifyDocumentStateChanged(isChanged)
  {
    window.updateCommands("save");
  }
};

const gSourceTextObserver =
{
  observe: function observe(aSubject, aTopic, aData)
  {
    
    window.updateCommands("undo");
  }
};

function TextEditorOnLoad()
{
    
    if ( window.arguments && window.arguments[0] ) {
        
        
        document.getElementById( "args" ).setAttribute( "value", window.arguments[0] );
    }
    
    EditorStartup();
}



function EditorCleanup()
{
  SwitchInsertCharToAnotherEditorOrClose();
}

var DocumentReloadListener =
{
  NotifyDocumentCreated: function() {},
  NotifyDocumentWillBeDestroyed: function() {},

  NotifyDocumentStateChanged:function( isNowDirty )
  {
    var editor = GetCurrentEditor();
    try {
      
      editor.removeDocumentStateListener( DocumentReloadListener );

      var charset = editor.documentCharacterSet;

      
      editor.documentCharacterSet = charset;

    } catch (e) {}
  }
};

function addEditorClickEventListener()
{
  try {
    var bodyelement = GetBodyElement();
    if (bodyelement)
      bodyelement.addEventListener("click", EditorClick, false);
  } catch (e) {}
}


var gEditorDocumentObserver =
{ 
  observe: function(aSubject, aTopic, aData)
  {
    
    var commandManager = GetCurrentCommandManager();
    if (commandManager != aSubject)
      return;

    var editor = GetCurrentEditor();
    switch(aTopic)
    {
      case "obs_documentCreated":
        
        gContentWindow = window.content;

        
        var params = newCommandParams();
        if (!params)
          return;

        try {
          commandManager.getCommandState(aTopic, gContentWindow, params);
          var errorStringId = 0;
          var editorStatus = params.getLongValue("state_data");
          if (!editor && editorStatus == nsIEditingSession.eEditorOK)
          {
            dump("\n ****** NO EDITOR BUT NO EDITOR ERROR REPORTED ******* \n\n");
            editorStatus = nsIEditingSession.eEditorErrorUnknown;
          }

          switch (editorStatus)
          {
            case nsIEditingSession.eEditorErrorCantEditFramesets:
              errorStringId = "CantEditFramesetMsg";
              break;
            case nsIEditingSession.eEditorErrorCantEditMimeType:
              errorStringId = "CantEditMimeTypeMsg";
              break;
            case nsIEditingSession.eEditorErrorUnknown:
              errorStringId = "CantEditDocumentMsg";
              break;
            
            
          }
          if (errorStringId)
            AlertWithTitle("", GetString(errorStringId));
        } catch(e) { dump("EXCEPTION GETTING obs_documentCreated state "+e+"\n"); }

        
        
        if (editorStatus)
          return; 

        if (!("InsertCharWindow" in window))
          window.InsertCharWindow = null;

        try {
          editor.QueryInterface(nsIEditorStyleSheets);

          
          editor.addOverrideStyleSheet(kNormalStyleSheet);

          
          
          editor.removeOverrideStyleSheet(kContentEditableStyleSheet);
        } catch (e) {}

        
        if (IsWebComposer())
        {
          InlineSpellCheckerUI.init(editor);
          document.getElementById('menu_inlinespellcheck').setAttribute('disabled', !InlineSpellCheckerUI.canSpellCheck);

          editor.returnInParagraphCreatesNewParagraph = gPrefs.getBoolPref(kCRInParagraphsPref);

          
          
          
          setTimeout(SetFocusOnStartup, 0);

          
          EditorSetDefaultPrefsAndDoctype();

          
          
          
          if (editor.contentsMIMEType == "text/plain")
          {
            try {
              GetCurrentEditorElement().editortype = "text";
            } catch (e) { dump (e)+"\n"; }

            
            HideItem("FormatToolbar");
            HideItem("EditModeToolbar");
            HideItem("formatMenu");
            HideItem("tableMenu");
            HideItem("menu_validate");
            HideItem("sep_validate");
            HideItem("previewButton");
            HideItem("imageButton");
            HideItem("linkButton");
            HideItem("namedAnchorButton");
            HideItem("hlineButton");
            HideItem("tableButton");

            HideItem("fileExportToText");
            HideItem("previewInBrowser");




            HideItem("menu_pasteNoFormatting"); 

            HideItem("cmd_viewFormatToolbar");
            HideItem("cmd_viewEditModeToolbar");

            HideItem("viewSep1");
            HideItem("viewNormalMode");
            HideItem("viewAllTagsMode");
            HideItem("viewSourceMode");
            HideItem("viewPreviewMode");

            HideItem("structSpacer");

            
            var menuPopup = document.getElementById("insertMenuPopup");
            if (menuPopup)
            {
              var children = menuPopup.childNodes;
              for (var i=0; i < children.length; i++) 
              {
                var item = children.item(i);
                if (item.id != "insertChars")
                  item.hidden = true;
              }
            }
          }
    
          
          UpdateWindowTitle();

          
          
          SetSaveAndPublishUI(GetDocumentUrl());

          
          SetDisplayMode(kDisplayModeNormal);
        }

        
        if (IsHTMLEditor())
        {
          addEditorClickEventListener();

          
          onFontColorChange();
          onBackgroundColorChange();
        }
        break;

      case "cmd_setDocumentModified":
        window.updateCommands("save");
        break;

      case "obs_documentWillBeDestroyed":
        dump("obs_documentWillBeDestroyed notification\n");
        break;

      case "obs_documentLocationChanged":
        
        
        if (editor)
          try {
            editor.updateBaseURL();
          } catch(e) { dump (e); }
        break;

      case "cmd_bold":
        
        
        window.updateCommands("style");
        window.updateCommands("undo");
        break;
    }
  }
}

function SetFocusOnStartup()
{
  gContentWindow.focus();
}

function EditorStartup()
{
  var ds = GetCurrentEditorElement().docShell;
  ds.useErrorPages = false;
  var root = ds.QueryInterface(Components.interfaces.nsIDocShellTreeItem).
    rootTreeItem.QueryInterface(Components.interfaces.nsIDocShell);

  root.QueryInterface(Components.interfaces.nsIDocShell).appType =
    Components.interfaces.nsIDocShell.APP_TYPE_EDITOR;

  var is_HTMLEditor = IsHTMLEditor();
  if (is_HTMLEditor)
  {
    
    gContentWindowDeck = document.getElementById("ContentWindowDeck");
    gFormatToolbar = document.getElementById("FormatToolbar");
    gViewFormatToolbar = document.getElementById("viewFormatToolbar");
  }

  
  GetPrefsService();

  
  EditorSharedStartup();

  
  
  
  SetupComposerWindowCommands();

  ShowHideToolbarButtons();
  gEditorToolbarPrefListener = new nsPrefListener(kEditorToolbarPrefs);

  gCSSPrefListener = new nsPrefListener(kUseCssPref);
  gReturnInParagraphPrefListener = new nsPrefListener(kCRInParagraphsPref);

  
  
  var cmd = document.getElementById("cmd_highlight");
  if (cmd) {
    var useCSS = gPrefs.getBoolPref(kUseCssPref);
    if (!useCSS && is_HTMLEditor) {
      cmd.collapsed = true;
    }
  }

  
  
  var url = document.getElementById("args").getAttribute("value");
  try {
    var charset = document.getElementById("args").getAttribute("charset");
    var contentViewer = GetCurrentEditorElement().docShell.contentViewer;
    contentViewer.QueryInterface(Components.interfaces.nsIMarkupDocumentViewer);
    contentViewer.defaultCharacterSet = charset;
    contentViewer.forceCharacterSet = charset;
  } catch (e) {}
  EditorLoadUrl(url);
}

function EditorLoadUrl(url)
{
  try {
    if (url)
      GetCurrentEditorElement().webNavigation.loadURI(url, 
             nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE,     
             null,                                         
             null,                                         
             null);
  } catch (e) { dump(" EditorLoadUrl failed: "+e+"\n"); }
}


function EditorSharedStartup()
{
  
  gContentWindow = window.content;

  
  if (IsHTMLEditor())
    SetupHTMLEditorCommands();
  else
    SetupTextEditorCommands();

  
  
  
  try {
    var commandManager = GetCurrentCommandManager();
    commandManager.addCommandObserver(gEditorDocumentObserver, "obs_documentCreated");
    commandManager.addCommandObserver(gEditorDocumentObserver, "cmd_setDocumentModified");
    commandManager.addCommandObserver(gEditorDocumentObserver, "obs_documentWillBeDestroyed");
    commandManager.addCommandObserver(gEditorDocumentObserver, "obs_documentLocationChanged");

    
    
    
    commandManager.addCommandObserver(gEditorDocumentObserver, "cmd_bold");
  } catch (e) { dump(e); }

  var isMac = (GetOS() == gMac);

  
  
  var tableKey = GetString(isMac ? "XulKeyMac" : "TableSelectKey");
  var dragStr = tableKey+GetString("Drag");
  var clickStr = tableKey+GetString("Click");

  var delStr = GetString(isMac ? "Clear" : "Del");

  SafeSetAttribute("menu_SelectCell", "acceltext", clickStr);
  SafeSetAttribute("menu_SelectRow", "acceltext", dragStr);
  SafeSetAttribute("menu_SelectColumn", "acceltext", dragStr);
  SafeSetAttribute("menu_SelectAllCells", "acceltext", dragStr);
  
  SafeSetAttribute("menu_DeleteCellContents", "acceltext", delStr);

  

  
  RemoveInapplicableUIElements();

  gPrefs = GetPrefs();

  
  var BrowserColors = GetDefaultBrowserColors();
  if (BrowserColors)
  {
    gDefaultTextColor = BrowserColors.TextColor;
    gDefaultBackgroundColor = BrowserColors.BackgroundColor;
  }

  
  gColorObj.LastTextColor = "";
  gColorObj.LastBackgroundColor = "";
  gColorObj.LastHighlightColor = "";
}


function EditorResetFontAndColorAttributes()
{
  try {  
    var editor = GetCurrentEditor();
    editor.rebuildDocumentFromSource("");
    
    
    editor.removeAllInlineProperties();
    document.getElementById("cmd_fontFace").setAttribute("state", "");
    gColorObj.LastTextColor = "";
    gColorObj.LastBackgroundColor = "";
    gColorObj.LastHighlightColor = "";
    document.getElementById("cmd_fontColor").setAttribute("state", "");
    document.getElementById("cmd_backgroundColor").setAttribute("state", "");
    UpdateDefaultColors();
  } catch (e) {}
}

function EditorShutdown()
{
  gEditorToolbarPrefListener.shutdown();
  gCSSPrefListener.shutdown();
  gReturnInParagraphPrefListener.shutdown();

  try {
    var commandManager = GetCurrentCommandManager();
    commandManager.removeCommandObserver(gEditorDocumentObserver, "obs_documentCreated");
    commandManager.removeCommandObserver(gEditorDocumentObserver, "obs_documentWillBeDestroyed");
    commandManager.removeCommandObserver(gEditorDocumentObserver, "obs_documentLocationChanged");
  } catch (e) { dump (e); }   
}

function SafeSetAttribute(nodeID, attributeName, attributeValue)
{
    var theNode = document.getElementById(nodeID);
    if (theNode)
        theNode.setAttribute(attributeName, attributeValue);
}

function DocumentHasBeenSaved()
{
  var fileurl = "";
  try {
    fileurl = GetDocumentUrl();
  } catch (e) {
    return false;
  }

  if (!fileurl || IsUrlAboutBlank(fileurl))
    return false;

  
  return true;
}

function CheckAndSaveDocument(command, allowDontSave)
{
  var document;
  try {
    
    var editor = GetCurrentEditor();
    document = editor.document;
    if (!document)
      return true;
  } catch (e) { return true; }

  if (!IsDocumentModified() && !IsHTMLSourceChanged())
    return true;

  
  
  top.document.commandDispatcher.focusedWindow.focus();  

  var scheme = GetScheme(GetDocumentUrl());
  var doPublish = (scheme && scheme != "file");

  var strID;
  switch (command)
  {
    case "cmd_close":
      strID = "BeforeClosing";
      break;
    case "cmd_preview":
      strID = "BeforePreview";
      break;
    case "cmd_editSendPage":
      strID = "SendPageReason";
      break;
    case "cmd_validate":
      strID = "BeforeValidate";
      break;
  }
    
  var reasonToSave = strID ? GetString(strID) : "";

  var title = document.title;
  if (!title)
    title = GetString("untitled");

  var dialogTitle = GetString(doPublish ? "PublishPage" : "SaveDocument");
  var dialogMsg = GetString(doPublish ? "PublishPrompt" : "SaveFilePrompt");
  dialogMsg = (dialogMsg.replace(/%title%/,title)).replace(/%reason%/,reasonToSave);

  var promptService = GetPromptService();
  if (!promptService)
    return false;

  var result = {value:0};
  var promptFlags = promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1;
  var button1Title = null;
  var button3Title = null;

  if (doPublish)
  {
    promptFlags += promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0;
    button1Title = GetString("Publish");
    button3Title = GetString("DontPublish");    
  }
  else
  {
    promptFlags += promptService.BUTTON_TITLE_SAVE * promptService.BUTTON_POS_0;
  }

  
  if (allowDontSave)
    promptFlags += doPublish ?
        (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2)
        : (promptService.BUTTON_TITLE_DONT_SAVE * promptService.BUTTON_POS_2);
  
  result = promptService.confirmEx(window, dialogTitle, dialogMsg, promptFlags,
                          button1Title, null, button3Title, null, {value:0});

  if (result == 0)
  {
    
    if (IsHTMLSourceChanged()) {
      try {
        FinishHTMLSource();
      } catch (e) { return false;}
    }

    if (doPublish)
    {
      
      
      
      gCommandAfterPublishing = command;
      goDoCommand("cmd_publish");
      return false;
    }

    
    var contentsMIMEType;
    if (IsHTMLEditor())
      contentsMIMEType = kHTMLMimeType;
    else
      contentsMIMEType = kTextMimeType;
    var success = SaveDocument(false, false, contentsMIMEType);
    return success;
  }

  if (result == 2) 
    return true;

  
  return false;
}



function EditorNewPlaintext()
{
  window.openDialog( "chrome://editor/content/TextEditorAppShell.xul",
                     "_blank",
                     "chrome,dialog=no,all",
                     "about:blank");
}



function EditorCanClose()
{
  

  
  var canClose = CheckAndSaveDocument("cmd_close", true);

  
  
  
  
  if (canClose && "InsertCharWindow" in window && window.InsertCharWindow)
    SwitchInsertCharToAnotherEditorOrClose();

  return canClose;
}



function EditorSetDocumentCharacterSet(aCharset)
{
  try {
    var editor = GetCurrentEditor();
    editor.documentCharacterSet = aCharset;
    var docUrl = GetDocumentUrl();
    if( !IsUrlAboutBlank(docUrl))
    {
      
      
      editor.addDocumentStateListener( DocumentReloadListener );
      EditorLoadUrl(docUrl);
    }
  } catch (e) {}
}


function updateCharsetPopupMenu(menuPopup)
{
  if (IsDocumentModified() && !IsDocumentEmpty())
  {
    for (var i = 0; i < menuPopup.childNodes.length; i++)
    {
      var menuItem = menuPopup.childNodes[i];
      menuItem.setAttribute('disabled', 'true');
    }
  }
}



function onParagraphFormatChange(paraMenuList, commandID)
{
  if (!paraMenuList)
    return;

  var commandNode = document.getElementById(commandID);
  var state = commandNode.getAttribute("state");

  
  if (state == "body")
    state = "";

  if (state == "mixed")
  {
    
    paraMenuList.selectedItem = null;
    paraMenuList.setAttribute("label",GetString('Mixed'));
  }
  else
  {
    var menuPopup = document.getElementById("ParagraphPopup");
    var menuItems = menuPopup.childNodes;
    for (var i=0; i < menuItems.length; i++)
    {
      var menuItem = menuItems.item(i);
      if ("value" in menuItem && menuItem.value == state)
      {
        paraMenuList.selectedItem = menuItem;
        break;
      }
    }
  }
}

function onFontFaceChange(fontFaceMenuList, commandID)
{
  var commandNode = document.getElementById(commandID);
  var state = commandNode.getAttribute("state");

  if (state == "mixed")
  {
    
    fontFaceMenuList.selectedItem = null;
    fontFaceMenuList.setAttribute("label",GetString('Mixed'));
  }
  else
  {
    var menuPopup = document.getElementById("FontFacePopup");
    var menuItems = menuPopup.childNodes;
    for (var i=0; i < menuItems.length; i++)
    {
      var menuItem = menuItems.item(i);
      if (menuItem.getAttribute("label") && ("value" in menuItem && menuItem.value.toLowerCase() == state.toLowerCase()))
      {
        fontFaceMenuList.selectedItem = menuItem;
        break;
      }
    }
  }
}

function EditorSelectFontSize()
{
  var select = document.getElementById("FontSizeSelect");
  if (select)
  {
    if (select.selectedIndex == -1)
      return;

    EditorSetFontSize(gFontSizeNames[select.selectedIndex]);
  }
}

function onFontSizeChange(fontSizeMenulist, commandID)
{
  
  var newIndex = 2;
  var size = fontSizeMenulist.getAttribute("size");
  if ( size == "mixed")
  {
    
    newIndex = -1;
  }
  else
  {
    for (var i = 0; i < gFontSizeNames.length; i++)
    {
      if( gFontSizeNames[i] == size )
      {
        newIndex = i;
        break;
      }
    }
  }
  if (fontSizeMenulist.selectedIndex != newIndex)
    fontSizeMenulist.selectedIndex = newIndex;
}

function EditorSetFontSize(size)
{
  if( size == "0" || size == "normal" ||
      size == "medium" )
  {
    EditorRemoveTextProperty("font", "size");
    
    
    EditorRemoveTextProperty("small", "");
    EditorRemoveTextProperty("big", "");
  } else {
    
    switch (size)
    {
      case "xx-small":
      case "x-small":
        size = "-2";
        break;
      case "small":
        size = "-1";
        break;
      case "large":
        size = "+1";
        break;
      case "x-large":
        size = "+2";
        break;
      case "xx-large":
        size = "+3";
        break;
    }
    EditorSetTextProperty("font", "size", size);
  }
  gContentWindow.focus();
}

function initFontFaceMenu(menuPopup)
{
  initLocalFontFaceMenu(menuPopup);

  if (menuPopup)
  {
    var children = menuPopup.childNodes;
    if (!children) return;

    var firstHas = { value: false };
    var anyHas = { value: false };
    var allHas = { value: false };

    
    

    
    EditorGetTextProperty("tt", "", "", firstHas, anyHas, allHas);
    children[1].setAttribute("checked", allHas.value);

    if (!anyHas.value)
      EditorGetTextProperty("font", "face", "", firstHas, anyHas, allHas);

    children[0].setAttribute("checked", !anyHas.value);

    
    for (var i = 3; i < children.length; i++)
    {
      var menuItem = children[i];
      var faceType = menuItem.getAttribute("value");

      if (faceType)
      {
        EditorGetTextProperty("font", "face", faceType, firstHas, anyHas, allHas);

        
        if (allHas.value)
        {
          menuItem.setAttribute("checked", "true");
          break;
        }

        
        menuItem.removeAttribute("checked");
      }
    }
  }
}

const kFixedFontFaceMenuItems = 7; 

function initLocalFontFaceMenu(menuPopup)
{
  if (!gLocalFonts)
  {
    
    try 
    {
      var enumerator = Components.classes["@mozilla.org/gfx/fontenumerator;1"]
                                 .getService(Components.interfaces.nsIFontEnumerator);
      var localFontCount = { value: 0 }
      gLocalFonts = enumerator.EnumerateAllFonts(localFontCount);
    }
    catch(e) { }
  }
  
  var useRadioMenuitems = (menuPopup.parentNode.localName == "menu"); 
  if (menuPopup.childNodes.length == kFixedFontFaceMenuItems) 
  {
    if (gLocalFonts.length == 0) {
      menuPopup.childNodes[kFixedFontFaceMenuItems - 1].hidden = true;
    }
    for (var i = 0; i < gLocalFonts.length; ++i)
    {
      if (gLocalFonts[i] != "")
      {
        var itemNode = document.createElementNS(XUL_NS, "menuitem");
        itemNode.setAttribute("label", gLocalFonts[i]);
        itemNode.setAttribute("value", gLocalFonts[i]);
        if (useRadioMenuitems) {
          itemNode.setAttribute("type", "radio");
          itemNode.setAttribute("name", "2");
          itemNode.setAttribute("observes", "cmd_renderedHTMLEnabler");
        }
        menuPopup.appendChild(itemNode);
      }
    }
  }
}
 

function initFontSizeMenu(menuPopup)
{
  if (menuPopup)
  {
    var children = menuPopup.childNodes;
    if (!children) return;

    var firstHas = { value: false };
    var anyHas = { value: false };
    var allHas = { value: false };

    var sizeWasFound = false;

    
    

    
    
    
    var menuItem = children[0];
    if (menuItem)
    {
      EditorGetTextProperty("small", "", "", firstHas, anyHas, allHas);
      menuItem.setAttribute("checked", allHas.value);
      sizeWasFound = anyHas.value;
    }

    menuItem = children[1];
    if (menuItem)
    {
      EditorGetTextProperty("big", "", "", firstHas, anyHas, allHas);
      menuItem.setAttribute("checked", allHas.value);
      sizeWasFound |= anyHas.value;
    }

    
    var menuIndex = 3;
    
    var mediumIndex = 5;

    
    for (var i = -2; i <= 3; i++)
    {
      menuItem = children[menuIndex];

      
      
      
      if (menuItem && (i != 0))
      {
        var sizeString = (i <= 0) ? String(i) : ("+" + String(i));
        EditorGetTextProperty("font", "size", sizeString, firstHas, anyHas, allHas);
        
        menuItem.setAttribute("checked", allHas.value);
        
        sizeWasFound |= anyHas.value;
      }
      menuIndex++;
    }

    
    
    children[mediumIndex].setAttribute("checked", !sizeWasFound);
  }
}

function onHighlightColorChange()
{
  var commandNode = document.getElementById("cmd_highlight");
  if (commandNode)
  {
    var color = commandNode.getAttribute("state");
    var button = document.getElementById("HighlightColorButton");
    if (button)
    {
      
      if (!color)
        color = "transparent" ;

      button.setAttribute("style", "background-color:"+color+" !important");
    }
  }
}

function onFontColorChange()
{
  var commandNode = document.getElementById("cmd_fontColor");
  if (commandNode)
  {
    var color = commandNode.getAttribute("state");
    var button = document.getElementById("TextColorButton");
    if (button)
    {
      
      if (!color)
        color = gDefaultTextColor;
      button.setAttribute("style", "background-color:"+color);
    }
  }
}

function onBackgroundColorChange()
{
  var commandNode = document.getElementById("cmd_backgroundColor");
  if (commandNode)
  {
    var color = commandNode.getAttribute("state");
    var button = document.getElementById("BackgroundColorButton");
    if (button)
    {
      if (!color)
        color = gDefaultBackgroundColor;

      button.setAttribute("style", "background-color:"+color);
    }
  }
}


function UpdateDefaultColors()
{
  var BrowserColors = GetDefaultBrowserColors();
  var bodyelement = GetBodyElement();
  var defTextColor = gDefaultTextColor;
  var defBackColor = gDefaultBackgroundColor;

  if (bodyelement)
  {
    var color = bodyelement.getAttribute("text");
    if (color)
      gDefaultTextColor = color;
    else if (BrowserColors)
      gDefaultTextColor = BrowserColors.TextColor;

    color = bodyelement.getAttribute("bgcolor");
    if (color)
      gDefaultBackgroundColor = color;
    else if (BrowserColors)
      gDefaultBackgroundColor = BrowserColors.BackgroundColor;
  }

  
  if (defTextColor != gDefaultTextColor)
  {
    goUpdateCommandState("cmd_fontColor");
    onFontColorChange();
  }
  if (defBackColor != gDefaultBackgroundColor)
  {
    goUpdateCommandState("cmd_backgroundColor");
    onBackgroundColorChange();
  }
}

function GetBackgroundElementWithColor()
{
  var editor = GetCurrentTableEditor();
  if (!editor)
    return null;

  gColorObj.Type = "";
  gColorObj.PageColor = "";
  gColorObj.TableColor = "";
  gColorObj.CellColor = "";
  gColorObj.BackgroundColor = "";
  gColorObj.SelectedType = "";

  var tagNameObj = { value: "" };
  var element;
  try {
    element = editor.getSelectedOrParentTableElement(tagNameObj, {value:0});
  }
  catch(e) {}

  if (element && tagNameObj && tagNameObj.value)
  {
    gColorObj.BackgroundColor = GetHTMLOrCSSStyleValue(element, "bgcolor", "background-color");
    gColorObj.BackgroundColor = ConvertRGBColorIntoHEXColor(gColorObj.BackgroundColor);
    if (tagNameObj.value.toLowerCase() == "td")
    {
      gColorObj.Type = "Cell";
      gColorObj.CellColor = gColorObj.BackgroundColor;

      
      var table = GetParentTable(element);
      gColorObj.TableColor = GetHTMLOrCSSStyleValue(table, "bgcolor", "background-color");
      gColorObj.TableColor = ConvertRGBColorIntoHEXColor(gColorObj.TableColor);
    }
    else
    {
      gColorObj.Type = "Table";
      gColorObj.TableColor = gColorObj.BackgroundColor;
    }
    gColorObj.SelectedType = gColorObj.Type;
  }
  else
  {
    var IsCSSPrefChecked = gPrefs.getBoolPref(kUseCssPref);
    if (IsCSSPrefChecked && IsHTMLEditor())
    {
      var selection = editor.selection;
      if (selection)
      {
        element = selection.focusNode;
        while (!editor.nodeIsBlock(element))
          element = element.parentNode;
      }
      else
      {
        element = GetBodyElement();
      }
    }
    else
    {
      element = GetBodyElement();
    }
    if (element)
    {
      gColorObj.Type = "Page";
      gColorObj.BackgroundColor = GetHTMLOrCSSStyleValue(element, "bgcolor", "background-color");
      if (gColorObj.BackgroundColor == "")
      {
        gColorObj.BackgroundColor = "transparent";
      }
      else
      {
        gColorObj.BackgroundColor = ConvertRGBColorIntoHEXColor(gColorObj.BackgroundColor);
      }
      gColorObj.PageColor = gColorObj.BackgroundColor;
    }
  }
  return element;
}

function SetSmiley(smileyText)
{
  try {
    GetCurrentEditor().insertText(smileyText);
    gContentWindow.focus();
  }
  catch(e) {}
}

function EditorSelectColor(colorType, mouseEvent)
{
  var editor = GetCurrentEditor();
  if (!editor || !gColorObj)
    return;

  
  var useLastColor = mouseEvent ? ( mouseEvent.button == 0 && mouseEvent.shiftKey ) : false;
  var element;
  var table;
  var currentColor = "";
  var commandNode;

  if (!colorType)
    colorType = "";

  if (colorType == "Text")
  {
    gColorObj.Type = colorType;

    
    commandNode = document.getElementById("cmd_fontColor");
    currentColor = commandNode.getAttribute("state");
    currentColor = ConvertRGBColorIntoHEXColor(currentColor);
    gColorObj.TextColor = currentColor;

    if (useLastColor && gColorObj.LastTextColor )
      gColorObj.TextColor = gColorObj.LastTextColor;
    else
      useLastColor = false;
  }
  else if (colorType == "Highlight")
  {
    gColorObj.Type = colorType;

    
    commandNode = document.getElementById("cmd_highlight");
    currentColor = commandNode.getAttribute("state");
    currentColor = ConvertRGBColorIntoHEXColor(currentColor);
    gColorObj.HighlightColor = currentColor;

    if (useLastColor && gColorObj.LastHighlightColor )
      gColorObj.HighlightColor = gColorObj.LastHighlightColor;
    else
      useLastColor = false;
  }
  else
  {
    element = GetBackgroundElementWithColor();
    if (!element)
      return;

    
    if (gColorObj.Type == "Table")
      table = element;
    else if (gColorObj.Type == "Cell")
      table = GetParentTable(element);

    
    currentColor = gColorObj.BackgroundColor;

    if (colorType == "TableOrCell" || colorType == "Cell")
    {
      if (gColorObj.Type == "Cell")
        gColorObj.Type = colorType;
      else if (gColorObj.Type != "Table")
        return;
    }
    else if (colorType == "Table" && gColorObj.Type == "Page")
      return;

    if (colorType == "" && gColorObj.Type == "Cell")
    {
      
      
      gColorObj.Type = "TableOrCell";
    }

    if (useLastColor && gColorObj.LastBackgroundColor )
      gColorObj.BackgroundColor = gColorObj.LastBackgroundColor;
    else
      useLastColor = false;
  }
  
  colorType = gColorObj.Type;

  if (!useLastColor)
  {
    
    gColorObj.NoDefault = false;

    
    
    window.openDialog("chrome://editor/content/EdColorPicker.xul", "_blank", "chrome,close,titlebar,modal", "", gColorObj);

    
    if (gColorObj.Cancel)
      return;
  }

  if (gColorObj.Type == "Text")
  {
    if (currentColor != gColorObj.TextColor)
    {
      if (gColorObj.TextColor)
        EditorSetTextProperty("font", "color", gColorObj.TextColor);
      else
        EditorRemoveTextProperty("font", "color");
    }
    
    goUpdateCommandState("cmd_fontColor");
  }
  else if (gColorObj.Type == "Highlight")
  {
    if (currentColor != gColorObj.HighlightColor)
    {
      if (gColorObj.HighlightColor)
        EditorSetTextProperty("font", "bgcolor", gColorObj.HighlightColor);
      else
        EditorRemoveTextProperty("font", "bgcolor");
    }
    
    goUpdateCommandState("cmd_highlight");
  }
  else if (element)
  {
    if (gColorObj.Type == "Table")
    {
      
      
      if (table)
      {
        var bgcolor = table.getAttribute("bgcolor");
        if (bgcolor != gColorObj.BackgroundColor)
        try {
          if (gColorObj.BackgroundColor)
            editor.setAttributeOrEquivalent(table, "bgcolor", gColorObj.BackgroundColor, false);
          else
            editor.removeAttributeOrEquivalent(table, "bgcolor", false);
        } catch (e) {}
      }
    }
    else if (currentColor != gColorObj.BackgroundColor && IsHTMLEditor())
    {
      editor.beginTransaction();
      try
      {
        editor.setBackgroundColor(gColorObj.BackgroundColor);

        if (gColorObj.Type == "Page" && gColorObj.BackgroundColor)
        {
          
          
          
          var bodyelement = GetBodyElement();
          if (bodyelement)
          {
            var defColors = GetDefaultBrowserColors();
            if (defColors)
            {
              if (!bodyelement.getAttribute("text"))
                editor.setAttributeOrEquivalent(bodyelement, "text", defColors.TextColor, false);

              
              
              if (!bodyelement.getAttribute("link"))
                editor.setAttribute(bodyelement, "link", defColors.LinkColor);

              if (!bodyelement.getAttribute("alink"))
                editor.setAttribute(bodyelement, "alink", defColors.ActiveLinkColor);

              if (!bodyelement.getAttribute("vlink"))
                editor.setAttribute(bodyelement, "vlink", defColors.VisitedLinkColor);
            }
          }
        }
      }
      catch(e) {}

      editor.endTransaction();
    }

    goUpdateCommandState("cmd_backgroundColor");
  }
  gContentWindow.focus();
}

function GetParentTable(element)
{
  var node = element;
  while (node)
  {
    if (node.nodeName.toLowerCase() == "table")
      return node;

    node = node.parentNode;
  }
  return node;
}

function GetParentTableCell(element)
{
  var node = element;
  while (node)
  {
    if (node.nodeName.toLowerCase() == "td" || node.nodeName.toLowerCase() == "th")
      return node;

    node = node.parentNode;
  }
  return node;
}

function EditorDblClick(event)
{
  
  
  if (event.explicitOriginalTarget)
  {
    
    var element;
    try {
      element = event.explicitOriginalTarget.QueryInterface(
                    Components.interfaces.nsIDOMElement);
    } catch (e) {}

     
    if (!element)
      try {
        element = GetCurrentEditor().getSelectedElement("href");
      } catch (e) {}

    if (element)
    {
      goDoCommand("cmd_objectProperties");  
      event.preventDefault();
    }
  }
}

function EditorClick(event)
{
  if (!event)
    return;

  if (event.detail == 2)
  {
    EditorDblClick(event);
    return;
  }

  
  
  
  if (IsWebComposer() && event.explicitOriginalTarget && IsHTMLEditor() &&
      gEditorDisplayMode == kDisplayModeAllTags)
  {
    try
    {
      
      
      var element = event.explicitOriginalTarget.QueryInterface(
                        Components.interfaces.nsIDOMElement);
      var name = element.localName.toLowerCase();
      if (name != "body" && name != "table" &&
          name != "td" && name != "th" && name != "caption" && name != "tr")
      {          
        GetCurrentEditor().selectElement(event.explicitOriginalTarget);
        event.preventDefault();
      }
    } catch (e) {}
  }
}







function GetObjectForProperties()
{
  var editor = GetCurrentEditor();
  if (!editor || !IsHTMLEditor())
    return null;

  var element;
  try {
    element = editor.getSelectedElement("");
  } catch (e) {}
  if (element)
    return element;

  
  

  var anchorNode
  var node;
  try {
    anchorNode = editor.selection.anchorNode;
    if (anchorNode.firstChild)
    {
      
      var offset = editor.selection.anchorOffset;
      
      
      node = anchorNode.childNodes.item(offset);
    }
    if (!node)
      node = anchorNode;
  } catch (e) {}

  while (node)
  {
    if (node.nodeName)
    {
      var nodeName = node.nodeName.toLowerCase();

      
      if (nodeName == "body") break;

      if ((nodeName == "a" && node.href) ||
          nodeName == "ol" || nodeName == "ul" || nodeName == "dl" ||
          nodeName == "td" || nodeName == "th" ||
          nodeName == "table")
      {
        return node;
      }
    }
    node = node.parentNode;
  }
  return null;
}

function SetEditMode(mode)
{
  if (!IsHTMLEditor())
    return;

  var bodyElement = GetBodyElement();
  if (!bodyElement)
  {
    dump("SetEditMode: We don't have a body node!\n");
    return;
  }

  
  var editor = GetCurrentEditor();
  var inlineSpellCheckItem = document.getElementById('menu_inlinespellcheck');

  
  
  var previousMode = gEditorDisplayMode;
  if (!SetDisplayMode(mode))
    return;

  if (mode == kDisplayModeSource)
  {
    
    var domdoc;
    try { domdoc = editor.document; } catch (e) { dump( e + "\n");}
    if (domdoc)
    {
      var doctypeNode = document.getElementById("doctype-text");
      var dt = domdoc.doctype;
      if (doctypeNode)
      {
        if (dt)
        {
          doctypeNode.collapsed = false;
          var doctypeText = "<!DOCTYPE " + domdoc.doctype.name;
          if (dt.publicId)
            doctypeText += " PUBLIC \"" + domdoc.doctype.publicId;
          if (dt.systemId)
            doctypeText += " "+"\"" + dt.systemId;
          doctypeText += "\">"
          doctypeNode.setAttribute("value", doctypeText);
        }
        else
          doctypeNode.collapsed = true;
      }
    }
    

    var flags = (editor.documentCharacterSet == "ISO-8859-1")
      ? kOutputEncodeLatin1Entities
      : kOutputEncodeBasicEntities;
    try { 
      var encodeEntity = gPrefs.getCharPref("editor.encode_entity");
      switch (encodeEntity) {
        case "basic"  : flags = kOutputEncodeBasicEntities; break;
        case "latin1" : flags = kOutputEncodeLatin1Entities; break;
        case "html"   : flags = kOutputEncodeHTMLEntities; break;
        case "none"   : flags = 0;     break;
      }
    } catch (e) { }

    try { 
      var prettyPrint = gPrefs.getBoolPref("editor.prettyprint");
      if (prettyPrint)
        flags |= kOutputFormatted;

    } catch (e) {}

    flags |= kOutputLFLineBreak;
    var source = editor.outputToString(kHTMLMimeType, flags);
    var start = source.search(/<html/i);
    if (start == -1) start = 0;
    gSourceTextEditor.insertText(source.slice(start));
    gSourceTextEditor.resetModificationCount();
    gSourceTextEditor.addDocumentStateListener(gSourceTextListener);
    gSourceTextEditor.enableUndo(true);
    gSourceContentWindow.commandManager.addCommandObserver(gSourceTextObserver, "cmd_undo");
    gSourceContentWindow.contentWindow.focus();
    goDoCommand("cmd_moveTop");
  }
  else if (previousMode == kDisplayModeSource)
  {
    
    if (IsHTMLSourceChanged())
    {
      
      InlineSpellCheckerUI.enabled = false;
      inlineSpellCheckItem.removeAttribute('checked');

      
      
      
      try {
        editor.transactionManager.maxTransactionCount = 1;
      } catch (e) {}

      editor.beginTransaction();
      try {
        
        
        source = gSourceTextEditor.outputToString(kTextMimeType, kOutputLFLineBreak);
        editor.rebuildDocumentFromSource(source);

        
        
        var title = "";
        var titlenodelist = editor.document.getElementsByTagName("title");
        if (titlenodelist)
        {
          var titleNode = titlenodelist.item(0);
          if (titleNode && titleNode.firstChild && titleNode.firstChild.data)
            title = titleNode.firstChild.data;
        }
        if (editor.document.title != title)
          SetDocumentTitle(title);

      } catch (ex) {
        dump(ex);
      }
      editor.endTransaction();

      
      try {
        editor.transactionManager.maxTransactionCount = -1;
      } catch (e) {}
    }

    
    gSourceContentWindow.commandManager.removeCommandObserver(gSourceTextObserver, "cmd_undo");
    gSourceTextEditor.removeDocumentStateListener(gSourceTextListener);
    gSourceTextEditor.enableUndo(false);
    gSourceTextEditor.selectAll();
    gSourceTextEditor.deleteSelection(gSourceTextEditor.eNone);
    gSourceTextEditor.resetModificationCount();

    gContentWindow.focus();
  }

  switch (mode) {
    case kDisplayModePreview:
      
      InlineSpellCheckerUI.enabled = false;
      inlineSpellCheckItem.removeAttribute('checked');
      
    case kDisplayModeSource:
      inlineSpellCheckItem.setAttribute('disabled', 'true');
      break;
    default:
      inlineSpellCheckItem.setAttribute('disabled', !InlineSpellCheckerUI.canSpellCheck);
      break;
  }
}

function CancelHTMLSource()
{
  
  gSourceTextEditor.resetModificationCount();
  SetDisplayMode(gPreviousNonSourceDisplayMode);
}

function FinishHTMLSource()
{
  
  
  if (IsInHTMLSourceMode())
  {
    var htmlSource = gSourceTextEditor.outputToString(kTextMimeType, kOutputLFLineBreak);
    if (htmlSource.length > 0)
    {
      var beginHead = htmlSource.indexOf("<head");
      if (beginHead == -1)
      {
        AlertWithTitle(GetString("Alert"), GetString("NoHeadTag"));
        
        gEditorDisplayMode = kDisplayModePreview;
        SetDisplayMode(kDisplayModeSource);
        throw Components.results.NS_ERROR_FAILURE;
      }

      var beginBody = htmlSource.indexOf("<body");
      if (beginBody == -1)
      {
        AlertWithTitle(GetString("Alert"), GetString("NoBodyTag"));
        
        gEditorDisplayMode = kDisplayModePreview;
        SetDisplayMode(kDisplayModeSource);
        throw Components.results.NS_ERROR_FAILURE;
      }
    }
  }

  
  SetEditMode(gPreviousNonSourceDisplayMode);
}

function SetDisplayMode(mode)
{
  if (!IsHTMLEditor())
    return false;

  
  
  if (mode == gEditorDisplayMode)
    return false;

  var previousMode = gEditorDisplayMode;
  gEditorDisplayMode = mode;

  ResetStructToolbar();
  if (mode == kDisplayModeSource)
  {
    
    gContentWindowDeck.selectedIndex = 1;

    
    gFormatToolbarHidden = gFormatToolbar.hidden;
    gFormatToolbar.hidden = true;
    gViewFormatToolbar.hidden = true;

    gSourceContentWindow.contentWindow.focus();
  }
  else
  {
    
    gPreviousNonSourceDisplayMode = mode;

    
    try {
      var editor = GetCurrentEditor();
      editor.QueryInterface(nsIEditorStyleSheets);
      editor instanceof Components.interfaces.nsIHTMLObjectResizer;

      switch (mode)
      {
        case kDisplayModePreview:
          
          editor.enableStyleSheet(kNormalStyleSheet, false);
          editor.enableStyleSheet(kAllTagsStyleSheet, false);
          editor.isImageResizingEnabled = true;
          break;

        case kDisplayModeNormal:
          editor.addOverrideStyleSheet(kNormalStyleSheet);
          
          editor.enableStyleSheet(kAllTagsStyleSheet, false);
          editor.isImageResizingEnabled = true;
          break;

        case kDisplayModeAllTags:
          editor.addOverrideStyleSheet(kNormalStyleSheet);
          editor.addOverrideStyleSheet(kAllTagsStyleSheet);
          
          
          if (editor.resizedObject) {
            editor.hideResizers();
          }
          editor.isImageResizingEnabled = false;
          break;
      }
    } catch(e) {}

    
    gContentWindowDeck.selectedIndex = 0;

    
    gFormatToolbar.hidden = gFormatToolbarHidden;
    gViewFormatToolbar.hidden = false;

    gContentWindow.focus();
  }

  
  window.updateCommands("mode_switch");

  
  
  document.getElementById("EditModeTabs").selectedItem = document.getElementById(kDisplayModeTabIDS[mode]);

  
  if (previousMode >= 0)
    document.getElementById(kDisplayModeMenuIDs[previousMode]).setAttribute("checked","false");
  document.getElementById(kDisplayModeMenuIDs[mode]).setAttribute("checked","true");
  

  return true;
}

function EditorToggleParagraphMarks()
{
  var menuItem = document.getElementById("viewParagraphMarks");
  if (menuItem)
  {
    
    
    
    var checked = menuItem.getAttribute("checked");
    try {
      var editor = GetCurrentEditor();
      editor.QueryInterface(nsIEditorStyleSheets);

      if (checked == "true")
        editor.addOverrideStyleSheet(kParagraphMarksStyleSheet);
      else
        editor.enableStyleSheet(kParagraphMarksStyleSheet, false);
    }
    catch(e) { return; }
  }
}

function InitPasteAsMenu()
{
  var menuItem = document.getElementById("menu_pasteTable")
  if(menuItem)
  {
    menuItem.IsInTable  
    menuItem.setAttribute("label", GetString(IsInTable() ? "NestedTable" : "Table"));
   
  }
  
}

function UpdateWindowTitle()
{
  try {
    var windowTitle = GetDocumentTitle();
    if (!windowTitle)
      windowTitle = GetString("untitled");

    
    var docUrl = GetDocumentUrl();
    if (docUrl && !IsUrlAboutBlank(docUrl))
    {
      var scheme = GetScheme(docUrl);
      var filename = GetFilename(docUrl);
      if (filename)
        windowTitle += " [" + scheme + ":/.../" + filename + "]";

      
      SaveRecentFilesPrefs();
    }
    
    var xulWin = document.documentElement;
    document.title = windowTitle + xulWin.getAttribute("titlemenuseparator") + 
                     xulWin.getAttribute("titlemodifier");
  } catch (e) { dump(e); }
}

function BuildRecentPagesMenu()
{
  var editor = GetCurrentEditor();
  if (!editor || !gPrefs)
    return;

  var popup = document.getElementById("menupopup_RecentFiles");
  if (!popup || !editor.document)
    return;

  
  while (popup.firstChild)
    popup.removeChild(popup.firstChild);

  
  
  var curUrl = StripPassword(GetDocumentUrl());
  var historyCount = 10;
  try {
    historyCount = gPrefs.getIntPref("editor.history.url_maximum");
  } catch(e) {}
  var menuIndex = 1;

  for (var i = 0; i < historyCount; i++)
  {
    var url = GetUnicharPref("editor.history_url_"+i);

    
    if (url && url != curUrl)
    {
      
      var title = GetUnicharPref("editor.history_title_"+i);
      AppendRecentMenuitem(popup, title, url, menuIndex);
      menuIndex++;
    }
  }
}

function SaveRecentFilesPrefs()
{
  
  if (!gPrefs) return;

  var curUrl = StripPassword(GetDocumentUrl());
  var historyCount = 10;
  try {
    historyCount = gPrefs.getIntPref("editor.history.url_maximum"); 
  } catch(e) {}

  var titleArray = [];
  var urlArray = [];

  if (historyCount && !IsUrlAboutBlank(curUrl) &&  GetScheme(curUrl) != "data")
  {
    titleArray.push(GetDocumentTitle());
    urlArray.push(curUrl);
  }

  for (var i = 0; i < historyCount && urlArray.length < historyCount; i++)
  {
    var url = GetUnicharPref("editor.history_url_"+i);

    
    

    
    if (url && url != curUrl && GetScheme(url) != "data")
    {
      var title = GetUnicharPref("editor.history_title_"+i);
      titleArray.push(title);
      urlArray.push(url);
    }
  }

  
  for (i = 0; i < urlArray.length; i++)
  {
    SetUnicharPref("editor.history_title_"+i, titleArray[i]);
    SetUnicharPref("editor.history_url_"+i, urlArray[i]);
  }
}

function AppendRecentMenuitem(menupopup, title, url, menuIndex)
{
  if (menupopup)
  {
    var menuItem = document.createElementNS(XUL_NS, "menuitem");
    if (menuItem)
    {
      var accessKey;
      if (menuIndex <= 9)
        accessKey = String(menuIndex);
      else if (menuIndex == 10)
        accessKey = "0";
      else
        accessKey = " ";

      var itemString = accessKey+" ";

      
      if (title)
      {
       itemString += title;
       itemString += " [";
      }
      itemString += url;
      if (title)
        itemString += "]";

      menuItem.setAttribute("label", itemString);
      menuItem.setAttribute("crop", "center");
      menuItem.setAttribute("value", url);
      if (accessKey != " ")
        menuItem.setAttribute("accesskey", accessKey);
      menupopup.appendChild(menuItem);
    }
  }
}

function EditorInitFileMenu()
{
  
  var docUrl = GetDocumentUrl();
  var scheme = GetScheme(docUrl);
  if (scheme && scheme != "file")
    SetElementEnabledById("saveMenuitem", false);

  
  var historyUrl = "";

  var historyCount = 10;
  try { historyCount = gPrefs.getIntPref("editor.history.url_maximum"); } catch(e) {}
  if (historyCount)
  {
    historyUrl = GetUnicharPref("editor.history_url_0");
    
    
    if (historyUrl && historyUrl == docUrl)
      historyUrl = GetUnicharPref("editor.history_url_1");
  }
  SetElementEnabledById("menu_RecentFiles", historyUrl != "");
}

function EditorInitFormatMenu()
{
  try {
    InitObjectPropertiesMenuitem("objectProperties");
    InitRemoveStylesMenuitems("removeStylesMenuitem", "removeLinksMenuitem", "removeNamedAnchorsMenuitem");
  } catch(ex) {}
}

function InitObjectPropertiesMenuitem(id)
{
  
  
  
  var menuItem = document.getElementById(id);
  if (!menuItem) return null;

  var element;
  var menuStr = GetString("AdvancedProperties");
  var name;

  if (IsEditingRenderedHTML())
    element = GetObjectForProperties();

  if (element && element.nodeName)
  {
    var objStr = "";
    menuItem.setAttribute("disabled", "");
    name = element.nodeName.toLowerCase();
    switch (name)
    {
      case "img":
        
        
        try
        {
          if (GetCurrentEditor().getElementOrParentByTagName("href", element))
            objStr = GetString("ImageAndLink");
        } catch(e) {}
        
        if (objStr == "")
          objStr = GetString("Image");
        break;
      case "hr":
        objStr = GetString("HLine");
        break;
      case "table":
        objStr = GetString("Table");
        break;
      case "th":
        name = "td";
      case "td":
        objStr = GetString("TableCell");
        break;
      case "ol":
      case "ul":
      case "dl":
        objStr = GetString("List");
        break;
      case "li":
        objStr = GetString("ListItem");
        break;
      case "form":
        objStr = GetString("Form");
        break;
      case "input":
        var type = element.getAttribute("type");
        if (type && type.toLowerCase() == "image")
          objStr = GetString("InputImage");
        else
          objStr = GetString("InputTag");
        break;
      case "textarea":
        objStr = GetString("TextArea");
        break;
      case "select":
        objStr = GetString("Select");
        break;
      case "button":
        objStr = GetString("Button");
        break;
      case "label":
        objStr = GetString("Label");
        break;
      case "fieldset":
        objStr = GetString("FieldSet");
        break;
      case "a":
        if (element.name)
        {
          objStr = GetString("NamedAnchor");
          name = "anchor";
        }
        else if(element.href)
        {
          objStr = GetString("Link");
          name = "href";
        }
        break;
    }
    if (objStr)
      menuStr = GetString("ObjectProperties").replace(/%obj%/,objStr);
  }
  else
  {
    
    menuItem.setAttribute("disabled","true");
  }
  menuItem.setAttribute("label", menuStr);
  menuItem.setAttribute("accesskey",GetString("ObjectPropertiesAccessKey"));
  return name;
}

function InitParagraphMenu()
{
  var mixedObj = { value: null };
  var state;
  try {
    state = GetCurrentEditor().getParagraphState(mixedObj);
  }
  catch(e) {}
  var IDSuffix;

  
  

  
  if (!state || state == "x")
    IDSuffix = "bodyText" 
  else
    IDSuffix = state;

  
  var menuItem = document.getElementById("menu_"+IDSuffix);
  menuItem.setAttribute("checked", "true");

  
  if (mixedObj.value)
    menuItem.setAttribute("checked", "false");
}

function GetListStateString()
{
  try {
    var editor = GetCurrentEditor();

    var mixedObj = { value: null };
    var hasOL = { value: false };
    var hasUL = { value: false };
    var hasDL = { value: false };
    editor.getListState(mixedObj, hasOL, hasUL, hasDL);

    if (mixedObj.value)
      return "mixed";
    if (hasOL.value)
      return "ol";
    if (hasUL.value)
      return "ul";

    if (hasDL.value)
    {
      var hasLI = { value: false };
      var hasDT = { value: false };
      var hasDD = { value: false };
      editor.getListItemState(mixedObj, hasLI, hasDT, hasDD);
      if (mixedObj.value)
        return "mixed";
      if (hasLI.value)
        return "li";
      if (hasDT.value)
        return "dt";
      if (hasDD.value)
        return "dd";
    }
  } catch (e) {}

  
  return "noList";
}

function InitListMenu()
{
  if (!IsHTMLEditor())
    return;

  var IDSuffix = GetListStateString();

  
  goSetCommandEnabled("cmd_removeList", IDSuffix != "noList");

  
  
  var menuItem = document.getElementById("menu_"+IDSuffix);
  if (menuItem)
    menuItem.setAttribute("checked", "true");
}

function GetAlignmentString()
{
  var mixedObj = { value: null };
  var alignObj = { value: null };
  try {
    GetCurrentEditor().getAlignment(mixedObj, alignObj);
  } catch (e) {}

  if (mixedObj.value)
    return "mixed";
  if (alignObj.value == nsIHTMLEditor.eLeft)
    return "left";
  if (alignObj.value == nsIHTMLEditor.eCenter)
    return "center";
  if (alignObj.value == nsIHTMLEditor.eRight)
    return "right";
  if (alignObj.value == nsIHTMLEditor.eJustify)
    return "justify";

  
  return "left";
}

function InitAlignMenu()
{
  if (!IsHTMLEditor())
    return;

  var IDSuffix = GetAlignmentString();

  
  var menuItem = document.getElementById("menu_"+IDSuffix);
  if (menuItem)
    menuItem.setAttribute("checked", "true");
}

function EditorSetDefaultPrefsAndDoctype()
{
  var editor = GetCurrentEditor();

  var domdoc;
  try { 
    domdoc = editor.document;
  } catch (e) { dump( e + "\n"); }
  if ( !domdoc )
  {
    dump("EditorSetDefaultPrefsAndDoctype: EDITOR DOCUMENT NOT FOUND\n");
    return;
  }

  
  
  if (!domdoc.doctype)
  {
    var newdoctype = domdoc.implementation.createDocumentType("HTML", "-//W3C//DTD HTML 4.01 Transitional//EN","");
    if (newdoctype)
      domdoc.insertBefore(newdoctype, domdoc.firstChild);
  }
  
  
  var headelement = 0;
  var headnodelist = domdoc.getElementsByTagName("head");
  if (headnodelist)
  {
    var sz = headnodelist.length;
    if ( sz >= 1 )
      headelement = headnodelist.item(0);
  }
  else
  {
    headelement = domdoc.createElement("head");
    if (headelement)
      domdoc.insertAfter(headelement, domdoc.firstChild);
  }

  
  if (!IsUrlAboutBlank(GetDocumentUrl()))
    return;

  
  
  
  

  var nodelist = domdoc.getElementsByTagName("meta");
  if ( nodelist )
  {
    
    
    
    var element;
    var prefCharsetString = 0;
    try
    {
      prefCharsetString = gPrefs.getComplexValue("intl.charset.default",
                                                 Components.interfaces.nsIPrefLocalizedString).data;
    }
    catch (ex) {}
    if ( prefCharsetString && prefCharsetString != 0)
    {
        element = domdoc.createElement("meta");
        if ( element )
        {
          element.setAttribute("http-equiv", "content-type");
          element.setAttribute("content", "text/html; charset=" + prefCharsetString);
          headelement.insertBefore( element, headelement.firstChild );
        }
    }

    var node = 0;
    var listlength = nodelist.length;

    
    var authorFound = false;
    for (var i = 0; i < listlength && !authorFound; i++)
    {
      node = nodelist.item(i);
      if ( node )
      {
        var value = node.getAttribute("name");
        if (value && value.toLowerCase() == "author")
        {
          authorFound = true;
        }
      }
    }

    var prefAuthorString = 0;
    try
    {
      prefAuthorString = gPrefs.getComplexValue("editor.author",
                                                Components.interfaces.nsISupportsString).data;
    }
    catch (ex) {}
    if ( prefAuthorString && prefAuthorString != 0)
    {
      if ( !authorFound && headelement)
      {
        
        element = domdoc.createElement("meta");
        if ( element )
        {
          element.setAttribute("name", "author");
          element.setAttribute("content", prefAuthorString);
          headelement.appendChild( element );
        }
      }
    }
  }

  
  var titlenodelist = editor.document.getElementsByTagName("title");
  if (headelement && titlenodelist && titlenodelist.length == 0)
  {
     titleElement = domdoc.createElement("title");
     if (titleElement)
       headelement.appendChild(titleElement);
  }

  
  var use_custom_colors = false;
  try {
    use_custom_colors = gPrefs.getBoolPref("editor.use_custom_colors");
  }
  catch (ex) {}

  
  var bodyelement = GetBodyElement();
  if (bodyelement)
  {
    if ( use_custom_colors )
    {
      
      var text_color;
      var link_color;
      var active_link_color;
      var followed_link_color;
      var background_color;

      try { text_color = gPrefs.getCharPref("editor.text_color"); } catch (e) {}
      try { link_color = gPrefs.getCharPref("editor.link_color"); } catch (e) {}
      try { active_link_color = gPrefs.getCharPref("editor.active_link_color"); } catch (e) {}
      try { followed_link_color = gPrefs.getCharPref("editor.followed_link_color"); } catch (e) {}
      try { background_color = gPrefs.getCharPref("editor.background_color"); } catch(e) {}

      
      
      try {
        if (text_color)
        {
          editor.setAttributeOrEquivalent(bodyelement, "text", text_color, true);
          gDefaultTextColor = text_color;
        }
        if (background_color)
        {
          editor.setAttributeOrEquivalent(bodyelement, "bgcolor", background_color, true);
          gDefaultBackgroundColor = background_color
        }

        if (link_color)
          bodyelement.setAttribute("link", link_color);
        if (active_link_color)
          bodyelement.setAttribute("alink", active_link_color);
        if (followed_link_color)
          bodyelement.setAttribute("vlink", followed_link_color);
      } catch (e) {}
    }
    
    try {
      var background_image = gPrefs.getCharPref("editor.default_background_image");
      if (background_image)
        editor.setAttributeOrEquivalent(bodyelement, "background", background_image, true);
    } catch (e) {dump("BACKGROUND EXCEPTION: "+e+"\n"); }

  }
  
}

function GetBodyElement()
{
  try {
    return GetCurrentEditor().rootElement;
  }
  catch (ex) {
    dump("no body tag found?!\n");
    
  }
  return null;
}



function EditorGetNodeFromOffsets(offsets)
{
  var node = null;
  try {
    node = GetCurrentEditor().document;

    for (var i = 0; i < offsets.length; i++)
      node = node.childNodes[offsets[i]];
  } catch (e) {}
  return node;
}

function EditorSetSelectionFromOffsets(selRanges)
{
  try {
    var editor = GetCurrentEditor();
    var selection = editor.selection;
    selection.removeAllRanges();

    var rangeArr, start, end, node, offset;
    for (var i = 0; i < selRanges.length; i++)
    {
      rangeArr = selRanges[i];
      start    = rangeArr[0];
      end      = rangeArr[1];

      var range = editor.document.createRange();

      node   = EditorGetNodeFromOffsets(start[0]);
      offset = start[1];

      range.setStart(node, offset);

      node   = EditorGetNodeFromOffsets(end[0]);
      offset = end[1];

      range.setEnd(node, offset);

      selection.addRange(range);
    }
  } catch (e) {}
}


function initFontStyleMenu(menuPopup)
{
  for (var i = 0; i < menuPopup.childNodes.length; i++)
  {
    var menuItem = menuPopup.childNodes[i];
    var theStyle = menuItem.getAttribute("state");
    if (theStyle)
    {
      menuItem.setAttribute("checked", theStyle);
    }
  }
}


function onButtonUpdate(button, commmandID)
{
  var commandNode = document.getElementById(commmandID);
  var state = commandNode.getAttribute("state");
  button.checked = state == "true";
}


function onStateButtonUpdate(button, commmandID, onState)
{
  var commandNode = document.getElementById(commmandID);
  var state = commandNode.getAttribute("state");

  button.checked = state == onState;
}


function getColorAndSetColorWell(ColorPickerID, ColorWellID)
{
  var colorWell;
  if (ColorWellID)
    colorWell = document.getElementById(ColorWellID);

  var colorPicker = document.getElementById(ColorPickerID);
  if (colorPicker)
  {
    
    var color = colorPicker.getAttribute("color");

    if (colorWell && color)
    {
      
      colorWell.setAttribute("style", "background-color: " + color);
    }
  }
  return color;
}


function IsSpellCheckerInstalled()
{
  return "@mozilla.org/spellchecker;1" in Components.classes;
}


function IsFindInstalled()
{
  return "@mozilla.org/embedcomp/rangefind;1" in Components.classes
          && "@mozilla.org/find/find_service;1" in Components.classes;
}


function RemoveInapplicableUIElements()
{
  
  

   
  if (!IsFindInstalled())
  {
    HideItem("menu_find");
    HideItem("menu_findnext");
    HideItem("menu_replace");
    HideItem("menu_find");
    RemoveItem("sep_find");
  }

   
  if (!IsSpellCheckerInstalled())
  {
    HideItem("spellingButton");
    HideItem("menu_checkspelling");
    RemoveItem("sep_checkspelling");
  }
  else
  {
    SetElementEnabled(document.getElementById("menu_checkspelling"), true);
    SetElementEnabled(document.getElementById("spellingButton"), true);
    SetElementEnabled(document.getElementById("checkspellingkb"), true);
  }

  
  if (!IsHTMLEditor())
  {
    HideItem("insertAnchor");
    HideItem("insertImage");
    HideItem("insertHline");
    HideItem("insertTable");
    HideItem("insertHTML");
    HideItem("insertFormMenu");
    HideItem("fileExportToText");
    HideItem("viewFormatToolbar");
    HideItem("viewEditModeToolbar");
  }
}

function HideItem(id)
{
  var item = document.getElementById(id);
  if (item)
    item.hidden = true;
}

function RemoveItem(id)
{
  var item = document.getElementById(id);
  if (item)
    item.parentNode.removeChild(item);
}




function EditorInitTableMenu()
{
  try {
    InitJoinCellMenuitem("menu_JoinTableCells");
  } catch (ex) {}

  
  goUpdateTableMenuItems(document.getElementById("composerTableMenuItems"));
}

function InitJoinCellMenuitem(id)
{
  
  
  
  
  
  var menuText;
  var menuItem = document.getElementById(id);
  if (!menuItem) return;

  
  var numSelected;
  var foundElement;
  
  try {
    var tagNameObj = {};
    var countObj = {value:0}
    foundElement = GetCurrentTableEditor().getSelectedOrParentTableElement(tagNameObj, countObj);
    numSelected = countObj.value
  }
  catch(e) {}
  if (foundElement && numSelected > 1)
    menuText = GetString("JoinSelectedCells");
  else
    menuText = GetString("JoinCellToRight");

  menuItem.setAttribute("label",menuText);
  menuItem.setAttribute("accesskey",GetString("JoinCellAccesskey"));
}

function InitRemoveStylesMenuitems(removeStylesId, removeLinksId, removeNamedAnchorsId)
{
  var editor = GetCurrentEditor();
  if (!editor)
    return;

  
  var stylesItem = document.getElementById(removeStylesId);
  var linkItem = document.getElementById(removeLinksId);

  var isCollapsed = editor.selection.isCollapsed;
  if (stylesItem)
  {
    stylesItem.setAttribute("label", isCollapsed ? GetString("StopTextStyles") : GetString("RemoveTextStyles"));
    stylesItem.setAttribute("accesskey", GetString("RemoveTextStylesAccesskey"));
  }
  if (linkItem)
  {
    linkItem.setAttribute("label", isCollapsed ? GetString("StopLinks") : GetString("RemoveLinks"));
    linkItem.setAttribute("accesskey", GetString("RemoveLinksAccesskey"));
    

    
    
    try {
      SetElementEnabled(linkItem, !isCollapsed ||
                      editor.getElementOrParentByTagName("href", null));
    } catch(e) {}      
  }
  
  SetElementEnabledById(removeNamedAnchorsId, !isCollapsed);
}

function goUpdateTableMenuItems(commandset)
{
  var editor = GetCurrentTableEditor();
  if (!editor)
  {
    dump("goUpdateTableMenuItems: too early, not initialized\n");
    return;
  }

  var enabled = false;
  var enabledIfTable = false;

  var flags = editor.flags;
  if (!(flags & nsIPlaintextEditor.eEditorReadonlyMask) &&
      IsEditingRenderedHTML())
  {
    var tagNameObj = { value: "" };
    var element;
    try {
      element = editor.getSelectedOrParentTableElement(tagNameObj, {value:0});
    }
    catch(e) {}

    if (element)
    {
      
      enabledIfTable = true;

      
      enabled = (tagNameObj.value == "td");
    }
  }

  
  for (var i = 0; i < commandset.childNodes.length; i++)
  {
    var commandID = commandset.childNodes[i].getAttribute("id");
    if (commandID)
    {
      if (commandID == "cmd_InsertTable" ||
          commandID == "cmd_JoinTableCells" ||
          commandID == "cmd_SplitTableCell" ||
          commandID == "cmd_ConvertToTable")
      {
        
        goUpdateCommand(commandID);
      }
      
      else if (commandID == "cmd_DeleteTable" ||
               commandID == "cmd_NormalizeTable" ||
               commandID == "cmd_editTable" ||
               commandID == "cmd_TableOrCellColor" ||
               commandID == "cmd_SelectTable")
      {
        goSetCommandEnabled(commandID, enabledIfTable);
      } else {
        goSetCommandEnabled(commandID, enabled);
      }
    }
  }
}




function IsInTable()
{
  var editor = GetCurrentEditor();
  try {
    var flags = editor.flags;
    return (IsHTMLEditor() &&
            !(flags & nsIPlaintextEditor.eEditorReadonlyMask) &&
            IsEditingRenderedHTML() &&
            null != editor.getElementOrParentByTagName("table", null));
  } catch (e) {}
  return false;
}

function IsInTableCell()
{
  try {
    var editor = GetCurrentEditor();
    var flags = editor.flags;
    return (IsHTMLEditor() &&
            !(flags & nsIPlaintextEditor.eEditorReadonlyMask) && 
            IsEditingRenderedHTML() &&
            null != editor.getElementOrParentByTagName("td", null));
  } catch (e) {}
  return false;

}

function IsSelectionInOneCell()
{
  try {
    var editor = GetCurrentEditor();
    var selection = editor.selection;

    if (selection.rangeCount == 1)
    {
      
      if (!selection.isCollapsed &&
         selection.anchorNode != selection.focusNode)
      {
        
        var anchorCell = editor.getElementOrParentByTagName("td", selection.anchorNode);
        var focusCell = editor.getElementOrParentByTagName("td", selection.focusNode);
        return (focusCell != null && anchorCell != null && (focusCell == anchorCell));
      }
      
      return true;
    }
  } catch (e) {}
  return false;
}



function EditorInsertOrEditTable(insertAllowed)
{
  if (IsInTable())
  {
    
    window.openDialog("chrome://editor/content/EdTableProps.xul", "_blank", "chrome,close,titlebar,modal", "","TablePanel");
    gContentWindow.focus();
  } 
  else if (insertAllowed)
  {
    try {
      if (GetCurrentEditor().selection.isCollapsed)
        
        EditorInsertTable();
      else
        
        goDoCommand("cmd_ConvertToTable");
    } catch (e) {}
  }
}

function EditorInsertTable()
{
  
  window.openDialog("chrome://editor/content/EdInsertTable.xul", "_blank", "chrome,close,titlebar,modal", "");
  gContentWindow.focus();
}

function EditorTableCellProperties()
{
  if (!IsHTMLEditor())
    return;

  try {
    var cell = GetCurrentEditor().getElementOrParentByTagName("td", null);
    if (cell) {
      
      window.openDialog("chrome://editor/content/EdTableProps.xul", "_blank", "chrome,close,titlebar,modal", "", "CellPanel");
      gContentWindow.focus();
    }
  } catch (e) {}
}

function GetNumberOfContiguousSelectedRows()
{
  if (!IsHTMLEditor())
    return 0;

  var rows = 0;
  try {
    var editor = GetCurrentTableEditor();
    var rowObj = { value: 0 };
    var colObj = { value: 0 };
    var cell = editor.getFirstSelectedCellInTable(rowObj, colObj);
    if (!cell)
      return 0;

    
    rows++;

    var lastIndex = rowObj.value;
    do {
      cell = editor.getNextSelectedCell({value:0});
      if (cell)
      {
        editor.getCellIndexes(cell, rowObj, colObj);
        var index = rowObj.value;
        if (index == lastIndex + 1)
        {
          lastIndex = index;
          rows++;
        }
      }
    }
    while (cell);
  } catch (e) {}

  return rows;
}

function GetNumberOfContiguousSelectedColumns()
{
  if (!IsHTMLEditor())
    return 0;

  var columns = 0;
  try {
    var editor = GetCurrentTableEditor();
    var colObj = { value: 0 };
    var rowObj = { value: 0 };
    var cell = editor.getFirstSelectedCellInTable(rowObj, colObj);
    if (!cell)
      return 0;

    
    columns++;

    var lastIndex = colObj.value;
    do {
      cell = editor.getNextSelectedCell({value:0});
      if (cell)
      {
        editor.getCellIndexes(cell, rowObj, colObj);
        var index = colObj.value;
        if (index == lastIndex +1)
        {
          lastIndex = index;
          columns++;
        }
      }
    }
    while (cell);
  } catch (e) {}

  return columns;
}

function EditorOnFocus()
{
  
  if ("InsertCharWindow" in window && window.InsertCharWindow) return;

  
  var windowWithDialog = FindEditorWithInsertCharDialog();
  if (windowWithDialog)
  {
    
    
    if (SwitchInsertCharToThisWindow(windowWithDialog))
      top.document.commandDispatcher.focusedWindow.focus();
  }
}

function SwitchInsertCharToThisWindow(windowWithDialog)
{
  if (windowWithDialog && "InsertCharWindow" in windowWithDialog &&
      windowWithDialog.InsertCharWindow)
  {
    
    window.InsertCharWindow = windowWithDialog.InsertCharWindow;
    windowWithDialog.InsertCharWindow = null;

    
    window.InsertCharWindow.opener = window;

    
    window.InsertCharWindow.focus();
    return true;
  }
  return false;
}

function FindEditorWithInsertCharDialog()
{
  try {
    
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    var enumerator = windowManagerInterface.getEnumerator( null );

    while ( enumerator.hasMoreElements()  )
    {
      var tempWindow = enumerator.getNext();

      if (tempWindow != window && "InsertCharWindow" in tempWindow &&
          tempWindow.InsertCharWindow)
      {
        return tempWindow;
      }
    }
  }
  catch(e) {}
  return null;
}

function EditorFindOrCreateInsertCharWindow()
{
  if ("InsertCharWindow" in window && window.InsertCharWindow)
    window.InsertCharWindow.focus();
  else
  {
    
    
    var windowWithDialog = FindEditorWithInsertCharDialog();
    if (windowWithDialog)
    {
      SwitchInsertCharToThisWindow(windowWithDialog);
    }
    else
    {
      
      window.openDialog("chrome://editor/content/EdInsertChars.xul", "_blank", "chrome,close,titlebar", "");
    }
  }
}



function SwitchInsertCharToAnotherEditorOrClose()
{
  if ("InsertCharWindow" in window && window.InsertCharWindow)
  {
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var enumerator;
    try {
      var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
      enumerator = windowManagerInterface.getEnumerator( null );
    }
    catch(e) {}
    if (!enumerator) return;

    
    
    while ( enumerator.hasMoreElements()  )
    {
      var  tempWindow = enumerator.getNext();
      if (tempWindow != window && tempWindow != window.InsertCharWindow &&
          "GetCurrentEditor" in tempWindow && tmpWindow.GetCurrentEditor())
      {
        tempWindow.InsertCharWindow = window.InsertCharWindow;
        window.InsertCharWindow = null;
        tempWindow.InsertCharWindow.opener = tempWindow;
        return;
      }
    }
    
    window.InsertCharWindow.close();
  }
}

function ResetStructToolbar()
{
  gLastFocusNode = null;
  UpdateStructToolbar();
}

function newCommandListener(element)
{
  return function() { return SelectFocusNodeAncestor(element); };
}

function newContextmenuListener(button, element)
{
  return function() { return InitStructBarContextMenu(button, element); };
}

function UpdateStructToolbar()
{
  var editor = GetCurrentEditor();
  if (!editor) return;

  var mixed = GetSelectionContainer();
  if (!mixed) return;
  var element = mixed.node;
  var oneElementSelected = mixed.oneElementSelected;

  if (!element) return;

  if (element == gLastFocusNode &&
      oneElementSelected == gLastFocusNodeWasSelected)
    return;

  gLastFocusNode = element;
  gLastFocusNodeWasSelected = mixed.oneElementSelected;

  var toolbar = document.getElementById("structToolbar");
  if (!toolbar) return;
  var childNodes = toolbar.childNodes;
  var childNodesLength = childNodes.length;
  
  
  while (childNodes.length > 1) {
    
    toolbar.removeChild(childNodes.item(childNodes.length - 2));
  }

  toolbar.removeAttribute("label");

  if ( IsInHTMLSourceMode() ) {
    
    
    
    return;
  }

  var tag, button;
  var bodyElement = GetBodyElement();
  var isFocusNode = true;
  var tmp;
  do {
    tag = element.nodeName.toLowerCase();

    button = document.createElementNS(XUL_NS, "toolbarbutton");
    button.setAttribute("label",   "<" + tag + ">");
    button.setAttribute("value",   tag);
    button.setAttribute("context", "structToolbarContext");
    button.className = "struct-button";

    toolbar.insertBefore(button, toolbar.firstChild);

    button.addEventListener("command", newCommandListener(element), false);

    button.addEventListener("contextmenu", newContextmenuListener(button, element), false);

    if (isFocusNode && oneElementSelected) {
      button.setAttribute("checked", "true");
      isFocusNode = false;
    }

    tmp = element;
    element = element.parentNode;

  } while (tmp != bodyElement);
}

function SelectFocusNodeAncestor(element)
{
  var editor = GetCurrentEditor();
  if (editor) {
    if (element == GetBodyElement())
      editor.selectAll();
    else
      editor.selectElement(element);
  }
  ResetStructToolbar();
}

function GetSelectionContainer()
{
  var editor = GetCurrentEditor();
  if (!editor) return null;

  try {
    var selection = editor.selection;
    if (!selection) return null;
  }
  catch (e) { return null; }

  var result = { oneElementSelected:false };

  if (selection.isCollapsed) {
    result.node = selection.focusNode;
  }
  else {
    var rangeCount = selection.rangeCount;
    if (rangeCount == 1) {
      result.node = editor.getSelectedElement("");
      var range = selection.getRangeAt(0);

      
      
      
      
      
      if (!result.node &&
          range.startContainer.nodeType == Node.TEXT_NODE &&
          range.startOffset == range.startContainer.length &&
          range.endContainer.nodeType == Node.TEXT_NODE &&
          range.endOffset == range.endContainer.length &&
          range.endContainer.nextSibling == null &&
          range.startContainer.nextSibling == range.endContainer.parentNode)
        result.node = range.endContainer.parentNode;

      if (!result.node) {
        
        result.node = range.commonAncestorContainer;
      }
      else {
        result.oneElementSelected = true;
      }
    }
    else {
      
      var i, container = null;
      for (i = 0; i < rangeCount; i++) {
        range = selection.getRangeAt(i);
        if (!container) {
          container = range.startContainer;
        }
        else if (container != range.startContainer) {
          
          
          result.node = container.parentNode;
          break;
        }
        result.node = container;
      }
    }
  }

  
  while (result.node.nodeType != Node.ELEMENT_NODE)
    result.node = result.node.parentNode;

  
  
  
  while (result.node.hasAttribute("_moz_editor_bogus_node") ||
         editor.isAnonymousElement(result.node))
    result.node = result.node.parentNode;

  return result;
}

function FillInHTMLTooltip(tooltip)
{
  const XLinkNS = "http://www.w3.org/1999/xlink";
  var tooltipText = null;
  var node;
  if (gEditorDisplayMode == kDisplayModePreview) {
    for (node = document.tooltipNode; node; node = node.parentNode) {
      if (node.nodeType == Node.ELEMENT_NODE) {
        tooltipText = node.getAttributeNS(XLinkNS, "title");
        if (tooltipText && /\S/.test(tooltipText)) {
          tooltip.setAttribute("label", tooltipText);
          return true;
        }
        tooltipText = node.getAttribute("title");
        if (tooltipText && /\S/.test(tooltipText)) {
          tooltip.setAttribute("label", tooltipText);
          return true;
        }
      }
    }
  } else {
    for (node = document.tooltipNode; node; node = node.parentNode) {
      if (node instanceof Components.interfaces.nsIDOMHTMLImageElement ||
          node instanceof Components.interfaces.nsIDOMHTMLInputElement)
        tooltipText = node.getAttribute("src");
      else if (node instanceof Components.interfaces.nsIDOMHTMLAnchorElement)
        tooltipText = node.getAttribute("href") || node.name;
      if (tooltipText) {
        tooltip.setAttribute("label", tooltipText);
        return true;
      }
    }
  }
  return false;
}

function UpdateTOC()
{
  window.openDialog("chrome://editor/content/EdInsertTOC.xul",
                    "_blank", "chrome,close,modal,titlebar");
  window.content.focus();
}

function InitTOCMenu()
{
  var elt = GetCurrentEditor().document.getElementById("mozToc");
  var createMenuitem = document.getElementById("insertTOCMenuitem");
  var updateMenuitem = document.getElementById("updateTOCMenuitem");
  var removeMenuitem = document.getElementById("removeTOCMenuitem");
  if (removeMenuitem && createMenuitem && updateMenuitem) {
    if (elt) {
      createMenuitem.setAttribute("disabled", "true");
      updateMenuitem.removeAttribute("disabled");
      removeMenuitem.removeAttribute("disabled");
    }
    else {
      createMenuitem.removeAttribute("disabled");
      removeMenuitem.setAttribute("disabled", "true");
      updateMenuitem.setAttribute("disabled", "true");
    }
  }
}

function RemoveTOC()
{
  var theDocument = GetCurrentEditor().document;
  var elt = theDocument.getElementById("mozToc");
  if (elt) {
    elt.parentNode.removeChild(elt);
  }

  function acceptNode(node)
  {
    if (node.nodeName.toLowerCase() == "a" &&
        node.hasAttribute("name") &&
        node.getAttribute("name").substr(0, 8) == "mozTocId") {
      return NodeFilter.FILTER_ACCEPT;
    }
    return NodeFilter.FILTER_SKIP;
  }

  var treeWalker = theDocument.createTreeWalker(theDocument.documentElement,
                                                NodeFilter.SHOW_ELEMENT,
                                                acceptNode,
                                                true);
  if (treeWalker) {
    var anchorNode = treeWalker.nextNode();
    while (anchorNode) {
      var tmp = treeWalker.nextNode();
      anchorNode.parentNode.removeChild(anchorNode);
      anchorNode = tmp;
    }
  }
}
