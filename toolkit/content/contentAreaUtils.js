# -*- indent-tabs-mode: nil; js-indent-level: 2 -*- 
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadLastDir",
                                  "resource://gre/modules/DownloadLastDir.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
var ContentAreaUtils = {

  
  get ioService() {
    return Services.io;
  },

  get stringBundle() {
    delete this.stringBundle;
    return this.stringBundle =
      Services.strings.createBundle("chrome://global/locale/contentAreaCommands.properties");
  }
}

function urlSecurityCheck(aURL, aPrincipal, aFlags)
{
  return BrowserUtils.urlSecurityCheck(aURL, aPrincipal, aFlags);
}




function isContentFrame(aFocusedWindow)
{
  if (!aFocusedWindow)
    return false;

  return (aFocusedWindow.top == window.content);
}



















function saveURL(aURL, aFileName, aFilePickerTitleKey, aShouldBypassCache,
                 aSkipPrompt, aReferrer, aSourceDocument)
{
  internalSave(aURL, null, aFileName, null, null, aShouldBypassCache,
               aFilePickerTitleKey, null, aReferrer, aSourceDocument,
               aSkipPrompt, null);
}





const imgICache = Components.interfaces.imgICache;
const nsISupportsCString = Components.interfaces.nsISupportsCString;

