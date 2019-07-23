








































const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";





var gDialog = {};

const kOutputEncodeBasicEntities = Components.interfaces.nsIDocumentEncoder.OutputEncodeBasicEntities;
const kOutputEncodeHTMLEntities = Components.interfaces.nsIDocumentEncoder.OutputEncodeHTMLEntities;
const kOutputEncodeLatin1Entities = Components.interfaces.nsIDocumentEncoder.OutputEncodeLatin1Entities;
const kOutputEncodeW3CEntities = Components.interfaces.nsIDocumentEncoder.OutputEncodeW3CEntities;
const kOutputFormatted = Components.interfaces.nsIDocumentEncoder.OutputFormatted;
const kOutputLFLineBreak = Components.interfaces.nsIDocumentEncoder.OutputLFLineBreak;
const kOutputSelectionOnly = Components.interfaces.nsIDocumentEncoder.OutputSelectionOnly;
const kOutputWrap = Components.interfaces.nsIDocumentEncoder.OutputWrap;

var gStringBundle;
var gIOService;
var gPrefsService;
var gPrefsBranch;
var gFilePickerDirectory;

var gOS = "";
const gWin = "Win";
const gUNIX = "UNIX";
const gMac = "Mac";

const kWebComposerWindowID = "editorWindow";
const kMailComposerWindowID = "msgcomposeWindow";

var gIsHTMLEditor;


function AlertWithTitle(title, message, parentWindow)
{
  if (!parentWindow)
    parentWindow = window;

  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
  promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService);

  if (promptService)
  {
    if (!title)
      title = GetString("Alert");

    
    promptService.alert(parentWindow, title, message);
  }
}


function ConfirmWithTitle(title, message, okButtonText, cancelButtonText)
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
  promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService);

  if (promptService)
  {
    var okFlag = okButtonText ? promptService.BUTTON_TITLE_IS_STRING : promptService.BUTTON_TITLE_OK;
    var cancelFlag = cancelButtonText ? promptService.BUTTON_TITLE_IS_STRING : promptService.BUTTON_TITLE_CANCEL;

    return promptService.confirmEx(window, title, message,
                            (okFlag * promptService.BUTTON_POS_0) +
                            (cancelFlag * promptService.BUTTON_POS_1),
                            okButtonText, cancelButtonText, null, null, {value:0}) == 0;
  }
  return false;
}



function GetString(name)
{
  if (!gStringBundle)
  {
    try {
      var strBundleService =
          Components.classes["@mozilla.org/intl/stringbundle;1"].getService(); 
      strBundleService = 
          strBundleService.QueryInterface(Components.interfaces.nsIStringBundleService);

      gStringBundle = strBundleService.createBundle("chrome://editor/locale/editor.properties"); 

    } catch (ex) {}
  }
  if (gStringBundle)
  {
    try {
      return gStringBundle.GetStringFromName(name);
    } catch (e) {}
  }
  return null;
}

function TrimStringLeft(string)
{
  if(!string) return "";
  return string.replace(/^\s+/, "");
}

function TrimStringRight(string)
{
  if (!string) return "";
  return string.replace(/\s+$/, '');
}


function TrimString(string)
{
  if (!string) return "";
  return string.replace(/(^\s+)|(\s+$)/g, '')
}

function IsWhitespace(string)
{
  return /^\s/.test(string);
}

function TruncateStringAtWordEnd(string, maxLength, addEllipses)
{
  
  if (!string)
    return "";

  
  string = string.replace(/^\s+/, '');
  if (string.length <= maxLength)
    return string;

  
  if (addEllipses)
    maxLength -= 3;
  string = string.replace(RegExp("(.{0," + maxLength + "})\\s.*"), "$1")

  if (string.length > maxLength)
    string = string.slice(0, maxLength);

  if (addEllipses)
    string += "...";
  return string;
}




function ReplaceWhitespace(string, charReplace)
{
  return string.replace(/(^\s+)|(\s+$)/g,'').replace(/\s+/g,charReplace)
}




