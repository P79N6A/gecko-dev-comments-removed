

























































this.EXPORTED_SYMBOLS = [ "BookmarkHTMLUtils" ];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
  "resource://gre/modules/PlacesBackups.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");

const Container_Normal = 0;
const Container_Toolbar = 1;
const Container_Menu = 2;
const Container_Unfiled = 3;
const Container_Places = 4;

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";

const MICROSEC_PER_SEC = 1000000;

const EXPORT_INDENT = "    "; 


let serialNumber = 0;

function base64EncodeString(aString) {
  let stream = Cc["@mozilla.org/io/string-input-stream;1"]
                 .createInstance(Ci.nsIStringInputStream);
  stream.setData(aString, aString.length);
  let encoder = Cc["@mozilla.org/scriptablebase64encoder;1"]
                  .createInstance(Ci.nsIScriptableBase64Encoder);
  return encoder.encodeToString(stream, aString.length);
}





function escapeHtmlEntities(aText) {
  return (aText || "").replace(/&/g, "&amp;")
                      .replace(/</g, "&lt;")
                      .replace(/>/g, "&gt;")
                      .replace(/"/g, "&quot;")
                      .replace(/'/g, "&#39;");
}





function escapeUrl(aText) {
  return (aText || "").replace(/"/g, "%22");
}

function notifyObservers(aTopic, aInitialImport) {
  Services.obs.notifyObservers(null, aTopic, aInitialImport ? "html-initial"
                                                            : "html");
}

this.BookmarkHTMLUtils = Object.freeze({
  












  importFromURL: function BHU_importFromURL(aSpec, aInitialImport) {
    return Task.spawn(function* () {
      notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_BEGIN, aInitialImport);
      try {
        let importer = new BookmarkImporter(aInitialImport);
        yield importer.importFromURL(aSpec);

        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_SUCCESS, aInitialImport);
      } catch(ex) {
        Cu.reportError("Failed to import bookmarks from " + aSpec + ": " + ex);
        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED, aInitialImport);
        throw ex;
      }
    });
  },

  












  importFromFile: function BHU_importFromFile(aFilePath, aInitialImport) {
    if (aFilePath instanceof Ci.nsIFile) {
      Deprecated.warning("Passing an nsIFile to BookmarksJSONUtils.importFromFile " +
                         "is deprecated. Please use an OS.File path string instead.",
                         "https://developer.mozilla.org/docs/JavaScript_OS.File");
      aFilePath = aFilePath.path;
    }

    return Task.spawn(function* () {
      notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_BEGIN, aInitialImport);
      try {
        if (!(yield OS.File.exists(aFilePath))) {
          throw new Error("Cannot import from nonexisting html file: " + aFilePath);
        }
        let importer = new BookmarkImporter(aInitialImport);
        yield importer.importFromURL(OS.Path.toFileURI(aFilePath));

        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_SUCCESS, aInitialImport);
      } catch(ex) {
        Cu.reportError("Failed to import bookmarks from " + aFilePath + ": " + ex);
        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED, aInitialImport);
        throw ex;
      }
    });
  },

  










  exportToFile: function BHU_exportToFile(aFilePath) {
    if (aFilePath instanceof Ci.nsIFile) {
      Deprecated.warning("Passing an nsIFile to BookmarksHTMLUtils.exportToFile " +
                         "is deprecated. Please use an OS.File path string instead.",
                         "https://developer.mozilla.org/docs/JavaScript_OS.File");
      aFilePath = aFilePath.path;
    }
    return Task.spawn(function* () {
      let [bookmarks, count] = yield PlacesBackups.getBookmarksTree();
      let startTime = Date.now();

      
      let exporter = new BookmarkExporter(bookmarks);
      yield exporter.exportToFile(aFilePath);

      try {
        Services.telemetry
                .getHistogramById("PLACES_EXPORT_TOHTML_MS")
                .add(Date.now() - startTime);
      } catch (ex) {
        Components.utils.reportError("Unable to report telemetry.");
      }

      return count;
    });
  },

  get defaultPath() {
    try {
      return Services.prefs.getCharPref("browser.bookmarks.file");
    } catch (ex) {}
    return OS.Path.join(OS.Constants.Path.profileDir, "bookmarks.html")
  }
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
      let kwPromise = PlacesUtils.keywords.insert({ keyword,
                                                    url: frame.previousLink.spec,
                                                    postData });
      this._importPromises.push(kwPromise);
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
      let chPromise = PlacesUtils.setCharsetForURI(frame.previousLink, lastCharset);
      this._importPromises.push(chPromise);
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
        
        
        let lmPromise = PlacesUtils.livemarks.addLivemark({
          "title": frame.previousText,
          "parentId": frame.containerId,
          "index": PlacesUtils.bookmarks.DEFAULT_INDEX,
          "feedURI": frame.previousFeed,
          "siteURI": frame.previousLink,
        });
        this._importPromises.push(lmPromise);
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

  importFromURL: Task.async(function* (href) {
    this._importPromises = [];
    yield new Promise((resolve, reject) => {
      let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
      xhr.onload = () => {
        try {
          this._walkTreeForImport(xhr.responseXML);
          resolve();
        } catch(e) {
          reject(e);
        }
      };
      xhr.onabort = xhr.onerror = xhr.ontimeout = () => {
        reject(new Error("xmlhttprequest failed"));
      };
      xhr.open("GET", href);
      xhr.responseType = "document";
      xhr.overrideMimeType("text/html");
      xhr.send();
    });
    
    
    try {
      yield Promise.all(this._importPromises);
    } finally {
      delete this._importPromises;
    }
  }),
};

