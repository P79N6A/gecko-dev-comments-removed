

























































this.EXPORTED_SYMBOLS = [ "BookmarkHTMLUtils" ];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");

const Container_Normal = 0;
const Container_Toolbar = 1;
const Container_Menu = 2;
const Container_Unfiled = 3;
const Container_Places = 4;

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";

const MICROSEC_PER_SEC = 1000000;

const EXPORT_INDENT = "    "; 

#ifdef XP_WIN
const EXPORT_NEWLINE = "\r\n";
#elifdef XP_OS2
const EXPORT_NEWLINE = "\r\n";
#else
const EXPORT_NEWLINE = "\n";
#endif

let serialNumber = 0; 

function base64EncodeString(aString) {
  let stream = Cc["@mozilla.org/io/string-input-stream;1"]
                 .createInstance(Ci.nsIStringInputStream);
  stream.setData(aString, aString.length);
  let encoder = Cc["@mozilla.org/scriptablebase64encoder;1"]
                  .createInstance(Ci.nsIScriptableBase64Encoder);
  return encoder.encodeToString(stream, aString.length);
}

this.BookmarkHTMLUtils = Object.freeze({
  












  importFromURL: function BHU_importFromURL(aUrlString, aInitialImport) {
    let importer = new BookmarkImporter(aInitialImport);
    return importer.importFromURL(aUrlString);
  },

  











  importFromFile: function BHU_importFromFile(aLocalFile, aInitialImport) {
    let importer = new BookmarkImporter(aInitialImport);
    return importer.importFromURL(NetUtil.newURI(aLocalFile).spec);
  },

  









  exportToFile: function BHU_exportToFile(aLocalFile) {
    let exporter = new BookmarkExporter();
    return exporter.exportToFile(aLocalFile);
  },
});

function Frame(aFrameId) {
  this.containerId = aFrameId;

  








  this.containerNesting = 0;

  




  this.lastContainerType = Container_Normal;

  




  this.previousText = "";

  














  this.inDescription = false;

  





  this.previousLink = null; 

  



  this.previousFeed = null; 

  


  this.previousId = 0;

  



  this.previousDateAdded = 0;
  this.previousLastModifiedDate = 0;
}

function BookmarkImporter(aInitialImport) {
  this._isImportDefaults = aInitialImport;
  this._frames = new Array();
  this._frames.push(new Frame(PlacesUtils.bookmarksMenuFolderId));
}