function ConvertToCDATAString(string)
{
  return string.replace(/\s+/g,"_").replace(/[^a-zA-Z0-9_\.\-\:\u0080-\uFFFF]+/g,'');
}

function GetSelectionAsText()
{
  try {
    return GetCurrentEditor().outputToString("text/plain", kOutputSelectionOnly);
  } catch (e) {}

  return "";
}



const nsIPlaintextEditor = Components.interfaces.nsIPlaintextEditor;
const nsIHTMLEditor = Components.interfaces.nsIHTMLEditor;
const nsITableEditor = Components.interfaces.nsITableEditor;
const nsIEditorStyleSheets = Components.interfaces.nsIEditorStyleSheets;
const nsIEditingSession = Components.interfaces.nsIEditingSession;

function GetCurrentEditor()
{
  
  
  

  
  var editor;
  try {
    var editorElement = GetCurrentEditorElement();
    editor = editorElement.getEditor(editorElement.contentWindow);

    
    
    editor instanceof Components.interfaces.nsIPlaintextEditor;
    editor instanceof Components.interfaces.nsIHTMLEditor;
  } catch (e) { dump (e)+"\n"; }

  return editor;
}

function GetCurrentTableEditor()
{
  var editor = GetCurrentEditor();
  return (editor && (editor instanceof nsITableEditor)) ? editor : null;
}

function GetCurrentEditorElement()
{
  var tmpWindow = window;
  
  do {
    
    var editorList = tmpWindow.document.getElementsByTagName("editor");

    
    if (editorList.item(0))
      return editorList.item(0);

    tmpWindow = tmpWindow.opener;
  } 
  while (tmpWindow);

  return null;
}

function GetCurrentCommandManager()
{
  try {
    return GetCurrentEditorElement().commandManager;
  } catch (e) { dump (e)+"\n"; }

  return null;
}

function GetCurrentEditorType()
{
  try {
    return GetCurrentEditorElement().editortype;
  } catch (e) { dump (e)+"\n"; }

  return "";
}

function IsHTMLEditor()
{
  
  if (!GetCurrentEditorElement())
    return false;

  var editortype = GetCurrentEditorType();
  switch (editortype)
  {
      case "html":
      case "htmlmail":
        return true;

      case "text":
      case "textmail":
        return false

      default:
        dump("INVALID EDITOR TYPE: " + editortype + "\n");
        break;
  }
  return false;
}

function PageIsEmptyAndUntouched()
{
  return IsDocumentEmpty() && !IsDocumentModified() && !IsHTMLSourceChanged();
}

function IsInHTMLSourceMode()
{
  return (gEditorDisplayMode == kDisplayModeSource);
}


function IsEditingRenderedHTML()
{
  return IsHTMLEditor() && !IsInHTMLSourceMode();
}

function IsWebComposer()
{
  return document.documentElement.id == "editorWindow";
}

function IsDocumentEditable()
{
  try {
    return GetCurrentEditor().isDocumentEditable;
  } catch (e) {}
  return false;
}

function IsDocumentEmpty()
{
  try {
    return GetCurrentEditor().documentIsEmpty;
  } catch (e) {}
  return false;
}

function IsDocumentModified()
{
  try {
    return GetCurrentEditor().documentModified;
  } catch (e) {}
  return false;
}

function IsHTMLSourceChanged()
{
  return gSourceTextEditor.documentModified;
}

function newCommandParams()
{
  try {
    return Components.classes["@mozilla.org/embedcomp/command-params;1"].createInstance(Components.interfaces.nsICommandParams);
  }
  catch(e) { dump("error thrown in newCommandParams: "+e+"\n"); }
  return null;
}



function GetDocumentTitle()
{
  try {
    return new XPCNativeWrapper(GetCurrentEditor().document, "title").title;
  } catch (e) {}

  return "";
}

function SetDocumentTitle(title)
{

  try {
    GetCurrentEditor().setDocumentTitle(title);

    
    if ("UpdateWindowTitle" in window)
      window.UpdateWindowTitle();
  } catch (e) {}
}

var gAtomService;
function GetAtomService()
{
  gAtomService = Components.classes["@mozilla.org/atom-service;1"].getService(Components.interfaces.nsIAtomService);
}