function saveImageURL(aURL, aFileName, aFilePickerTitleKey, aShouldBypassCache,
                      aSkipPrompt, aReferrer, aDoc)
{
  var contentType = null;
  var contentDisposition = null;
  if (!aShouldBypassCache) {
    try {
      var imageCache = Components.classes["@mozilla.org/image/tools;1"]
                                 .getService(Components.interfaces.imgITools)
                                 .getImgCacheForDocument(aDoc);
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
               aDoc, aSkipPrompt, null);
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
               aDocument, aSkipPrompt, cacheKey);
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
                      aChosenData, aReferrer, aInitiatingDocument, aSkipPrompt,
                      aCacheKey)
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

    continueSave();
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

    
    let relatedURI = aReferrer || sourceURI;

    promiseTargetFile(fpParams, aSkipPrompt, relatedURI).then(aDialogAccepted => {
      if (!aDialogAccepted)
        return;

      saveAsType = fpParams.saveAsType;
      file = fpParams.file;

      continueSave();
    }).then(null, Components.utils.reportError);
  }

  function continueSave() {
    
    
    
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
      bypassCache       : aShouldBypassCache,
      initiatingWindow  : aInitiatingDocument.defaultView
    };

    
    internalPersist(persistArgs);
  }
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

  var isPrivate = PrivateBrowsingUtils.isWindowPrivate(persistArgs.initiatingWindow);

  
  var tr = Components.classes["@mozilla.org/transfer;1"].createInstance(Components.interfaces.nsITransfer);
  tr.init(persistArgs.sourceURI,
          targetFileURL, "", null, null, null, persist, isPrivate);
  persist.progressListener = new DownloadListener(window, tr);

  if (persistArgs.sourceDocument) {
    
    var filesFolder = null;
    if (persistArgs.targetContentType != "text/plain") {
      
      filesFolder = persistArgs.targetFile.clone();

      var nameWithoutExtension = getFileBaseName(filesFolder.leafName);
      var filesFolderLeafName =
        ContentAreaUtils.stringBundle
                        .formatStringFromName("filesFolder", [nameWithoutExtension], 1);

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
    let privacyContext = persistArgs.initiatingWindow
                                    .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                    .getInterface(Components.interfaces.nsIWebNavigation)
                                    .QueryInterface(Components.interfaces.nsILoadContext);
    persist.saveURI(persistArgs.sourceURI,
                    persistArgs.sourceCacheKey, persistArgs.sourceReferrer,
                    Components.interfaces.nsIHttpChannel.REFERRER_POLICY_NO_REFERRER_WHEN_DOWNGRADE,
                    persistArgs.sourcePostData, null, targetFileURL, privacyContext);
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






















function promiseTargetFile(aFpP,  aSkipPrompt,  aRelatedURI)
{
  return Task.spawn(function() {
    let downloadLastDir = new DownloadLastDir(window);
    let prefBranch = Services.prefs.getBranch("browser.download.");
    let useDownloadDir = prefBranch.getBoolPref("useDownloadDir");

    if (!aSkipPrompt)
      useDownloadDir = false;

    
    
    let dirPath = yield Downloads.getPreferredDownloadsDirectory();
    let dirExists = yield OS.File.exists(dirPath);
    let dir = new FileUtils.File(dirPath);

    if (useDownloadDir && dirExists) {
      dir.append(getNormalizedLeafName(aFpP.fileInfo.fileName,
                                       aFpP.fileInfo.fileExt));
      aFpP.file = uniqueFile(dir);
      throw new Task.Result(true);
    }

    
    
    let deferred = Promise.defer();
    if (useDownloadDir) {
      
      Services.tm.mainThread.dispatch(function() {
        deferred.resolve(null);
      }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
    } else {
      downloadLastDir.getFileAsync(aRelatedURI, function getFileAsyncCB(aFile) {
        deferred.resolve(aFile);
      });
    }
    let file = yield deferred.promise;
    if (file && (yield OS.File.exists(file.path))) {
      dir = file;
      dirExists = true;
    }

    if (!dirExists) {
      
      dir = Services.dirsvc.get("Desk", Components.interfaces.nsIFile);
    }

    let fp = makeFilePicker();
    let titleKey = aFpP.fpTitleKey || "SaveLinkTitle";
    fp.init(window, ContentAreaUtils.stringBundle.GetStringFromName(titleKey),
            Components.interfaces.nsIFilePicker.modeSave);

    fp.displayDirectory = dir;
    fp.defaultExtension = aFpP.fileInfo.fileExt;
    fp.defaultString = getNormalizedLeafName(aFpP.fileInfo.fileName,
                                             aFpP.fileInfo.fileExt);
    appendFiltersForContentType(fp, aFpP.contentType, aFpP.fileInfo.fileExt,
                                aFpP.saveMode);

    
    
    if (aFpP.saveMode != SAVEMODE_FILEONLY) {
      try {
        fp.filterIndex = prefBranch.getIntPref("save_converter_index");
      }
      catch (e) {
      }
    }

    let deferComplete = Promise.defer();
    fp.open(function(aResult) {
      deferComplete.resolve(aResult);
    });
    let result = yield deferComplete.promise;
    if (result == Components.interfaces.nsIFilePicker.returnCancel || !fp.file) {
      throw new Task.Result(false);
    }

    if (aFpP.saveMode != SAVEMODE_FILEONLY)
      prefBranch.setIntPref("save_converter_index", fp.filterIndex);

    
    downloadLastDir.setFile(aRelatedURI, fp.file.parent);

    fp.file.leafName = validateFileName(fp.file.leafName);

    aFpP.saveAsType = fp.filterIndex;
    aFpP.file = fp.file;
    aFpP.fileURL = fp.fileURL;

    throw new Task.Result(true);
  });
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

#ifdef MOZ_JSDOWNLOADS










function DownloadURL(aURL, aFileName, aInitiatingDocument) {
  
  
  let isPrivate = aInitiatingDocument.defaultView
                                     .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                     .getInterface(Components.interfaces.nsIWebNavigation)
                                     .QueryInterface(Components.interfaces.nsILoadContext)
                                     .usePrivateBrowsing;

  let fileInfo = new FileInfo(aFileName);
  initFileInfo(fileInfo, aURL, null, null, null, null);

  let filepickerParams = {
    fileInfo: fileInfo,
    saveMode: SAVEMODE_FILEONLY
  };

  Task.spawn(function* () {
    let accepted = yield promiseTargetFile(filepickerParams, true, fileInfo.uri);
    if (!accepted)
      return;

    let file = filepickerParams.file;
    let download = yield Downloads.createDownload({
      source: { url: aURL, isPrivate: isPrivate },
      target: { path: file.path, partFilePath: file.path + ".part" }
    });
    download.tryToKeepPartialData = true;
    download.start();

    
    let list = yield Downloads.getList(Downloads.ALL);
    list.add(download);
  }).then(null, Components.utils.reportError);
}
#endif


const SAVEMODE_FILEONLY      = 0x00;

const SAVEMODE_COMPLETE_DOM  = 0x01;

const SAVEMODE_COMPLETE_TEXT = 0x02;





function appendFiltersForContentType(aFilePicker, aContentType, aFileExtension, aSaveMode)
{
  
  var bundleName;
  
  var filterString;

  
  
  if (aSaveMode != SAVEMODE_FILEONLY) {
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
    }
  }

  if (!bundleName) {
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
  }

  if (aSaveMode & SAVEMODE_COMPLETE_DOM) {
    aFilePicker.appendFilter(ContentAreaUtils.stringBundle.GetStringFromName("WebPageCompleteFilter"),
                             filterString);
    
    
    aFilePicker.appendFilter(ContentAreaUtils.stringBundle.GetStringFromName(bundleName),
                             filterString);
  }

  if (aSaveMode & SAVEMODE_COMPLETE_TEXT)
    aFilePicker.appendFilters(Components.interfaces.nsIFilePicker.filterText);

  
  aFilePicker.appendFilters(Components.interfaces.nsIFilePicker.filterAll);
}

function getPostData(aDocument)
{
  try {
    
    
    
    var sessionHistoryEntry =
        aDocument.defaultView
                 .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                 .getInterface(Components.interfaces.nsIWebNavigation)
                 .QueryInterface(Components.interfaces.nsIWebPageDescriptor)
                 .currentDescriptor
                 .QueryInterface(Components.interfaces.nsISHEntry);
    return sessionHistoryEntry.postData;
  }
  catch (e) {
  }
  return null;
}

function makeWebBrowserPersist()
{
  const persistContractID = "@mozilla.org/embedding/browser/nsWebBrowserPersist;1";
  const persistIID = Components.interfaces.nsIWebBrowserPersist;
  return Components.classes[persistContractID].createInstance(persistIID);
}

function makeURI(aURL, aOriginCharset, aBaseURI)
{
  return BrowserUtils.makeURI(aURL, aOriginCharset, aBaseURI);
}

function makeFileURI(aFile)
{
  return BrowserUtils.makeFileURI(aFile);
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

  let docTitle;
  if (aDocument) {
    
    docTitle = validateFileName(aDocument.title).trim();
    if (docTitle) {
      let contentType = aDocument.contentType;
      if (contentType == "application/xhtml+xml" ||
          contentType == "application/xml" ||
          contentType == "image/svg+xml" ||
          contentType == "text/html" ||
          contentType == "text/xml") {
        
        return docTitle;
      }
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

  if (docTitle)
    
    return docTitle;

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
    
    return ContentAreaUtils.stringBundle.GetStringFromName("DefaultSaveFileName");
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
  else if (navigator.appVersion.indexOf("Android") != -1) {
    
    
    
    const dangerousChars = "*?<>|\":/\\[];,+=";
    var processed = "";
    for (var i = 0; i < aFileName.length; i++)
      processed += aFileName.charCodeAt(i) >= 32 &&
                   !(dangerousChars.indexOf(aFileName[i]) >= 0) ? aFileName[i]
                                                                : "_";

    
    processed = processed.trim();

    
    
    if (processed.replace(/_/g, "").length <= processed.length/2) {
      
      
      
      
      var original = processed;
      processed = "download";

      
      if (original.indexOf(".") >= 0) {
        var suffix = original.split(".").slice(-1)[0];
        if (suffix && suffix.indexOf("_") < 0)
          processed += "." + suffix;
      }
    }
    return processed;
  }

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
  
  
  if (!aDocument || Components.utils.isCrossProcessWrapper(aDocument))
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
    var recentWindow = Services.wm.getMostRecentWindow("navigator:browser");
    if (recentWindow) {
      recentWindow.openUILinkIn(uri.spec, "tab");
      return;
    }

    var loadgroup = Components.classes["@mozilla.org/network/load-group;1"]
                              .createInstance(Components.interfaces.nsILoadGroup);
    var appstartup = Services.startup;

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

    var channel = Services.io.newChannelFromURI2(uri,
                                                 null,      
                                                 Services.scriptSecurityManager.getSystemPrincipal(),
                                                 null,      
                                                 Ci.nsILoadInfo.SEC_NORMAL,
                                                 Ci.nsIContentPolicy.TYPE_OTHER);
    var uriLoader = Components.classes["@mozilla.org/uriloader;1"]
                              .getService(Components.interfaces.nsIURILoader);
    uriLoader.openURI(channel,
                      Components.interfaces.nsIURILoader.IS_CONTENT_PREFERRED,
                      uriListener);
  }
}
