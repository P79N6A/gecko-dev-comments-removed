









































function isContentFrame(aFocusedWindow)
{
  if (!aFocusedWindow)
    return false;

  return (aFocusedWindow.top == window.content);
}

function urlSecurityCheck(url, doc)
{
  
  var focusedWindow = doc.commandDispatcher.focusedWindow;
  var sourceURL = getContentFrameURI(focusedWindow);
  const nsIScriptSecurityManager = Components.interfaces.nsIScriptSecurityManager;
  var secMan = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                         .getService(nsIScriptSecurityManager);
  try {
    secMan.checkLoadURIStr(sourceURL, url, nsIScriptSecurityManager.STANDARD);
  } catch (e) {
    throw "Load of " + url + " denied.";
  }
}

function getContentFrameURI(aFocusedWindow)
{
  var contentFrame = isContentFrame(aFocusedWindow) ? aFocusedWindow : window.content;
  return contentFrame.location.href;
}

function getContentFrameDocument(aFocusedWindow)
{
  var contentFrame = isContentFrame(aFocusedWindow) ?
                       aFocusedWindow : window.content;
  return contentFrame.document;
}

function getReferrer(doc)
{
  var focusedWindow = doc.commandDispatcher.focusedWindow;
  var sourceDocument = getContentFrameDocument(focusedWindow);

  try {
    return makeURI(sourceDocument.location.href, sourceDocument.characterSet);
  } catch (e) {
    return null;
  }
}

function openNewWindowWith(url, sendReferrer)
{
  urlSecurityCheck(url, document);

  
  
  
  var charsetArg = null;
  var wintype = document.documentElement.getAttribute('windowtype');
  if (wintype == "navigator:browser")
    charsetArg = "charset=" + window.content.document.characterSet;

  var referrer = sendReferrer ? getReferrer(document) : null;
  window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no", url, charsetArg, referrer);
}

function openTopBrowserWith(url)
{
  urlSecurityCheck(url, document);

  var windowMediator = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
  var browserWin = windowMediator.getMostRecentWindow("navigator:browser");

  
  if (browserWin) {
    browserWin.getBrowser().loadURI(url); 
    browserWin.content.focus();
  }
  else
    window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no", url, null, null);
}

function openNewTabWith(url, sendReferrer, reverseBackgroundPref)
{
  var browser;
  try {
    
    
    browser = getBrowser();

  } catch (ex if ex instanceof ReferenceError) {

    
    
    
    var windowMediator =
      Components.classes["@mozilla.org/appshell/window-mediator;1"]
      .getService(Components.interfaces.nsIWindowMediator);

    var browserWin = windowMediator.getMostRecentWindow("navigator:browser");

    
    
    
    if (!browserWin) {
      urlSecurityCheck(url, document);
      window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no",
                        url, null, referrer);
      return;
    }

    
    
    browser = browserWin.getBrowser();
  }

  
  
  
  var browserDocument = browser.ownerDocument;

  urlSecurityCheck(url, browserDocument);

  var referrer = sendReferrer ? getReferrer(browserDocument) : null;

  
  
  var wintype = browserDocument.documentElement.getAttribute('windowtype');
  var originCharset;
  if (wintype == "navigator:browser") {
    originCharset = window.content.document.characterSet;
  }

  
  var loadInBackground = false;
  if (pref) {
    loadInBackground = pref.getBoolPref("browser.tabs.loadInBackground");
    if (reverseBackgroundPref)
      loadInBackground = !loadInBackground;
  }
  browser.addTab(url, referrer, originCharset, !loadInBackground);
}


















function saveURL(aURL, aFileName, aFilePickerTitleKey, aShouldBypassCache,
                 aReferrer)
{
  internalSave(aURL, null, aFileName, null, null, aShouldBypassCache,
               aFilePickerTitleKey, null, aReferrer);
}





const imgICache = Components.interfaces.imgICache;
const nsISupportsCString = Components.interfaces.nsISupportsCString;