function EditorGetTextProperty(property, attribute, value, firstHas, anyHas, allHas)
{
  try {
    if (!gAtomService) GetAtomService();
    var propAtom = gAtomService.getAtom(property);

    GetCurrentEditor().getInlineProperty(propAtom, attribute, value,
                                         firstHas, anyHas, allHas);
  }
  catch(e) {}
}

function EditorSetTextProperty(property, attribute, value)
{
  try {
    if (!gAtomService) GetAtomService();
    var propAtom = gAtomService.getAtom(property);

    GetCurrentEditor().setInlineProperty(propAtom, attribute, value);
    if ("gContentWindow" in window)
      window.gContentWindow.focus();
  }
  catch(e) {}
}

function EditorRemoveTextProperty(property, attribute)
{
  try {
    if (!gAtomService) GetAtomService();
    var propAtom = gAtomService.getAtom(property);

    GetCurrentEditor().removeInlineProperty(propAtom, attribute);
    if ("gContentWindow" in window)
      window.gContentWindow.focus();
  }
  catch(e) {}
}






function SetElementEnabledById(elementID, doEnable)
{
  SetElementEnabled(document.getElementById(elementID), doEnable);
}

function SetElementEnabled(element, doEnable)
{
  if ( element )
  {
    if ( doEnable )
      element.removeAttribute("disabled");
    else
      element.setAttribute("disabled", "true");
  }
  else
  {
    dump("Element  not found in SetElementEnabled\n");
  }
}



function GetIOService()
{
  if (gIOService)
    return gIOService;

  gIOService = Components.classes["@mozilla.org/network/io-service;1"]
               .getService(Components.interfaces.nsIIOService);

  return gIOService;
}

function GetFileProtocolHandler()
{
  var ios = GetIOService();
  var handler = ios.getProtocolHandler("file");
  return handler.QueryInterface(Components.interfaces.nsIFileProtocolHandler);
}

function GetPrefsService()
{
  if (gPrefsService)
    return gPrefsService;

  try {
    gPrefsService = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);
  }
  catch(ex) {
    dump("failed to get prefs service!\n");
  }

  return gPrefsService;
}

function GetPrefs()
{
  if (gPrefsBranch)
    return gPrefsBranch;

  try {
    var prefService = GetPrefsService();
    if (prefService)
      gPrefsBranch = prefService.getBranch(null);

    if (gPrefsBranch)
      return gPrefsBranch;
    else
      dump("failed to get root prefs!\n");
  }
  catch(ex) {
    dump("failed to get root prefs!\n");
  }
  return null;
}

function GetStringPref(name)
{
  try {
    return GetPrefs().getComplexValue(name, Components.interfaces.nsISupportsString).data;
  } catch (e) {}
  return "";
}

function GetBoolPref(name)
{
  try {
    return GetPrefs().getBoolPref(name);
  } catch (e) {}
  return false;
}

function SetUnicharPref(aPrefName, aPrefValue)
{
  var prefs = GetPrefs();
  if (prefs)
  {
    try {
      var str = Components.classes["@mozilla.org/supports-string;1"]
                          .createInstance(Components.interfaces.nsISupportsString);
      str.data = aPrefValue;
      prefs.setComplexValue(aPrefName, Components.interfaces.nsISupportsString, str);
    }
    catch(e) {}
  }
}

function GetUnicharPref(aPrefName, aDefVal)
{
  var prefs = GetPrefs();
  if (prefs)
  {
    try {
      return prefs.getComplexValue(aPrefName, Components.interfaces.nsISupportsString).data;
    }
    catch(e) {}
  }
  return "";
}


function SetFilePickerDirectory(filePicker, fileType)
{
  if (filePicker)
  {
    try {
      var prefBranch = GetPrefs();
      if (prefBranch)
      {
        
        gFilePickerDirectory = filePicker.displayDirectory;

        var location = prefBranch.getComplexValue("editor.lastFileLocation."+fileType, Components.interfaces.nsILocalFile);
        if (location)
          filePicker.displayDirectory = location;
      }
    }
    catch(e) {}
  }
}


