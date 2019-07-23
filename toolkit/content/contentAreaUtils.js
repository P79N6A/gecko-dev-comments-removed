# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- 
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@netscape.com> (Save File)
#   Fredrik Holmqvist <thesuckiestemail@yahoo.se>
#   Asaf Romano <mozilla.mano@sent.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
#   Kathleen Brade <brade@pearlcrescent.com>
#   Mark Smith <mcs@pearlcrescent.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

var ContentAreaUtils = {
  get ioService() {
    delete this.ioService;
    return this.ioService =
      Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);
  }
}
















function urlSecurityCheck(aURL, aPrincipal, aFlags)
{
  const nsIScriptSecurityManager =
    Components.interfaces.nsIScriptSecurityManager;
  var secMan = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                         .getService(nsIScriptSecurityManager);
  if (aFlags === undefined)
    aFlags = nsIScriptSecurityManager.STANDARD;

  try {
    if (aURL instanceof Components.interfaces.nsIURI)
      secMan.checkLoadURIWithPrincipal(aPrincipal, aURL, aFlags);
    else
      secMan.checkLoadURIStrWithPrincipal(aPrincipal, aURL, aFlags);
  } catch (e) {
    
    throw "Load of " + aURL + " denied.";
  }
}




function isContentFrame(aFocusedWindow)
{
  if (!aFocusedWindow)
    return false;

  return (aFocusedWindow.top == window.content);
}




















function saveURL(aURL, aFileName, aFilePickerTitleKey, aShouldBypassCache,
                 aSkipPrompt, aReferrer)
{
  internalSave(aURL, null, aFileName, null, null, aShouldBypassCache,
               aFilePickerTitleKey, null, aReferrer, aSkipPrompt, null);
}





const imgICache = Components.interfaces.imgICache;
const nsISupportsCString = Components.interfaces.nsISupportsCString;

function saveImageURL(aURL, aFileName, aFilePickerTitleKey, aShouldBypassCache,
                      aSkipPrompt, aReferrer)
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
               aShouldBypassCache, aFilePickerTitleKey, null, aReferrer,
               aSkipPrompt, null);
}

function saveDocument(aDocument, aSkipPrompt)
{
  if (!aDocument)
    throw "Must have a document when calling saveDocument";

  
  var ifreq =
    aDocument.defaultView
             .QueryInterface(Components.interfaces.nsIInterfaceRequestor);

  var contentDisposition = null;
  try {
    contentDisposition =
      ifreq.getInterface(Components.interfaces.nsIDOMWindowUtils)
           .getDocumentMetadata("content-disposition");
  } catch (ex) {
    
  }

  var cacheKey = null;
  try {
    cacheKey =
      ifreq.getInterface(Components.interfaces.nsIWebNavigation)
           .QueryInterface(Components.interfaces.nsIWebPageDescriptor);
  } catch (ex) {
    
  }

  internalSave(aDocument.location.href, aDocument, null, contentDisposition,
               aDocument.contentType, false, null, null,
               aDocument.referrer ? makeURI(aDocument.referrer) : null,
               aSkipPrompt, cacheKey);
}

function DownloadListener(win, transfer) {
  function makeClosure(name) {
    return function() {
      transfer[name].apply(transfer, arguments);
    }
  }

  this.window = win;

  
  for (var i in transfer) {
    if (i != "QueryInterface")
      this[i] = makeClosure(i);
  }
}

DownloadListener.prototype = {
  QueryInterface: function dl_qi(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIInterfaceRequestor) ||
        aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsIWebProgressListener2) ||
        aIID.equals(Components.interfaces.nsISupports)) {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  getInterface: function dl_gi(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIAuthPrompt) ||
        aIID.equals(Components.interfaces.nsIAuthPrompt2)) {
      var ww =
        Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                  .getService(Components.interfaces.nsIPromptFactory);
      return ww.getPrompt(this.window, aIID);
    }

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

const kSaveAsType_Complete = 0; 

const kSaveAsType_Text     = 2; 























