function saveImageURL(aURL, aFileName, aFilePickerTitleKey, aShouldBypassCache,
                      aReferrer)
{
  var contentType = null;
  var contentDisposition = null;
  if (!aShouldBypassCache) {
    try {
      var imageCache = Components.classes["@mozilla.org/image/cache;1"]
                                 .getService(imgICache);
      var props =
        imageCache.findEntryProperties(makeURI(aURL, getCharsetforSave(null)));
      if (props) {
        contentType = props.get("type", nsISupportsCString);
        contentDisposition = props.get("content-disposition",
                                       nsISupportsCString);
      }
    } catch (e) {
      
    }
  }
  internalSave(aURL, null, aFileName, contentDisposition, contentType,
               aShouldBypassCache, aFilePickerTitleKey, null, aReferrer);
}

function saveFrameDocument()
{
  var focusedWindow = document.commandDispatcher.focusedWindow;
  if (isContentFrame(focusedWindow))
    saveDocument(focusedWindow.document);
}

function saveDocument(aDocument)
{
  if (!aDocument) {
    throw "Must have a document when calling saveDocument";
  }

  
  var dispHeader = null;
  try {
    dispHeader =
      aDocument.defaultView
               .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
               .getInterface(Components.interfaces.nsIDOMWindowUtils)
               .getDocumentMetadata("content-disposition");
  } catch (ex) {
    
  }
  internalSave(aDocument.location.href, aDocument, null, dispHeader,
               aDocument.contentType, false, null, null);
}

const SAVETYPE_COMPLETE_PAGE = 0x00;
const SAVETYPE_TEXT_ONLY     = 0x02;


