function SaveFilePickerDirectory(filePicker, fileType)
{
  if (filePicker && filePicker.file)
  {
    try {
      var prefBranch = GetPrefs();

      var fileDir;
      if (filePicker.file.parent)
        fileDir = filePicker.file.parent.QueryInterface(Components.interfaces.nsILocalFile);

      if (prefBranch)
       prefBranch.setComplexValue("editor.lastFileLocation."+fileType, Components.interfaces.nsILocalFile, fileDir);
    
      var prefsService = GetPrefsService();
        prefsService.savePrefFile(null);
    } catch (e) {}
  }

  
  
  if (gFilePickerDirectory)
    filePicker.displayDirectory = gFilePickerDirectory;

  gFilePickerDirectory = null;
}

function GetDefaultBrowserColors()
{
  var prefs = GetPrefs();
  var colors = { TextColor:0, BackgroundColor:0, LinkColor:0, ActiveLinkColor:0 , VisitedLinkColor:0 };
  var useSysColors = false;
  try { useSysColors = prefs.getBoolPref("browser.display.use_system_colors"); } catch (e) {}

  if (!useSysColors)
  {
    try { colors.TextColor = prefs.getCharPref("browser.display.foreground_color"); } catch (e) {}

    try { colors.BackgroundColor = prefs.getCharPref("browser.display.background_color"); } catch (e) {}
  }
  
  if (!colors.TextColor)
    colors.TextColor = "windowtext";

  if (!colors.BackgroundColor)
    colors.BackgroundColor = "window";

  colors.LinkColor = prefs.getCharPref("browser.anchor_color");
  colors.ActiveLinkColor = prefs.getCharPref("browser.active_color");
  colors.VisitedLinkColor = prefs.getCharPref("browser.visited_color");

  return colors;
}



function TextIsURI(selectedText)
{
  return selectedText && /^http:\/\/|^https:\/\/|^file:\/\/|\
    ^ftp:\/\/|^about:|^mailto:|^news:|^snews:|^telnet:|^ldap:|\
    ^ldaps:|^gopher:|^finger:|^javascript:/i.test(selectedText);
}

function IsUrlAboutBlank(urlString)
{
  return (urlString == "about:blank");
}

function MakeRelativeUrl(url)
{
  var inputUrl = TrimString(url);
  if (!inputUrl)
    return inputUrl;

  
  
  var docUrl = GetDocumentBaseUrl();
  var docScheme = GetScheme(docUrl);

  
  if (!docScheme)
    return inputUrl;

  var urlScheme = GetScheme(inputUrl);

  
  if (docScheme != urlScheme)
    return inputUrl;

  var IOService = GetIOService();
  if (!IOService)
    return inputUrl;

  
  var docHost = GetHost(docUrl);
  var urlHost = GetHost(inputUrl);
  if (docHost != urlHost)
    return inputUrl;


  
  
  var docPath = IOService.newURI(docUrl, GetCurrentEditor().documentCharacterSet, null).path;
  var urlPath = IOService.newURI(inputUrl, GetCurrentEditor().documentCharacterSet, null).path;

  
  
  var os = GetOS();
  var doCaseInsensitive = (docScheme == "file" && os == gWin);
  if (doCaseInsensitive)
    docPath = docPath.toLowerCase();

  
  var docFilename = GetFilename(docPath);

  
  
  urlPath = urlPath.slice(1);
  docPath = docPath.slice(1);

  var firstDirTest = true;
  var nextDocSlash = 0;
  var done = false;

  
  do {
    nextDocSlash = docPath.indexOf("\/");
    var nextUrlSlash = urlPath.indexOf("\/");

    if (nextUrlSlash == -1)
    {
      
      
      done = true;

      
      if (nextDocSlash == -1 && docFilename)
      { 
        var anchorIndex = urlPath.indexOf("#");
        if (anchorIndex > 0)
        {
          var urlFilename = doCaseInsensitive ? urlPath.toLowerCase() : urlPath;
        
          if (urlFilename.indexOf(docFilename) == 0)
            urlPath = urlPath.slice(anchorIndex);
        }
      }
    }
    else if (nextDocSlash >= 0)
    {
      
      var docDir = docPath.slice(0, nextDocSlash);
      var urlDir = urlPath.slice(0, nextUrlSlash);
      if (doCaseInsensitive)
        urlDir = urlDir.toLowerCase();

      if (urlDir == docDir)
      {

        
        
        docPath = docPath.slice(nextDocSlash+1);
        urlPath = urlPath.slice(nextUrlSlash+1);
      }
      else
      {
        
        done = true;

        
        
        
        
        
        if (firstDirTest && docScheme == "file" && os != gUNIX)
          return inputUrl;
      }
    }
    else  
      done = true;

    firstDirTest = false;
  }
  while (!done);

  
  while (nextDocSlash > 0)
  {
    urlPath = "../" + urlPath;
    nextDocSlash = docPath.indexOf("\/", nextDocSlash+1);
  }
  return urlPath;
}