function internalSave(aURL, aDocument, aDefaultFileName, aContentDisposition,
                      aContentType, aShouldBypassCache, aFilePickerTitleKey,
                      aChosenData, aReferrer, aSkipPrompt, aCacheKey)
{
  if (aSkipPrompt == undefined)
    aSkipPrompt = false;

  if (aCacheKey == undefined)
    aCacheKey = null;

  
  var saveMode = GetSaveModeForContentType(aContentType, aDocument);

  var file, sourceURI, saveAsType;
  
  
  if (aChosenData) {
    file = aChosenData.file;
    sourceURI = aChosenData.uri;
    saveAsType = kSaveAsType_Complete;
  } else {
    var charset = null;
    if (aDocument)
      charset = aDocument.characterSet;
    else if (aReferrer)
      charset = aReferrer.originCharset;
    var fileInfo = new FileInfo(aDefaultFileName);
    initFileInfo(fileInfo, aURL, charset, aDocument,
                 aContentType, aContentDisposition);
    sourceURI = fileInfo.uri;

    var fpParams = {
      fpTitleKey: aFilePickerTitleKey,
      fileInfo: fileInfo,
      contentType: aContentType,
      saveMode: saveMode,
      saveAsType: kSaveAsType_Complete,
      file: file
    };

    if (!getTargetFile(fpParams, aSkipPrompt))
      
      
      return;

    saveAsType = fpParams.saveAsType;
    file = fpParams.file;
  }

  
  
  
  var useSaveDocument = aDocument &&
                        (((saveMode & SAVEMODE_COMPLETE_DOM) && (saveAsType == kSaveAsType_Complete)) ||
                         ((saveMode & SAVEMODE_COMPLETE_TEXT) && (saveAsType == kSaveAsType_Text)));
  
  
  
  var persistArgs = {
    sourceURI         : sourceURI,
    sourceReferrer    : aReferrer,
    sourceDocument    : useSaveDocument ? aDocument : null,
    targetContentType : (saveAsType == kSaveAsType_Text) ? "text/plain" : null,
    targetFile        : file,
    sourceCacheKey    : aCacheKey,
    sourcePostData    : aDocument ? getPostData(aDocument) : null,
    bypassCache       : aShouldBypassCache
  };

  
  internalPersist(persistArgs);
}





























function internalPersist(persistArgs)
{
  var persist = makeWebBrowserPersist();

  
  const nsIWBP = Components.interfaces.nsIWebBrowserPersist;
  const flags = nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                nsIWBP.PERSIST_FLAGS_FORCE_ALLOW_COOKIES;
  if (persistArgs.bypassCache)
    persist.persistFlags = flags | nsIWBP.PERSIST_FLAGS_BYPASS_CACHE;
  else
    persist.persistFlags = flags | nsIWBP.PERSIST_FLAGS_FROM_CACHE;

  
  persist.persistFlags |= nsIWBP.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;

  
  var targetFileURL = makeFileURI(persistArgs.targetFile);

  
  var tr = Components.classes["@mozilla.org/transfer;1"].createInstance(Components.interfaces.nsITransfer);
  tr.init(persistArgs.sourceURI,
          targetFileURL, "", null, null, null, persist);
  persist.progressListener = new DownloadListener(window, tr);

  if (persistArgs.sourceDocument) {
    
    var filesFolder = null;
    if (persistArgs.targetContentType != "text/plain") {
      
      filesFolder = persistArgs.targetFile.clone();

      var nameWithoutExtension = getFileBaseName(filesFolder.leafName);
      var filesFolderLeafName = getStringBundle().formatStringFromName("filesFolder",
                                                                       [nameWithoutExtension],
                                                                       1);

      filesFolder.leafName = filesFolderLeafName;
    }

    var encodingFlags = 0;
    if (persistArgs.targetContentType == "text/plain") {
      encodingFlags |= nsIWBP.ENCODE_FLAGS_FORMATTED;
      encodingFlags |= nsIWBP.ENCODE_FLAGS_ABSOLUTE_LINKS;
      encodingFlags |= nsIWBP.ENCODE_FLAGS_NOFRAMES_CONTENT;
    }
    else {
      encodingFlags |= nsIWBP.ENCODE_FLAGS_ENCODE_BASIC_ENTITIES;
    }

    const kWrapColumn = 80;
    persist.saveDocument(persistArgs.sourceDocument, targetFileURL, filesFolder,
                         persistArgs.targetContentType, encodingFlags, kWrapColumn);
  } else {
    persist.saveURI(persistArgs.sourceURI,
                    persistArgs.sourceCacheKey, persistArgs.sourceReferrer, persistArgs.sourcePostData, null,
                    targetFileURL);
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
      aFI.fileExt = getDefaultExtension(aFI.fileName, aFI.uri, aContentType);
      aFI.fileBaseName = getFileBaseName(aFI.fileName);
    }
  } catch (e) {
  }
}

