function internalSave(aURL, aDocument, aDefaultFileName, aContentDisposition,
                      aContentType, aShouldBypassCache, aFilePickerTitleKey,
                      aChosenData, aReferrer)
{
  
  var saveMode = GetSaveModeForContentType(aContentType);
  var isDocument = aDocument != null && saveMode != SAVEMODE_FILEONLY;
  var saveAsType = SAVETYPE_COMPLETE_PAGE;

  var file, fileURL;
  
  
  var fileInfo = new FileInfo(aDefaultFileName);
  if (aChosenData)
    file = aChosenData.file;
  else {
    var charset = null;
    if (aDocument)
      charset = aDocument.characterSet;
    else if (aReferrer)
      charset = aReferrer.originCharset;
    initFileInfo(fileInfo, aURL, charset, aDocument,
                 aContentType, aContentDisposition);
    var fpParams = {
      fpTitleKey: aFilePickerTitleKey,
      isDocument: isDocument,
      fileInfo: fileInfo,
      contentType: aContentType,
      saveMode: saveMode,
      saveAsType: saveAsType,
      file: file,
      fileURL: fileURL
    };
    if (!poseFilePicker(fpParams))
      
      
      return;

    saveAsType = fpParams.saveAsType;
    saveMode = fpParams.saveMode;
    file = fpParams.file;
    fileURL = fpParams.fileURL;
  }

  if (!fileURL)
    fileURL = makeFileURI(file);

  
  
  
  var useSaveDocument = isDocument &&
                        (((saveMode & SAVEMODE_COMPLETE_DOM) && (saveAsType == SAVETYPE_COMPLETE_PAGE)) ||
                         ((saveMode & SAVEMODE_COMPLETE_TEXT) && (saveAsType == SAVETYPE_TEXT_ONLY)));

  
  
  
  var source = useSaveDocument ? aDocument : fileInfo.uri;
  var persistArgs = {
    source      : source,
    contentType : (!aChosenData && useSaveDocument &&
                   saveAsType == SAVETYPE_TEXT_ONLY) ?
                  "text/plain" : aContentType,
    target      : fileURL,
    postData    : isDocument ? getPostData() : null,
    bypassCache : aShouldBypassCache
  };

  var persist = makeWebBrowserPersist();

  
  const nsIWBP = Components.interfaces.nsIWebBrowserPersist;
  const flags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES;
  if (aShouldBypassCache)
    persist.persistFlags = flags | nsIWBP.PERSIST_FLAGS_BYPASS_CACHE;
  else
    persist.persistFlags = flags | nsIWBP.PERSIST_FLAGS_FROM_CACHE;

  
  persist.persistFlags |= nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

  
  var tr = Components.classes["@mozilla.org/transfer;1"].createInstance(Components.interfaces.nsITransfer);

  if (useSaveDocument) {
    
    var filesFolder = null;
    if (persistArgs.contentType != "text/plain") {
      
      filesFolder = file.clone();

      var nameWithoutExtension = filesFolder.leafName.replace(/\.[^.]*$/, "");
      var filesFolderLeafName = getStringBundle().formatStringFromName("filesFolder",
                                                                       [nameWithoutExtension],
                                                                       1);

      filesFolder.leafName = filesFolderLeafName;
    }

    var encodingFlags = 0;
    if (persistArgs.contentType == "text/plain") {
      encodingFlags |= nsIWBP.ENCODE_FLAGS_FORMATTED;
      encodingFlags |= nsIWBP.ENCODE_FLAGS_ABSOLUTE_LINKS;
      encodingFlags |= nsIWBP.ENCODE_FLAGS_NOFRAMES_CONTENT;
    }
    else {
      encodingFlags |= nsIWBP.ENCODE_FLAGS_ENCODE_BASIC_ENTITIES;
    }

    const kWrapColumn = 80;
    tr.init((aChosenData ? aChosenData.uri : fileInfo.uri),
            persistArgs.target, "", null, null, null, persist);
    persist.progressListener = tr;
    persist.saveDocument(persistArgs.source, persistArgs.target, filesFolder,
                         persistArgs.contentType, encodingFlags, kWrapColumn);
  } else {
    tr.init((aChosenData ? aChosenData.uri : source),
            persistArgs.target, "", null, null, null, persist);
    persist.progressListener = tr;
    persist.saveURI((aChosenData ? aChosenData.uri : source),
                    null, aReferrer, persistArgs.postData, null,
                    persistArgs.target);
  }
}









function AutoChosen(aFileAutoChosen, aUriAutoChosen) {
  this.file = aFileAutoChosen;
  this.uri  = aUriAutoChosen;
}











function FileInfo(aSuggestedFileName, aFileName, aFileBaseName, aFileExt, aUri) {
  this.suggestedFileName = aSuggestedFileName;
  this.fileName = aFileName;
  this.fileBaseName = aFileBaseName;
  this.fileExt = aFileExt;
  this.uri = aUri;
}