function MakeAbsoluteUrl(url)
{
  var resultUrl = TrimString(url);
  if (!resultUrl)
    return resultUrl;

  
  var urlScheme = GetScheme(resultUrl);

  if (urlScheme)
    return resultUrl;

  var docUrl = GetDocumentBaseUrl();
  var docScheme = GetScheme(docUrl);

  
  if (!docScheme)
    return resultUrl;

  var  IOService = GetIOService();
  if (!IOService)
    return resultUrl;
  
  
  var absoluteUrl = resultUrl;
  var docUri = IOService.newURI(docUrl, GetCurrentEditor().documentCharacterSet, null);

  try {
    absoluteUrl = docUri.resolve(resultUrl);
    
    
    
  } catch (e) {}

  return absoluteUrl;
}



function GetDocumentBaseUrl()
{
  try {
    var docUrl;

    
    var baseList = GetCurrentEditor().document.getElementsByTagName("base");
    if (baseList)
    {
      var base = baseList.item(0);
      if (base)
        docUrl = base.getAttribute("href");
    }
    if (!docUrl)
      docUrl = GetDocumentUrl();

    if (!IsUrlAboutBlank(docUrl))
      return docUrl;
  } catch (e) {}
  return "";
}

function GetDocumentUrl()
{
  try {
    var aDOMHTMLDoc = GetCurrentEditor().document.QueryInterface(Components.interfaces.nsIDOMHTMLDocument);
    return aDOMHTMLDoc.URL;
  }
  catch (e) {}
  return "";
}


function GetScheme(urlspec)
{
  var resultUrl = TrimString(urlspec);
  
  if (!resultUrl || IsUrlAboutBlank(resultUrl))
    return "";

  var IOService = GetIOService();
  if (!IOService)
    return "";

  var scheme = "";
  try {
    
    scheme = IOService.extractScheme(resultUrl);
  } catch (e) {}

  return scheme ? scheme.toLowerCase() : "";
}

function GetHost(urlspec)
{
  if (!urlspec)
    return "";

  var IOService = GetIOService();
  if (!IOService)
    return "";

  var host = "";
  try {
    host = IOService.newURI(urlspec, null, null).host;
   } catch (e) {}

  return host;
}

function GetUsername(urlspec)
{
  if (!urlspec)
    return "";

  var IOService = GetIOService();
  if (!IOService)
    return "";

  var username = "";
  try {
    username = IOService.newURI(urlspec, null, null).username;
  } catch (e) {}

  return username;
}

function GetFilename(urlspec)
{
  if (!urlspec || IsUrlAboutBlank(urlspec))
    return "";

  var IOService = GetIOService();
  if (!IOService)
    return "";

  var filename;

  try {
    var uri = IOService.newURI(urlspec, null, null);
    if (uri)
    {
      var url = uri.QueryInterface(Components.interfaces.nsIURL);
      if (url)
        filename = url.fileName;
    }
  } catch (e) {}

  return filename ? filename : "";
}