function getTargetFile(aFpP,  aSkipPrompt)
{
  if (typeof gDownloadLastDir != "object")
    Components.utils.import("resource:

  var prefs = getPrefsBrowserDownload("browser.download.");
  var useDownloadDir = prefs.getBoolPref("useDownloadDir");
  const nsILocalFile = Components.interfaces.nsILocalFile;

  if (!aSkipPrompt)
    useDownloadDir = false;

  
  
  var dlMgr = Components.classes["@mozilla.org/download-manager;1"]
                        .getService(Components.interfaces.nsIDownloadManager);
  var dir = dlMgr.userDownloadsDirectory;
  var dirExists = dir && dir.exists();

  if (useDownloadDir && dirExists) {
    dir.append(getNormalizedLeafName(aFpP.fileInfo.fileName,
                                     aFpP.fileInfo.fileExt));
    aFpP.file = uniqueFile(dir);
    return true;
  }

  
  
  if (!useDownloadDir) try {
    
    
    
    
    var lastDir = gDownloadLastDir.file;
    if (lastDir.exists()) {
      dir = lastDir;
      dirExists = true;
    }
  } catch(e) {}

  if (!dirExists) {
    
    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    dir = fileLocator.get("Desk", nsILocalFile);
  }

  var fp = makeFilePicker();
  var titleKey = aFpP.fpTitleKey || "SaveLinkTitle";
  var bundle = getStringBundle();
  fp.init(window, bundle.GetStringFromName(titleKey),
          Components.interfaces.nsIFilePicker.modeSave);

  fp.displayDirectory = dir;
  fp.defaultExtension = aFpP.fileInfo.fileExt;
  fp.defaultString = getNormalizedLeafName(aFpP.fileInfo.fileName,
                                           aFpP.fileInfo.fileExt);
  appendFiltersForContentType(fp, aFpP.contentType, aFpP.fileInfo.fileExt,
                              aFpP.saveMode);

  
  
  if (aFpP.saveMode != SAVEMODE_FILEONLY) {
    try {
      fp.filterIndex = prefs.getIntPref("save_converter_index");
    }
    catch (e) {
    }
  }

  if (fp.show() == Components.interfaces.nsIFilePicker.returnCancel || !fp.file)
    return false;

  if (aFpP.saveMode != SAVEMODE_FILEONLY)
    prefs.setIntPref("save_converter_index", fp.filterIndex);

  
  var directory = fp.file.parent.QueryInterface(nsILocalFile);
  gDownloadLastDir.file = directory;

  fp.file.leafName = validateFileName(fp.file.leafName);
  
  aFpP.saveAsType = fp.filterIndex;
  aFpP.file = fp.file;
  aFpP.fileURL = fp.fileURL;
  return true;
}








function uniqueFile(aLocalFile)
{
  var collisionCount = 0;
  while (aLocalFile.exists()) {
    collisionCount++;
    if (collisionCount == 1) {
      
      
      if (aLocalFile.leafName.match(/\.[^\.]{1,3}\.(gz|bz2|Z)$/i))
        aLocalFile.leafName = aLocalFile.leafName.replace(/\.[^\.]{1,3}\.(gz|bz2|Z)$/i, "(2)$&");
      else
        aLocalFile.leafName = aLocalFile.leafName.replace(/(\.[^\.]*)?$/, "(2)$&");
    }
    else {
      
      aLocalFile.leafName = aLocalFile.leafName.replace(/^(.*\()\d+\)/, "$1" + (collisionCount + 1) + ")");
    }
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
    if (aSaveMode != SAVEMODE_FILEONLY)
      throw "Invalid save mode for type '" + aContentType + "'";

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

      if (extString)
        aFilePicker.appendFilter(mimeInfo.description, extString);
    }

    break;
  }

  if (aSaveMode & SAVEMODE_COMPLETE_DOM) {
    aFilePicker.appendFilter(bundle.GetStringFromName("WebPageCompleteFilter"), filterString);
    
    
    aFilePicker.appendFilter(bundle.GetStringFromName(bundleName), filterString);
  }

  if (aSaveMode & SAVEMODE_COMPLETE_TEXT)
    aFilePicker.appendFilters(Components.interfaces.nsIFilePicker.filterText);

  
  aFilePicker.appendFilters(Components.interfaces.nsIFilePicker.filterAll);
}