function initFileInfo(aFI, aURL, aURLCharset, aDocument,
                      aContentType, aContentDisposition)
{
  try {
    
    try {
      aFI.uri = makeURI(aURL, aURLCharset);
      
      
      var url = aFI.uri.QueryInterface(Components.interfaces.nsIURL);
      aFI.fileExt = url.fileExtension;
    } catch (e) {
    }

    
    aFI.fileName = getDefaultFileName((aFI.suggestedFileName || aFI.fileName),
                                      aFI.uri, aDocument, aContentDisposition);
    
    
    
    
    if (!aFI.fileExt && !aDocument && !aContentType && (/^http(s?):\/\//i.test(aURL))) {
      aFI.fileExt = "htm";
      aFI.fileBaseName = aFI.fileName;
    } else {
      aFI.fileExt    = getDefaultExtension(aFI.fileName, aFI.uri, aContentType);
      aFI.fileBaseName = getFileBaseName(aFI.fileName, aFI.fileExt);
    }
  } catch (e) {
  }
}









function poseFilePicker(aFpP)
{
  const nsILocalFile = Components.interfaces.nsILocalFile;
  const kDownloadDirPref = "dir";

  var branch = getPrefsBrowserDownload("browser.download.");
  var dir = null;

  
  try {
    dir = branch.getComplexValue(kDownloadDirPref, nsILocalFile);
  } catch (e) {
  }

  var autoDownload = branch.getBoolPref("autoDownload");
  if (autoDownload && dir && dir.exists()) {
    dir.append(getNormalizedLeafName(aFpP.fileInfo.fileName, aFpP.fileInfo.fileExt));
    aFpP.file = uniqueFile(dir);
    return true;
  }

  
  var fp = makeFilePicker();
  var titleKey = aFpP.fpTitleKey || "SaveLinkTitle";
  var bundle = getStringBundle();
  fp.init(window, bundle.GetStringFromName(titleKey),
          Components.interfaces.nsIFilePicker.modeSave);

  try {
    if (dir.exists())
      fp.displayDirectory = dir;
  } catch (e) {
  }

  fp.defaultExtension = aFpP.fileInfo.fileExt;
  fp.defaultString = getNormalizedLeafName(aFpP.fileInfo.fileName,
                                           aFpP.fileInfo.fileExt);
  appendFiltersForContentType(fp, aFpP.contentType, aFpP.fileInfo.fileExt,
                              aFpP.saveMode);

  if (aFpP.isDocument) {
    try {
      fp.filterIndex = branch.getIntPref("save_converter_index");
    }
    catch (e) {
    }
  }

  if (fp.show() == Components.interfaces.nsIFilePicker.returnCancel || !fp.file)
    return false;

  if (aFpP.isDocument) 
    branch.setIntPref("save_converter_index", fp.filterIndex);

  
  
  if (branch.getBoolPref("lastLocation") || autoDownload) {
    var directory = fp.file.parent.QueryInterface(nsILocalFile);
    branch.setComplexValue(kDownloadDirPref, nsILocalFile, directory);
  }
  fp.file.leafName = validateFileName(fp.file.leafName);

  aFpP.saveAsType = fp.filterIndex;
  aFpP.file = fp.file;
  aFpP.fileURL = fp.fileURL;
  return true;
}








function uniqueFile(aLocalFile)
{
  while (aLocalFile.exists()) {
    parts = /(-\d+)?(\.[^.]+)?$/.test(aLocalFile.leafName);
    aLocalFile.leafName = RegExp.leftContext + (RegExp.$1 - 1) + RegExp.$2;
  }
  return aLocalFile;
}


const SAVEMODE_FILEONLY      = 0x00;

const SAVEMODE_COMPLETE_DOM  = 0x01;

const SAVEMODE_COMPLETE_TEXT = 0x02;





function appendFiltersForContentType(aFilePicker, aContentType, aFileExtension, aSaveMode)
{
  var bundle = getStringBundle();
  
  var bundleName;
  
  var filterString;

  
  
  switch (aContentType) {
  case "text/html":
    bundleName   = "WebPageHTMLOnlyFilter";
    filterString = "*.htm; *.html";
    break;

  case "application/xhtml+xml":
    bundleName   = "WebPageXHTMLOnlyFilter";
    filterString = "*.xht; *.xhtml";
    break;

  case "image/svg+xml":
    bundleName   = "WebPageSVGOnlyFilter";
    filterString = "*.svg; *.svgz";
    break;

  case "text/xml":
  case "application/xml":
    bundleName   = "WebPageXMLOnlyFilter";
    filterString = "*.xml";
    break;

  default:
    if (aSaveMode != SAVEMODE_FILEONLY) {
      throw "Invalid save mode for type '" + aContentType + "'";
    }

    var mimeInfo = getMIMEInfoForType(aContentType, aFileExtension);
    if (mimeInfo) {

      var extEnumerator = mimeInfo.getFileExtensions();

      var extString = "";
      while (extEnumerator.hasMore()) {
        var extension = extEnumerator.getNext();
        if (extString)
          extString += "; ";    
                                
        extString += "*." + extension;
      }

      if (extString) {
        aFilePicker.appendFilter(mimeInfo.description, extString);
      }
    }

    break;
  }

  if (aSaveMode & SAVEMODE_COMPLETE_DOM) {
    aFilePicker.appendFilter(bundle.GetStringFromName("WebPageCompleteFilter"), filterString);
    
    
    aFilePicker.appendFilter(bundle.GetStringFromName(bundleName), filterString);
  }

  if (aSaveMode & SAVEMODE_COMPLETE_TEXT) {
    aFilePicker.appendFilters(Components.interfaces.nsIFilePicker.filterText);
  }

  
  aFilePicker.appendFilters(Components.interfaces.nsIFilePicker.filterAll);
}

function getPostData()
{
  try {
    var sessionHistory = getWebNavigation().sessionHistory;
    return sessionHistory.getEntryAtIndex(sessionHistory.index, false)
                         .QueryInterface(Components.interfaces.nsISHEntry)
                         .postData;
  }
  catch (e) {
  }
  return null;
}

function getStringBundle()
{
  const bundleURL = "chrome://communicator/locale/contentAreaCommands.properties";

  const sbsContractID = "@mozilla.org/intl/stringbundle;1";
  const sbsIID = Components.interfaces.nsIStringBundleService;
  const sbs = Components.classes[sbsContractID].getService(sbsIID);

  const lsContractID = "@mozilla.org/intl/nslocaleservice;1";
  const lsIID = Components.interfaces.nsILocaleService;
  const ls = Components.classes[lsContractID].getService(lsIID);
  var appLocale = ls.getApplicationLocale();
  return sbs.createBundle(bundleURL, appLocale);
}


function getPrefsBrowserDownload(branch)
{
  const prefSvcContractID = "@mozilla.org/preferences-service;1";
  const prefSvcIID = Components.interfaces.nsIPrefService;                              
  return Components.classes[prefSvcContractID].getService(prefSvcIID).getBranch(branch);
}

function makeWebBrowserPersist()
{
  const persistContractID = "@mozilla.org/embedding/browser/nsWebBrowserPersist;1";
  const persistIID = Components.interfaces.nsIWebBrowserPersist;
  return Components.classes[persistContractID].createInstance(persistIID);
}







function makeURI(aURL, aOriginCharset)
{
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);
  return ioService.newURI(aURL, aOriginCharset, null);
}