BookmarkImporter.prototype = {

  _safeTrim: function safeTrim(aStr) {
    return aStr ? aStr.trim() : aStr;
  },

  get _curFrame() {
    return this._frames[this._frames.length - 1];
  },

  get _previousFrame() {
    return this._frames[this._frames.length - 2];
  },

  



  _newFrame: function newFrame() {
    let containerId = -1;
    let frame = this._curFrame;
    let containerTitle = frame.previousText;
    frame.previousText = "";
    let containerType = frame.lastContainerType;

    switch (containerType) {
      case Container_Normal:
        
        containerId = 
          PlacesUtils.bookmarks.createFolder(frame.containerId,
                                             containerTitle,
                                             PlacesUtils.bookmarks.DEFAULT_INDEX);
        break;
      case Container_Places:
        containerId = PlacesUtils.placesRootId;
        break;
      case Container_Menu:
        containerId = PlacesUtils.bookmarksMenuFolderId;
        break;
      case Container_Unfiled:
        containerId = PlacesUtils.unfiledBookmarksFolderId;
        break;
      case Container_Toolbar:
        containerId = PlacesUtils.toolbarFolderId;
        break;
      default:
        
        throw new Error("Unreached");
    }

    if (frame.previousDateAdded > 0) {
      try {
        PlacesUtils.bookmarks.setItemDateAdded(containerId, frame.previousDateAdded);
      } catch(e) {
      }
      frame.previousDateAdded = 0;
    }
    if (frame.previousLastModifiedDate > 0) {
      try {
        PlacesUtils.bookmarks.setItemLastModified(containerId, frame.previousLastModifiedDate);
      } catch(e) {
      }
      
    }

    frame.previousId = containerId;

    this._frames.push(new Frame(containerId));
  },

  







  _handleSeparator: function handleSeparator(aElt) {
    let frame = this._curFrame;
    try {
      frame.previousId =
        PlacesUtils.bookmarks.insertSeparator(frame.containerId,
                                              PlacesUtils.bookmarks.DEFAULT_INDEX);
    } catch(e) {}
  },

  




  _handleHead1Begin: function handleHead1Begin(aElt) {
    if (this._frames.length > 1) {
      return;
    }
    if (aElt.hasAttribute("places_root")) {
      this._curFrame.containerId = PlacesUtils.placesRootId;
    }
  },

  






  _handleHeadBegin: function handleHeadBegin(aElt) {
    let frame = this._curFrame;

    
    
    frame.previousLink = null;
    frame.lastContainerType = Container_Normal;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (frame.containerNesting == 0 && this._frames.length > 1) {
      this._frames.pop();
    }

    
    
    
    if (aElt.hasAttribute("personal_toolbar_folder")) {
      if (this._isImportDefaults) {
        frame.lastContainerType = Container_Toolbar;
      }
    } else if (aElt.hasAttribute("bookmarks_menu")) {
      if (this._isImportDefaults) {
        frame.lastContainerType = Container_Menu;
      }
    } else if (aElt.hasAttribute("unfiled_bookmarks_folder")) {
      if (this._isImportDefaults) {
        frame.lastContainerType = Container_Unfiled;
      }
    } else if (aElt.hasAttribute("places_root")) {
      if (this._isImportDefaults) {
        frame.lastContainerType = Container_Places;
      }
    } else {
      let addDate = aElt.getAttribute("add_date");
      if (addDate) {
        frame.previousDateAdded =
          this._convertImportedDateToInternalDate(addDate);
      }
      let modDate = aElt.getAttribute("last_modified");
      if (modDate) {
        frame.previousLastModifiedDate =
          this._convertImportedDateToInternalDate(modDate);
      }
    }
    this._curFrame.previousText = "";
  },

  




  _handleLinkBegin: function handleLinkBegin(aElt) {
    let frame = this._curFrame;

    
    frame.previousFeed = null;
    
    frame.previousId = 0;
    
    frame.previousText = "";

    
    let href = this._safeTrim(aElt.getAttribute("href"));
    let feedUrl = this._safeTrim(aElt.getAttribute("feedurl"));
    let icon = this._safeTrim(aElt.getAttribute("icon"));
    let iconUri = this._safeTrim(aElt.getAttribute("icon_uri"));
    let lastCharset = this._safeTrim(aElt.getAttribute("last_charset"));
    let keyword = this._safeTrim(aElt.getAttribute("shortcuturl"));
    let postData = this._safeTrim(aElt.getAttribute("post_data"));
    let webPanel = this._safeTrim(aElt.getAttribute("web_panel"));
    let micsumGenURI = this._safeTrim(aElt.getAttribute("micsum_gen_uri"));
    let generatedTitle = this._safeTrim(aElt.getAttribute("generated_title"));
    let dateAdded = this._safeTrim(aElt.getAttribute("add_date"));
    let lastModified = this._safeTrim(aElt.getAttribute("last_modified"));

    
    
    if (feedUrl) {
      frame.previousFeed = NetUtil.newURI(feedUrl);
    }

    
    if (href) {
      
      
      try {
        frame.previousLink = NetUtil.newURI(href);
      } catch(e) {
        if (!frame.previousFeed) {
          frame.previousLink = null;
          return;
        }
      }
    } else {
      frame.previousLink = null;
      
      
      if (!frame.previousFeed) {
        return;
      }
    }

    
    if (lastModified) {
      frame.previousLastModifiedDate =
        this._convertImportedDateToInternalDate(lastModified);
    }

    
    
    if (frame.previousFeed) {
      return;
    }

    
    try {
      frame.previousId =
        PlacesUtils.bookmarks.insertBookmark(frame.containerId,
                                             frame.previousLink,
                                             PlacesUtils.bookmarks.DEFAULT_INDEX,
                                             "");
    } catch(e) {
      return;
    }

    
    if (dateAdded) {
      try {
        PlacesUtils.bookmarks.setItemDateAdded(frame.previousId,
          this._convertImportedDateToInternalDate(dateAdded));
      } catch(e) {
      }
    }

    
    if (icon || iconUri) {
      let iconUriObject;
      try {
        iconUriObject = NetUtil.newURI(iconUri);
      } catch(e) {
      }
      if (icon || iconUriObject) {
        try {
          this._setFaviconForURI(frame.previousLink, iconUriObject, icon);
        } catch(e) {
        }
      }
    }

    
    if (keyword) {
      try {
        PlacesUtils.bookmarks.setKeywordForBookmark(frame.previousId, keyword);
        if (postData) {
          PlacesUtils.annotations.setItemAnnotation(frame.previousId,
                                                    PlacesUtils.POST_DATA_ANNO,
                                                    postData,
                                                    0,
                                                    PlacesUtils.annotations.EXPIRE_NEVER);
        }
      } catch(e) {
      }
    }

    
    if (webPanel && webPanel.toLowerCase() == "true") {
      try {
        PlacesUtils.annotations.setItemAnnotation(frame.previousId,
                                                  LOAD_IN_SIDEBAR_ANNO,
                                                  1,
                                                  0,
                                                  PlacesUtils.annotations.EXPIRE_NEVER);
      } catch(e) {
      }
    }

    
    if (lastCharset) {
      PlacesUtils.setCharsetForURI(frame.previousLink, lastCharset);
    }
  },

  _handleContainerBegin: function handleContainerBegin() {
    this._curFrame.containerNesting++;
  },

  




  _handleContainerEnd: function handleContainerEnd() {
    let frame = this._curFrame;
    if (frame.containerNesting > 0)
      frame.containerNesting --;
    if (this._frames.length > 1 && frame.containerNesting == 0) {
      
      
      let prevFrame = this._previousFrame;
      if (prevFrame.previousLastModifiedDate > 0) {
        PlacesUtils.bookmarks.setItemLastModified(frame.containerId,
                                                  prevFrame.previousLastModifiedDate);
      }
      this._frames.pop();
    }
  },

  




  _handleHeadEnd: function handleHeadEnd() {
    this._newFrame();
  },

  


  _handleLinkEnd: function handleLinkEnd() {
    let frame = this._curFrame;
    frame.previousText = frame.previousText.trim();

    try {
      if (frame.previousFeed) {
        
        
        PlacesUtils.livemarks.addLivemark({
          "title": frame.previousText,
          "parentId": frame.containerId,
          "index": PlacesUtils.bookmarks.DEFAULT_INDEX,
          "feedURI": frame.previousFeed,
          "siteURI": frame.previousLink,
        });
      } else if (frame.previousLink) {
        
        PlacesUtils.bookmarks.setItemTitle(frame.previousId,
                                           frame.previousText);
      }
    } catch(e) {
    }


    
    if (frame.previousId > 0 && frame.previousLastModifiedDate > 0) {
      try {
        PlacesUtils.bookmarks.setItemLastModified(frame.previousId,
                                                  frame.previousLastModifiedDate);
      } catch(e) {
      }
      
      
    }

    frame.previousText = "";

  },

  _openContainer: function openContainer(aElt) {
    if (aElt.namespaceURI != "http://www.w3.org/1999/xhtml") {
      return;
    }
    switch(aElt.localName) {
      case "h1":
        this._handleHead1Begin(aElt);
        break;
      case "h2":
      case "h3":
      case "h4":
      case "h5":
      case "h6":
        this._handleHeadBegin(aElt);
        break;
      case "a":
        this._handleLinkBegin(aElt);
        break;
      case "dl":
      case "ul":
      case "menu":
        this._handleContainerBegin();
        break;
      case "dd":
        this._curFrame.inDescription = true;
        break;
      case "hr":
        this._handleSeparator(aElt);
        break;
    }
  },

  _closeContainer: function closeContainer(aElt) {
    let frame = this._curFrame;

    
    
    
    if (frame.inDescription) {
      
      frame.previousText = frame.previousText.trim(); 
      if (frame.previousText) {

        let itemId = !frame.previousLink ? frame.containerId
                                         : frame.previousId;

        try {
          if (!PlacesUtils.annotations.itemHasAnnotation(itemId, DESCRIPTION_ANNO)) {
            PlacesUtils.annotations.setItemAnnotation(itemId,
                                                      DESCRIPTION_ANNO,
                                                      frame.previousText,
                                                      0,
                                                      PlacesUtils.annotations.EXPIRE_NEVER);
          }
        } catch(e) {
        }
        frame.previousText = "";

        
        
        
        
        
        
        
        
        
        

        let lastModified;
        if (!frame.previousLink) {
          lastModified = this._previousFrame.previousLastModifiedDate;
        } else {
          lastModified = frame.previousLastModifiedDate;
        }

        if (itemId > 0 && lastModified > 0) {
          PlacesUtils.bookmarks.setItemLastModified(itemId, lastModified);
        }
      }
      frame.inDescription = false;
    }

    if (aElt.namespaceURI != "http://www.w3.org/1999/xhtml") {
      return;
    }
    switch(aElt.localName) {
      case "dl":
      case "ul":
      case "menu":
        this._handleContainerEnd();
        break;
      case "dt":
        break;
      case "h1":
        
        break;
      case "h2":
      case "h3":
      case "h4":
      case "h5":
      case "h6":
        this._handleHeadEnd();
        break;
      case "a":
        this._handleLinkEnd();
        break;
      default:
        break;
    }
  },

  _appendText: function appendText(str) {
    this._curFrame.previousText += str;
  },

  











  _setFaviconForURI: function setFaviconForURI(aPageURI, aIconURI, aData) {
    
    
    if (aIconURI) {
      if (aIconURI.schemeIs("chrome")) {
        PlacesUtils.favicons.setAndFetchFaviconForPage(aPageURI, aIconURI,
                                                       false,
                                                       PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
        return;
      }
    }

    
    
    if (aData.length <= 5) {
      return;
    }

    let faviconURI;
    if (aIconURI) {
      faviconURI = aIconURI;
    } else {
      
      
      
      let faviconSpec = "http://www.mozilla.org/2005/made-up-favicon/"
                      + serialNumber
                      + "-"
                      + new Date().getTime();
      faviconURI = NetUtil.newURI(faviconSpec);
      serialNumber++;
    }

    
    
    
    PlacesUtils.favicons.replaceFaviconDataFromDataURL(faviconURI, aData);
    PlacesUtils.favicons.setAndFetchFaviconForPage(aPageURI, faviconURI, false, PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
  },

  


  _convertImportedDateToInternalDate: function convertImportedDateToInternalDate(aDate) {
    if (aDate && !isNaN(aDate)) {
      return parseInt(aDate) * 1000000; 
    } else {
      return Date.now();
    }
  },

  runBatched: function runBatched(aDoc) {
    if (!aDoc) {
      return;
    }

    if (this._isImportDefaults) {
      PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.bookmarksMenuFolderId);
      PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.toolbarFolderId);
      PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
    }

    let current = aDoc;
    let next;
    for (;;) {
      switch (current.nodeType) {
        case Ci.nsIDOMNode.ELEMENT_NODE:
          this._openContainer(current);
          break;
        case Ci.nsIDOMNode.TEXT_NODE:
          this._appendText(current.data);
          break;
      }
      if ((next = current.firstChild)) {
        current = next;
        continue;
      }
      for (;;) {
        if (current.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
          this._closeContainer(current);
        }
        if (current == aDoc) {
          return;
        }
        if ((next = current.nextSibling)) {
          current = next;
          break;
        }
        current = current.parentNode;
      }
    }
  },

  _walkTreeForImport: function walkTreeForImport(aDoc) {
    PlacesUtils.bookmarks.runInBatchMode(this, aDoc);
  },

  _notifyObservers: function notifyObservers(topic) {
    Services.obs.notifyObservers(null,
                                 topic,
                                 this._isImportDefaults ? "html-initial"
                                                        : "html");
  },

  importFromURL: function importFromURL(aUrlString, aCallback) {
    let deferred = Promise.defer();
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.onload = (function onload() {
      try {
        this._walkTreeForImport(xhr.responseXML);
        this._notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_SUCCESS);
        deferred.resolve();
      } catch(e) {
        this._notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
        deferred.reject(e);
        throw e;
      }
    }).bind(this);
    xhr.onabort = xhr.onerror = xhr.ontimeout = (function handleFail() {
      this._notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
      deferred.reject(new Error("xmlhttprequest failed"));
    }).bind(this);
    this._notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_BEGIN);
    try {
      xhr.open("GET", aUrlString);
      xhr.responseType = "document";
      xhr.overrideMimeType("text/html");
      xhr.send();
    } catch (e) {
      this._notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
      deferred.reject(e);
    }
    return deferred.promise;
  },

};