function StripUsernamePassword(urlspec, usernameObj, passwordObj)
{
  urlspec = TrimString(urlspec);
  if (!urlspec || IsUrlAboutBlank(urlspec))
    return urlspec;

  if (usernameObj)
    usernameObj.value = "";
  if (passwordObj)
    passwordObj.value = "";

  
  var atIndex = urlspec.indexOf("@");
  if (atIndex > 0)
  {
    try {
      var IOService = GetIOService();
      if (!IOService)
        return urlspec;

      var uri = IOService.newURI(urlspec, null, null);
      var username = uri.username;
      var password = uri.password;

      if (usernameObj && username)
        usernameObj.value = username;
      if (passwordObj && password)
        passwordObj.value = password;
      if (username)
      {
        var usernameStart = urlspec.indexOf(username);
        if (usernameStart != -1)
          return urlspec.slice(0, usernameStart) + urlspec.slice(atIndex+1);
      }
    } catch (e) {}
  }
  return urlspec;
}

function StripPassword(urlspec, passwordObj)
{
  urlspec = TrimString(urlspec);
  if (!urlspec || IsUrlAboutBlank(urlspec))
    return urlspec;

  if (passwordObj)
    passwordObj.value = "";

  
  var atIndex = urlspec.indexOf("@");
  if (atIndex > 0)
  {
    try {
      var IOService = GetIOService();
      if (!IOService)
        return urlspec;

      var password = IOService.newURI(urlspec, null, null).password;

      if (passwordObj && password)
        passwordObj.value = password;
      if (password)
      {
        
        var colon = urlspec.lastIndexOf(":", atIndex);
        if (colon != -1)
        {
          
          return urlspec.slice(0, colon) + urlspec.slice(atIndex);
        }
      }
    } catch (e) {}
  }
  return urlspec;
}


function StripUsernamePasswordFromURI(uri)
{
  var urlspec = "";
  if (uri)
  {
    try {
      urlspec = uri.spec;
      var userPass = uri.userPass;
      if (userPass)
      {
        start = urlspec.indexOf(userPass);
        urlspec = urlspec.slice(0, start) + urlspec.slice(start+userPass.length+1);
      }
    } catch (e) {}    
  }
  return urlspec;
}

function InsertUsernameIntoUrl(urlspec, username)
{
  if (!urlspec || !username)
    return urlspec;

  try {
    var ioService = GetIOService();
    var URI = ioService.newURI(urlspec, GetCurrentEditor().documentCharacterSet, null);
    URI.username = username;
    return URI.spec;
  } catch (e) {}

  return urlspec;
}

function GetOS()
{
  if (gOS)
    return gOS;

  var platform = navigator.platform.toLowerCase();

  if (platform.indexOf("win") != -1)
    gOS = gWin;
  else if (platform.indexOf("mac") != -1)
    gOS = gMac;
  else if (platform.indexOf("unix") != -1 || platform.indexOf("linux") != -1 || platform.indexOf("sun") != -1)
    gOS = gUNIX;
  else
    gOS = "";
  

  return gOS;
}

function ConvertRGBColorIntoHEXColor(color)
{
  if ( /rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)/.test(color) ) {
    var r = Number(RegExp.$1).toString(16);
    if (r.length == 1) r = "0"+r;
    var g = Number(RegExp.$2).toString(16);
    if (g.length == 1) g = "0"+g;
    var b = Number(RegExp.$3).toString(16);
    if (b.length == 1) b = "0"+b;
    return "#"+r+g+b;
  }
  else
  {
    return color;
  }
}



function GetHTMLOrCSSStyleValue(element, attrName, cssPropertyName)
{
  var prefs = GetPrefs();
  var IsCSSPrefChecked = prefs.getBoolPref("editor.use_css");
  var value;
  if (IsCSSPrefChecked && IsHTMLEditor())
    value = element.style.getPropertyValue(cssPropertyName);

  if (!value)
    value = element.getAttribute(attrName);

  if (!value)
    return "";

  return value;
}



function Clone(obj) 
{ 
  var clone = {};
  for (var i in obj)
  {
    if( typeof obj[i] == 'object')
      clone[i] = Clone(obj[i]);
    else
      clone[i] = obj[i];
  }
  return clone;
}