function makeFileURI(aFile)
{
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);
  return ioService.newFileURI(aFile);
}

function makeFilePicker()
{
  const fpContractID = "@mozilla.org/filepicker;1";
  const fpIID = Components.interfaces.nsIFilePicker;
  return Components.classes[fpContractID].createInstance(fpIID);
}

function getMIMEService()
{
  const mimeSvcContractID = "@mozilla.org/mime;1";
  const mimeSvcIID = Components.interfaces.nsIMIMEService;
  const mimeSvc = Components.classes[mimeSvcContractID].getService(mimeSvcIID);
  return mimeSvc;
}


function getFileBaseName(aFileName, aFileExt)
{
  
  return aFileName.replace(/\.[^.]*$/, "");
}

function getMIMETypeForURI(aURI)
{
  try {
    return getMIMEService().getTypeFromURI(aURI);
  }
  catch (e) {
  }
  return null;
}

function getMIMEInfoForType(aMIMEType, aExtension)
{
  if (aMIMEType || aExtension) {
    try {
      return getMIMEService().getFromTypeAndExtension(aMIMEType, aExtension);
    }
    catch (e) {
    }
  }
  return null;
}

function getDefaultFileName(aDefaultFileName, aURI, aDocument,
                            aContentDisposition)
{
  
  if (aContentDisposition) {
    const mhpContractID = "@mozilla.org/network/mime-hdrparam;1";
    const mhpIID = Components.interfaces.nsIMIMEHeaderParam;
    const mhp = Components.classes[mhpContractID].getService(mhpIID);
    var dummy = { value: null };  
    var charset = getCharsetforSave(aDocument);

    var fileName = null;
    try {
      fileName = mhp.getParameter(aContentDisposition, "filename", charset,
                                  true, dummy);
    }
    catch (e) {
      try {
        fileName = mhp.getParameter(aContentDisposition, "name", charset, true,
                                    dummy);
      }
      catch (e) {
      }
    }
    if (fileName) {
      return fileName;
    }
  }

  try {
    var url = aURI.QueryInterface(Components.interfaces.nsIURL);
    if (url.fileName != "") {
      
      var textToSubURI = Components.classes["@mozilla.org/intl/texttosuburi;1"]
                                   .getService(Components.interfaces.nsITextToSubURI);
      return validateFileName(textToSubURI.unEscapeURIForUI(url.originCharset || "UTF-8", url.fileName));
    }
  } catch (e) {
    
  }

  if (aDocument) {
    var docTitle = GenerateValidFilename(aDocument.title, "");

    if (docTitle) {
      
      return docTitle;
    }
  }

  if (aDefaultFileName)
    
    return validateFileName(aDefaultFileName);

  
  var path = aURI.path.match(/\/([^\/]+)\/$/);
  if (path && path.length > 1) {
    return validateFileName(path[1]);
  }

  try {
    if (aURI.host)
      
      return aURI.host;
  } catch (e) {
    
  }
  try {
    
    return getStringBundle().GetStringFromName("DefaultSaveFileName");
  } catch (e) {
    
  }
  
  return "index";
}