function BookmarkExporter(aBookmarksTree) {
  
  let rootsMap = new Map();
  for (let child of aBookmarksTree.children) {
    if (child.root)
      rootsMap.set(child.root, child);
  }

  
  
  this._root = rootsMap.get("bookmarksMenuFolder");

  for (let key of [ "toolbarFolder", "unfiledBookmarksFolder" ]) {
    let root = rootsMap.get(key);
    if (root.children && root.children.length > 0) {
      if (!this._root.children)
        this._root.children = [];
      this._root.children.push(root);
    }
  }
}

BookmarkExporter.prototype = {
  exportToFile: function exportToFile(aFilePath) {
    return Task.spawn(function* () {
      
      let out = FileUtils.openAtomicFileOutputStream(new FileUtils.File(aFilePath));
      try {
        
        let bufferedOut = Cc["@mozilla.org/network/buffered-output-stream;1"]
                          .createInstance(Ci.nsIBufferedOutputStream);
        bufferedOut.init(out, 4096);
        try {
          
          this._converterOut = Cc["@mozilla.org/intl/converter-output-stream;1"]
                               .createInstance(Ci.nsIConverterOutputStream);
          this._converterOut.init(bufferedOut, "utf-8", 0, 0);
          try {
            this._writeHeader();
            yield this._writeContainer(this._root);
            
            bufferedOut.QueryInterface(Ci.nsISafeOutputStream).finish();
          } finally {
            this._converterOut.close();
            this._converterOut = null;
          }
        } finally {
          bufferedOut.close();
        }
      } finally {
        out.close();
      }
    }.bind(this));
  },

  _converterOut: null,

  _write: function (aText) {
    this._converterOut.writeString(aText || "");
  },

  _writeAttribute: function (aName, aValue) {
    this._write(' ' +  aName + '="' + aValue + '"');
  },

  _writeLine: function (aText) {
    this._write(aText + "\n");
  },

  _writeHeader: function () {
    this._writeLine("<!DOCTYPE NETSCAPE-Bookmark-file-1>");
    this._writeLine("<!-- This is an automatically generated file.");
    this._writeLine("     It will be read and overwritten.");
    this._writeLine("     DO NOT EDIT! -->");
    this._writeLine('<META HTTP-EQUIV="Content-Type" CONTENT="text/html; ' +
                    'charset=UTF-8">');
    this._writeLine("<TITLE>Bookmarks</TITLE>");
  },

  _writeContainer: function (aItem, aIndent = "") {
    if (aItem == this._root) {
      this._writeLine("<H1>" + escapeHtmlEntities(this._root.title) + "</H1>");
      this._writeLine("");
    }
    else {
      this._write(aIndent + "<DT><H3");
      this._writeDateAttributes(aItem);

      if (aItem.root === "toolbarFolder")
        this._writeAttribute("PERSONAL_TOOLBAR_FOLDER", "true");
      else if (aItem.root === "unfiledBookmarksFolder")
        this._writeAttribute("UNFILED_BOOKMARKS_FOLDER", "true");
      this._writeLine(">" + escapeHtmlEntities(aItem.title) + "</H3>");
    }

    this._writeDescription(aItem, aIndent);

    this._writeLine(aIndent + "<DL><p>");
    if (aItem.children)
      yield this._writeContainerContents(aItem, aIndent);
    if (aItem == this._root)
      this._writeLine(aIndent + "</DL>");
    else
      this._writeLine(aIndent + "</DL><p>");
  },

  _writeContainerContents: function (aItem, aIndent) {
    let localIndent = aIndent + EXPORT_INDENT;

    for (let child of aItem.children) {
      if (child.annos && child.annos.some(anno => anno.name == PlacesUtils.LMANNO_FEEDURI))
          this._writeLivemark(child, localIndent);
      else if (child.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER)
          yield this._writeContainer(child, localIndent);
      else if (child.type == PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR)
        this._writeSeparator(child, localIndent);
      else
        yield this._writeItem(child, localIndent);
    }
  },

  _writeSeparator: function (aItem, aIndent) {
    this._write(aIndent + "<HR");
    
    if (aItem.title)
      this._writeAttribute("NAME", escapeHtmlEntities(aItem.title));
    this._write(">");
  },

  _writeLivemark: function (aItem, aIndent) {
    this._write(aIndent + "<DT><A");
    let feedSpec = aItem.annos.find(anno => anno.name == PlacesUtils.LMANNO_FEEDURI).value;
    this._writeAttribute("FEEDURL", escapeUrl(feedSpec));
    let siteSpecAnno = aItem.annos.find(anno => anno.name == PlacesUtils.LMANNO_SITEURI);
    if (siteSpecAnno)
      this._writeAttribute("HREF", escapeUrl(siteSpecAnno.value));
    this._writeLine(">" + escapeHtmlEntities(aItem.title) + "</A>");
    this._writeDescription(aItem, aIndent);
  },

  _writeItem: function (aItem, aIndent) {
    let uri = null;
    try {
      uri = NetUtil.newURI(aItem.uri);
    } catch (ex) {
      
      return;
    }

    this._write(aIndent + "<DT><A");
    this._writeAttribute("HREF", escapeUrl(aItem.uri));
    this._writeDateAttributes(aItem);
    yield this._writeFaviconAttribute(aItem);

    if (aItem.keyword) {
      this._writeAttribute("SHORTCUTURL", escapeHtmlEntities(aItem.keyword));
      if (aItem.postData)
        this._writeAttribute("POST_DATA", escapeHtmlEntities(aItem.postData));
    }

    if (aItem.annos && aItem.annos.some(anno => anno.name == LOAD_IN_SIDEBAR_ANNO))
      this._writeAttribute("WEB_PANEL", "true");
    if (aItem.charset)
      this._writeAttribute("LAST_CHARSET", escapeHtmlEntities(aItem.charset));
    if (aItem.tags)
      this._writeAttribute("TAGS", aItem.tags);
    this._writeLine(">" + escapeHtmlEntities(aItem.title) + "</A>");
    this._writeDescription(aItem, aIndent);
  },

  _writeDateAttributes: function (aItem) {
    if (aItem.dateAdded)
      this._writeAttribute("ADD_DATE",
                           Math.floor(aItem.dateAdded / MICROSEC_PER_SEC));
    if (aItem.lastModified)
      this._writeAttribute("LAST_MODIFIED",
                           Math.floor(aItem.lastModified / MICROSEC_PER_SEC));
  },

  _writeFaviconAttribute: function (aItem) {
    if (!aItem.iconuri)
      return;
    let favicon;
    try {
      favicon  = yield PlacesUtils.promiseFaviconData(aItem.uri);
    } catch (ex) {
      Components.utils.reportError("Unexpected Error trying to fetch icon data");
      return;
    }

    this._writeAttribute("ICON_URI", escapeUrl(favicon.uri.spec));

    if (!favicon.uri.schemeIs("chrome") && favicon.dataLen > 0) {
      let faviconContents = "data:image/png;base64," +
        base64EncodeString(String.fromCharCode.apply(String, favicon.data));
      this._writeAttribute("ICON", faviconContents);
    }
  },

  _writeDescription: function (aItem, aIndent) {
    let descriptionAnno = aItem.annos &&
                          aItem.annos.find(anno => anno.name == DESCRIPTION_ANNO);
    if (descriptionAnno)
      this._writeLine(aIndent + "<DD>" + escapeHtmlEntities(descriptionAnno.value));
  }
};