function getPostData(aDocument)
{
  try {
    var sessionHistory = aDocument.defaultView
                                  .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                  .getInterface(Components.interfaces.nsIWebNavigation)
                                  .sessionHistory;
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
  return Components.classes["@mozilla.org/intl/stringbundle;1"]
                   .getService(Components.interfaces.nsIStringBundleService)
                   .createBundle("chrome://global/locale/contentAreaCommands.properties");
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








function makeURI(aURL, aOriginCharset, aBaseURI)
{
  return ContentAreaUtils.ioService.newURI(aURL, aOriginCharset, aBaseURI);
}

function makeFileURI(aFile)
{
  return ContentAreaUtils.ioService.newFileURI(aFile);
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


function getFileBaseName(aFileName)
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
    if (fileName)
      return fileName;
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
    var docTitle = validateFileName(aDocument.title).replace(/^\s+|\s+$/g, "");
    if (docTitle) {
      
      return docTitle;
    }
  }

  if (aDefaultFileName)
    
    return validateFileName(aDefaultFileName);

  
  var path = aURI.path.match(/\/([^\/]+)\/$/);
  if (path && path.length > 1)
    return validateFileName(path[1]);

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

function validateFileName(aFileName)
{
  var re = /[\/]+/g;
  if (navigator.appVersion.indexOf("Windows") != -1) {
    re = /[\\\/\|]+/g;
    aFileName = aFileName.replace(/[\"]+/g, "'");
    aFileName = aFileName.replace(/[\*\:\?]+/g, " ");
    aFileName = aFileName.replace(/[\<]+/g, "(");
    aFileName = aFileName.replace(/[\>]+/g, ")");
  }
  else if (navigator.appVersion.indexOf("Macintosh") != -1)
    re = /[\:\/]+/g;
  
  return aFileName.replace(re, "_");
}

function getNormalizedLeafName(aFile, aDefaultExtension)
{
  if (!aDefaultExtension)
    return aFile;

#ifdef XP_WIN
  
  aFile = aFile.replace(/[\s.]+$/, "");
#endif

  
  aFile = aFile.replace(/^\.+/, "");

  
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

  if (ext && mimeInfo && mimeInfo.extensionExists(ext))
    return ext;

  
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

function GetSaveModeForContentType(aContentType, aDocument)
{
  
  if (!aDocument)
    return SAVEMODE_FILEONLY;

  
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

  return window.content.document.characterSet;
}






function openURL(aURL)
{
  var uri = makeURI(aURL);

  var protocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"]
                              .getService(Components.interfaces.nsIExternalProtocolService);

  if (!protocolSvc.isExposedProtocol(uri.scheme)) {
    
    protocolSvc.loadUrl(uri);
  }
  else {
    var loadgroup = Components.classes["@mozilla.org/network/load-group;1"]
                              .createInstance(Components.interfaces.nsILoadGroup);
    var appstartup = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                               .getService(Components.interfaces.nsIAppStartup);

    var loadListener = {
      onStartRequest: function ll_start(aRequest, aContext) {
        appstartup.enterLastWindowClosingSurvivalArea();
      },
      onStopRequest: function ll_stop(aRequest, aContext, aStatusCode) {
        appstartup.exitLastWindowClosingSurvivalArea();
      },
      QueryInterface: function ll_QI(iid) {
        if (iid.equals(Components.interfaces.nsISupports) ||
            iid.equals(Components.interfaces.nsIRequestObserver) ||
            iid.equals(Components.interfaces.nsISupportsWeakReference))
          return this;
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
    }
    loadgroup.groupObserver = loadListener;

    var uriListener = {
      onStartURIOpen: function(uri) { return false; },
      doContent: function(ctype, preferred, request, handler) { return false; },
      isPreferred: function(ctype, desired) { return false; },
      canHandleContent: function(ctype, preferred, desired) { return false; },
      loadCookie: null,
      parentContentListener: null,
      getInterface: function(iid) {
        if (iid.equals(Components.interfaces.nsIURIContentListener))
          return this;
        if (iid.equals(Components.interfaces.nsILoadGroup))
          return loadgroup;
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
    }

    var channel = ContentAreaUtils.ioService.newChannelFromURI(uri);
    var uriLoader = Components.classes["@mozilla.org/uriloader;1"]
                              .getService(Components.interfaces.nsIURILoader);
    uriLoader.openURI(channel, true, uriListener);
  }
}