function getNormalizedLeafName(aFile, aDefaultExtension)
{
  if (!aDefaultExtension)
    return aFile;

  
  if (/Win/.test(navigator.platform))
    aFile = aFile.replace(/[\s.]+$/, "");

  
  var i = aFile.lastIndexOf(".");
  if (aFile.substr(i + 1) != aDefaultExtension)
    return aFile + "." + aDefaultExtension;

  return aFile;
}

function getDefaultExtension(aFilename, aURI, aContentType)
{
  if (aContentType == "text/plain" || aContentType == "application/octet-stream" || aURI.scheme == "ftp")
    return "";   

  
  const stdURLContractID = "@mozilla.org/network/standard-url;1";
  const stdURLIID = Components.interfaces.nsIURL;
  var url = Components.classes[stdURLContractID].createInstance(stdURLIID);
  url.filePath = aFilename;

  var ext = url.fileExtension;

  
  

  var mimeInfo = getMIMEInfoForType(aContentType, ext);

  if (ext && mimeInfo && mimeInfo.extensionExists(ext)) {
    return ext;
  }

  
  var urlext;
  try {
    url = aURI.QueryInterface(Components.interfaces.nsIURL);
    urlext = url.fileExtension;
  } catch (e) {
  }

  if (urlext && mimeInfo && mimeInfo.extensionExists(urlext)) {
    return urlext;
  }
  else {
    try {
      if (mimeInfo)
        return mimeInfo.primaryExtension;
    }
    catch (e) {
    }
    
    
    return ext || urlext;
  }
}

function GetSaveModeForContentType(aContentType)
{
  var saveMode = SAVEMODE_FILEONLY;
  switch (aContentType) {
  case "text/html":
  case "application/xhtml+xml":
  case "image/svg+xml":
    saveMode |= SAVEMODE_COMPLETE_TEXT;
    
  case "text/xml":
  case "application/xml":
    saveMode |= SAVEMODE_COMPLETE_DOM;
    break;
  }

  return saveMode;
}

function getCharsetforSave(aDocument)
{
  if (aDocument)
    return aDocument.characterSet;

  if (document.commandDispatcher.focusedWindow)
    return document.commandDispatcher.focusedWindow.document.characterSet;

  return  window.content.document.characterSet;
}