function BookmarkExporter() { }

BookmarkExporter.prototype = {

  



  escapeHtml: function escapeHtml(aText) {
    return (aText || "").replace("&", "&amp;", "g")
                        .replace("<", "&lt;", "g")
                        .replace(">", "&gt;", "g")
                        .replace("\"", "&quot;", "g")
                        .replace("'", "&#39;", "g");
  },

  



  escapeUrl: function escapeUrl(aText) {
    return (aText || "").replace("\"", "%22", "g");
  },

  exportToFile: function exportToFile(aLocalFile) {
    return Task.spawn(this._doExportToFile(aLocalFile));
  },

  _doExportToFile: function doExportToFile(aLocalFile) {
    
    let safeFileOut = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                      .createInstance(Ci.nsIFileOutputStream);
    safeFileOut.init(aLocalFile,
                     FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE
                                           | FileUtils.MODE_TRUNCATE,
                     parseInt("0600", 8), 0);
    try {
      
      let bufferedOut = Cc["@mozilla.org/network/buffered-output-stream;1"]
                        .createInstance(Ci.nsIBufferedOutputStream);
      bufferedOut.init(safeFileOut, 4096);
      try {
        
        this._converterOut = Cc["@mozilla.org/intl/converter-output-stream;1"]
                             .createInstance(Ci.nsIConverterOutputStream);
        this._converterOut.init(bufferedOut, "utf-8", 0, 0);
        try {
          yield this._doExport();

          
          bufferedOut.QueryInterface(Ci.nsISafeOutputStream).finish();
        } finally {
          this._converterOut.close();
          this._converterOut = null;
        }
      } finally {
        bufferedOut.close();
      }
    } finally {
      safeFileOut.close();
    }
  },

  _converterOut: null,

  _write: function write(aText) {
    this._converterOut.writeString(aText || "");
  },

  _writeLine: function writeLine(aText) {
    this._write(aText + EXPORT_NEWLINE);
  },

  _doExport: function doExport() {
    this._writeLine("<!DOCTYPE NETSCAPE-Bookmark-file-1>");
    this._writeLine("<!-- This is an automatically generated file.");
    this._writeLine("     It will be read and overwritten.");
    this._writeLine("     DO NOT EDIT! -->");
    this._writeLine("<META HTTP-EQUIV=\"Content-Type\"" +
                    " CONTENT=\"text/html; charset=UTF-8\">");
    this._writeLine("<TITLE>Bookmarks</TITLE>");

    
    let root = PlacesUtils.getFolderContents(
                                    PlacesUtils.bookmarksMenuFolderId).root;
    try {
      this._writeLine("<H1>" + this.escapeHtml(root.title) + "</H1>");
      this._writeLine("");
      this._writeLine("<DL><p>");
      yield this._writeContainerContents(root, "");
    } finally {
      root.containerOpen = false;
    }

    
    root = PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;
    try {
      if (root.childCount > 0) {
        yield this._writeContainer(root, EXPORT_INDENT);
      }
    } finally {
      root.containerOpen = false;
    }

    
    root = PlacesUtils.getFolderContents(
                                PlacesUtils.unfiledBookmarksFolderId).root;
    try {
      if (root.childCount > 0) {
        yield this._writeContainer(root, EXPORT_INDENT);
      }
    } finally {
      root.containerOpen = false;
    }

    this._writeLine("</DL><p>");
  },

  _writeContainer: function writeContainer(aItem, aIndent) {
    this._write(aIndent + "<DT><H3");
    yield this._writeDateAttributes(aItem);

    if (aItem.itemId == PlacesUtils.placesRootId) {
      this._write(" PLACES_ROOT=\"true\"");
    } else if (aItem.itemId == PlacesUtils.bookmarksMenuFolderId) {
      this._write(" BOOKMARKS_MENU=\"true\"");
    } else if (aItem.itemId == PlacesUtils.unfiledBookmarksFolderId) {
      this._write(" UNFILED_BOOKMARKS_FOLDER=\"true\"");
    } else if (aItem.itemId == PlacesUtils.toolbarFolderId) {
      this._write(" PERSONAL_TOOLBAR_FOLDER=\"true\"");
    }

    this._writeLine(">" + this.escapeHtml(aItem.title) + "</H3>");
    yield this._writeDescription(aItem);
    this._writeLine(aIndent + "<DL><p>");
    yield this._writeContainerContents(aItem, aIndent);
    this._writeLine(aIndent + "</DL><p>");
  },

  _writeContainerContents: function writeContainerContents(aItem, aIndent) {
    let localIndent = aIndent + EXPORT_INDENT;

    for (let i = 0; i < aItem.childCount; ++i) {
      let child = aItem.getChild(i);
      if (child.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER) {
        
        
        if (PlacesUtils.annotations
                       .itemHasAnnotation(child.itemId,
                                          PlacesUtils.LMANNO_FEEDURI)) {
          yield this._writeLivemark(child, localIndent);
        } else {
          
          PlacesUtils.asContainer(child).containerOpen = true;
          try {
            yield this._writeContainer(child, localIndent);
          } finally {
            child.containerOpen = false;
          }
        }
      } else if (child.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
        yield this._writeSeparator(child, localIndent);
      } else {
        yield this._writeItem(child, localIndent);
      }
    }
  },

  _writeSeparator: function writeSeparator(aItem, aIndent) {
    this._write(aIndent + "<HR");

    
    let title = null;
    try {
      title = PlacesUtils.bookmarks.getItemTitle(aItem.itemId);
    } catch (ex) { }

    if (title) {
      this._write(" NAME=\"" + this.escapeHtml(title) + "\"");
    }

    this._write(">");
  },

  _writeLivemark: function writeLivemark(aItem, aIndent) {
    this._write(aIndent + "<DT><A");
    let feedSpec = PlacesUtils.annotations
                              .getItemAnnotation(aItem.itemId,
                                                 PlacesUtils.LMANNO_FEEDURI);
    this._write(" FEEDURL=\"" + this.escapeUrl(feedSpec) + "\"");

    
    try {
      let siteSpec = PlacesUtils.annotations
                                .getItemAnnotation(aItem.itemId,
                                                   PlacesUtils.LMANNO_SITEURI);
      if (siteSpec) {
        this._write(" HREF=\"" + this.escapeUrl(siteSpec) + "\"");
      }
    } catch (ex) { }

    this._writeLine(">" + this.escapeHtml(aItem.title) + "</A>");
    yield this._writeDescription(aItem);
  },

  _writeItem: function writeItem(aItem, aIndent) {
    let itemUri = null;
    try {
      itemUri = NetUtil.newURI(aItem.uri);
    } catch (ex) {
      
      return;
    }

    this._write(aIndent + "<DT><A HREF=\"" + this.escapeUrl(aItem.uri) + "\"");
    yield this._writeDateAttributes(aItem);
    yield this._writeFaviconAttribute(itemUri);

    let keyword = PlacesUtils.bookmarks.getKeywordForBookmark(aItem.itemId);
    if (keyword) {
      this._write(" SHORTCUTURL=\"" + this.escapeHtml(keyword) + "\"");
    }

    if (PlacesUtils.annotations.itemHasAnnotation(aItem.itemId,
                                                  PlacesUtils.POST_DATA_ANNO)) {
      let postData = PlacesUtils.annotations
                                .getItemAnnotation(aItem.itemId,
                                                   PlacesUtils.POST_DATA_ANNO);
      this._write(" POST_DATA=\"" + this.escapeHtml(postData) + "\"");
    }

    if (PlacesUtils.annotations.itemHasAnnotation(aItem.itemId,
                                                  LOAD_IN_SIDEBAR_ANNO)) {
      this._write(" WEB_PANEL=\"true\"");
    }

    try {
      let lastCharset = yield PlacesUtils.getCharsetForURI(itemUri);
      if (lastCharset) {
        this._write(" LAST_CHARSET=\"" + this.escapeHtml(lastCharset) + "\"");
      }
    } catch(ex) { }

    this._writeLine(">" + this.escapeHtml(aItem.title) + "</A>");
    yield this._writeDescription(aItem);
  },

  _writeDateAttributes: function writeDateAttributes(aItem) {
    if (aItem.dateAdded) {
      this._write(" ADD_DATE=\"" +
                  Math.floor(aItem.dateAdded / MICROSEC_PER_SEC) + "\"");
    }
    if (aItem.lastModified) {
      this._write(" LAST_MODIFIED=\"" +
                  Math.floor(aItem.lastModified / MICROSEC_PER_SEC) + "\"");
    }
  },

  _writeFaviconAttribute: function writeFaviconAttribute(aItemUri) {
    let [faviconURI, dataLen, data] = yield this._promiseFaviconData(aItemUri);

    if (!faviconURI) {
      
      return;
    }

    this._write(" ICON_URI=\"" + this.escapeUrl(faviconURI.spec) + "\"");

    if (!faviconURI.schemeIs("chrome") && dataLen > 0) {
      let faviconContents = "data:image/png;base64," +
        base64EncodeString(String.fromCharCode.apply(String, data));
      this._write(" ICON=\"" + faviconContents + "\"");
    }
  },

  _promiseFaviconData: function(aPageURI) {
    var deferred = Promise.defer();
    PlacesUtils.favicons.getFaviconDataForPage(aPageURI,
      function (aURI, aDataLen, aData, aMimeType) {
        deferred.resolve([aURI, aDataLen, aData, aMimeType]);
      });
    return deferred.promise;
  },

  _writeDescription: function writeDescription(aItem) {
    if (PlacesUtils.annotations.itemHasAnnotation(aItem.itemId,
                                                  DESCRIPTION_ANNO)) {
      let description = PlacesUtils.annotations
                                   .getItemAnnotation(aItem.itemId,
                                                      DESCRIPTION_ANNO);
      
      this._writeLine("<DD>" + this.escapeHtml(description));
    }
  },

};
