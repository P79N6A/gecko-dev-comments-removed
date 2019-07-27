



const Cu = Components.utils;
Cu.import("resource://gre/modules/LoadContextInfo.jsm");
Cu.import("resource://gre/modules/Services.jsm");


function pageInfoTreeView(treeid, copycol)
{
  
  
  this.treeid = treeid;
  this.copycol = copycol;
  this.rows = 0;
  this.tree = null;
  this.data = [ ];
  this.selection = null;
  this.sortcol = -1;
  this.sortdir = false;
}

pageInfoTreeView.prototype = {
  set rowCount(c) { throw "rowCount is a readonly property"; },
  get rowCount() { return this.rows; },

  setTree: function(tree)
  {
    this.tree = tree;
  },

  getCellText: function(row, column)
  {
    
    
    
    
    return this.data[row][column.index] || "";
  },

  setCellValue: function(row, column, value)
  {
  },

  setCellText: function(row, column, value)
  {
    this.data[row][column.index] = value;
  },

  addRow: function(row)
  {
    this.rows = this.data.push(row);
    this.rowCountChanged(this.rows - 1, 1);
    if (this.selection.count == 0 && this.rowCount && !gImageElement)
      this.selection.select(0);
  },

  rowCountChanged: function(index, count)
  {
    this.tree.rowCountChanged(index, count);
  },

  invalidate: function()
  {
    this.tree.invalidate();
  },

  clear: function()
  {
    if (this.tree)
      this.tree.rowCountChanged(0, -this.rows);
    this.rows = 0;
    this.data = [ ];
  },

  handleCopy: function(row)
  {
    return (row < 0 || this.copycol < 0) ? "" : (this.data[row][this.copycol] || "");
  },

  performActionOnRow: function(action, row)
  {
    if (action == "copy") {
      var data = this.handleCopy(row)
      this.tree.treeBody.parentNode.setAttribute("copybuffer", data);
    }
  },

  onPageMediaSort : function(columnname)
  {
    var tree = document.getElementById(this.treeid);
    var treecol = tree.columns.getNamedColumn(columnname);

    this.sortdir =
      gTreeUtils.sort(
        tree,
        this,
        this.data,
        treecol.index,
        function textComparator(a, b) { return a.toLowerCase().localeCompare(b.toLowerCase()); },
        this.sortcol,
        this.sortdir
      );

    this.sortcol = treecol.index;
  },

  getRowProperties: function(row) { return ""; },
  getCellProperties: function(row, column) { return ""; },
  getColumnProperties: function(column) { return ""; },
  isContainer: function(index) { return false; },
  isContainerOpen: function(index) { return false; },
  isSeparator: function(index) { return false; },
  isSorted: function() { },
  canDrop: function(index, orientation) { return false; },
  drop: function(row, orientation) { return false; },
  getParentIndex: function(index) { return 0; },
  hasNextSibling: function(index, after) { return false; },
  getLevel: function(index) { return 0; },
  getImageSrc: function(row, column) { },
  getProgressMode: function(row, column) { },
  getCellValue: function(row, column) { },
  toggleOpenState: function(index) { },
  cycleHeader: function(col) { },
  selectionChanged: function() { },
  cycleCell: function(row, column) { },
  isEditable: function(row, column) { return false; },
  isSelectable: function(row, column) { return false; },
  performAction: function(action) { },
  performActionOnCell: function(action, row, column) { }
};


var gWindow = null;
var gDocument = null;
var gImageElement = null;


const COL_IMAGE_ADDRESS = 0;
const COL_IMAGE_TYPE    = 1;
const COL_IMAGE_SIZE    = 2;
const COL_IMAGE_ALT     = 3;
const COL_IMAGE_COUNT   = 4;
const COL_IMAGE_NODE    = 5;
const COL_IMAGE_BG      = 6;


const COPYCOL_NONE = -1;
const COPYCOL_META_CONTENT = 1;
const COPYCOL_IMAGE = COL_IMAGE_ADDRESS;


var gMetaView = new pageInfoTreeView('metatree', COPYCOL_META_CONTENT);
var gImageView = new pageInfoTreeView('imagetree', COPYCOL_IMAGE);

gImageView.getCellProperties = function(row, col) {
  var data = gImageView.data[row];
  var item = gImageView.data[row][COL_IMAGE_NODE];
  var props = "";
  if (!checkProtocol(data) ||
      item instanceof HTMLEmbedElement ||
      (item instanceof HTMLObjectElement && !item.type.startsWith("image/")))
    props += "broken";

  if (col.element.id == "image-address")
    props += " ltr";

  return props;
};

gImageView.getCellText = function(row, column) {
  var value = this.data[row][column.index];
  if (column.index == COL_IMAGE_SIZE) {
    if (value == -1) {
      return gStrings.unknown;
    } else {
      var kbSize = Number(Math.round(value / 1024 * 100) / 100);
      return gBundle.getFormattedString("mediaFileSize", [kbSize]);
    }
  }
  return value || "";
};

gImageView.onPageMediaSort = function(columnname) {
  var tree = document.getElementById(this.treeid);
  var treecol = tree.columns.getNamedColumn(columnname);

  var comparator;
  if (treecol.index == COL_IMAGE_SIZE || treecol.index == COL_IMAGE_COUNT) {
    comparator = function numComparator(a, b) { return a - b; };
  } else {
    comparator = function textComparator(a, b) { return a.toLowerCase().localeCompare(b.toLowerCase()); };
  }

  this.sortdir =
    gTreeUtils.sort(
      tree,
      this,
      this.data,
      treecol.index,
      comparator,
      this.sortcol,
      this.sortdir
    );

  this.sortcol = treecol.index;
};

var gImageHash = { };



var gStrings = { };
var gBundle;

const PERMISSION_CONTRACTID     = "@mozilla.org/permissionmanager;1";
const PREFERENCES_CONTRACTID    = "@mozilla.org/preferences-service;1";
const ATOM_CONTRACTID           = "@mozilla.org/atom-service;1";



const nsICacheStorageService = Components.interfaces.nsICacheStorageService;
const nsICacheStorage = Components.interfaces.nsICacheStorage;
const cacheService = Components.classes["@mozilla.org/netwerk/cache-storage-service;1"].getService(nsICacheStorageService);

var loadContextInfo = LoadContextInfo.fromLoadContext(
  window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIWebNavigation)
        .QueryInterface(Components.interfaces.nsILoadContext), false);
var diskStorage = cacheService.diskCacheStorage(loadContextInfo, false);

const nsICookiePermission  = Components.interfaces.nsICookiePermission;
const nsIPermissionManager = Components.interfaces.nsIPermissionManager;

const nsICertificateDialogs = Components.interfaces.nsICertificateDialogs;
const CERTIFICATEDIALOGS_CONTRACTID = "@mozilla.org/nsCertificateDialogs;1"


try {
  const gClipboardHelper = Components.classes["@mozilla.org/widget/clipboardhelper;1"].getService(Components.interfaces.nsIClipboardHelper);
}
catch(e) {
  
}


const nsIImageLoadingContent = Components.interfaces.nsIImageLoadingContent;


const XLinkNS  = "http://www.w3.org/1999/xlink";
const XULNS    = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const XMLNS    = "http://www.w3.org/XML/1998/namespace";
const XHTMLNS  = "http://www.w3.org/1999/xhtml";
const XHTML2NS = "http://www.w3.org/2002/06/xhtml2"

const XHTMLNSre  = "^http\:\/\/www\.w3\.org\/1999\/xhtml$";
const XHTML2NSre = "^http\:\/\/www\.w3\.org\/2002\/06\/xhtml2$";
const XHTMLre = RegExp(XHTMLNSre + "|" + XHTML2NSre, "");












var onLoadRegistry = [ ];





var onResetRegistry = [ ];



var onProcessFrame = [ ];



var onProcessElement = [ ];



var onFinished = [ ];


var onUnloadRegistry = [ ];


var onImagePreviewShown = [ ];







function onLoadPageInfo()
{
  gBundle = document.getElementById("pageinfobundle");
  gStrings.unknown = gBundle.getString("unknown");
  gStrings.notSet = gBundle.getString("notset");
  gStrings.mediaImg = gBundle.getString("mediaImg");
  gStrings.mediaBGImg = gBundle.getString("mediaBGImg");
  gStrings.mediaBorderImg = gBundle.getString("mediaBorderImg");
  gStrings.mediaListImg = gBundle.getString("mediaListImg");
  gStrings.mediaCursor = gBundle.getString("mediaCursor");
  gStrings.mediaObject = gBundle.getString("mediaObject");
  gStrings.mediaEmbed = gBundle.getString("mediaEmbed");
  gStrings.mediaLink = gBundle.getString("mediaLink");
  gStrings.mediaInput = gBundle.getString("mediaInput");
  gStrings.mediaVideo = gBundle.getString("mediaVideo");
  gStrings.mediaAudio = gBundle.getString("mediaAudio");

  var args = "arguments" in window &&
             window.arguments.length >= 1 &&
             window.arguments[0];

  if (!args || !args.doc) {
    gWindow = window.opener.gBrowser.selectedBrowser.contentWindowAsCPOW;
    gDocument = gWindow.document;
  }

  
  var imageTree = document.getElementById("imagetree");
  imageTree.view = gImageView;

  
  loadTab(args);
  Components.classes["@mozilla.org/observer-service;1"]
            .getService(Components.interfaces.nsIObserverService)
            .notifyObservers(window, "page-info-dialog-loaded", null);
}

function loadPageInfo()
{
  var titleFormat = gWindow != gWindow.top ? "pageInfo.frame.title"
                                           : "pageInfo.page.title";
  document.title = gBundle.getFormattedString(titleFormat, [gDocument.location]);

  document.getElementById("main-window").setAttribute("relatedUrl", gDocument.location);

  
  makeGeneralTab();

  
  makeTabs(gDocument, gWindow);

  initFeedTab();
  onLoadPermission();

  
  onLoadRegistry.forEach(function(func) { func(); });
}

function resetPageInfo(args)
{
  
  gMetaView.clear();

  
  var mediaTab = document.getElementById("mediaTab");
  if (!mediaTab.hidden) {
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .removeObserver(imagePermissionObserver, "perm-changed");
    mediaTab.hidden = true;
  }
  gImageView.clear();
  gImageHash = {};

  
  var feedListbox = document.getElementById("feedListbox");
  while (feedListbox.firstChild)
    feedListbox.removeChild(feedListbox.firstChild);

  
  onResetRegistry.forEach(function(func) { func(); });

  
  loadTab(args);
}

function onUnloadPageInfo()
{
  
  if (!document.getElementById("mediaTab").hidden) {
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .removeObserver(imagePermissionObserver, "perm-changed");
  }

  
  onUnloadRegistry.forEach(function(func) { func(); });
}

function doHelpButton()
{
  const helpTopics = {
    "generalPanel":  "pageinfo_general",
    "mediaPanel":    "pageinfo_media",
    "feedPanel":     "pageinfo_feed",
    "permPanel":     "pageinfo_permissions",
    "securityPanel": "pageinfo_security"
  };

  var deck  = document.getElementById("mainDeck");
  var helpdoc = helpTopics[deck.selectedPanel.id] || "pageinfo_general";
  openHelpLink(helpdoc);
}

function showTab(id)
{
  var deck  = document.getElementById("mainDeck");
  var pagel = document.getElementById(id + "Panel");
  deck.selectedPanel = pagel;
}

function loadTab(args)
{
  if (args && args.doc) {
    gDocument = args.doc;
    gWindow = gDocument.defaultView;
  }

  gImageElement = args && args.imageElement;

  
  loadPageInfo();

  var initialTab = (args && args.initialTab) || "generalTab";
  var radioGroup = document.getElementById("viewGroup");
  initialTab = document.getElementById(initialTab) || document.getElementById("generalTab");
  radioGroup.selectedItem = initialTab;
  radioGroup.selectedItem.doCommand();
  radioGroup.focus();
}

function toggleGroupbox(id)
{
  var elt = document.getElementById(id);
  if (elt.hasAttribute("closed")) {
    elt.removeAttribute("closed");
    if (elt.flexWhenOpened)
      elt.flex = elt.flexWhenOpened;
  }
  else {
    elt.setAttribute("closed", "true");
    if (elt.flex) {
      elt.flexWhenOpened = elt.flex;
      elt.flex = 0;
    }
  }
}

function openCacheEntry(key, cb)
{
  var checkCacheListener = {
    onCacheEntryCheck: function(entry, appCache) {
      return Components.interfaces.nsICacheEntryOpenCallback.ENTRY_WANTED;
    },
    onCacheEntryAvailable: function(entry, isNew, appCache, status) {
      cb(entry);
    }
  };
  diskStorage.asyncOpenURI(Services.io.newURI(key, null, null), "", nsICacheStorage.OPEN_READONLY, checkCacheListener);
}

function makeGeneralTab()
{
  var title = (gDocument.title) ? gBundle.getFormattedString("pageTitle", [gDocument.title]) : gBundle.getString("noPageTitle");
  document.getElementById("titletext").value = title;

  var url = gDocument.location.toString();
  setItemValue("urltext", url);

  var referrer = ("referrer" in gDocument && gDocument.referrer);
  setItemValue("refertext", referrer);

  var mode = ("compatMode" in gDocument && gDocument.compatMode == "BackCompat") ? "generalQuirksMode" : "generalStrictMode";
  document.getElementById("modetext").value = gBundle.getString(mode);

  
  var mimeType = gDocument.contentType;
  setItemValue("typetext", mimeType);

  
  var encoding = gDocument.characterSet;
  document.getElementById("encodingtext").value = encoding;

  
  var metaNodes = gDocument.getElementsByTagName("meta");
  var length = metaNodes.length;

  var metaGroup = document.getElementById("metaTags");
  if (!length)
    metaGroup.collapsed = true;
  else {
    var metaTagsCaption = document.getElementById("metaTagsCaption");
    if (length == 1)
      metaTagsCaption.label = gBundle.getString("generalMetaTag");
    else
      metaTagsCaption.label = gBundle.getFormattedString("generalMetaTags", [length]);
    var metaTree = document.getElementById("metatree");
    metaTree.treeBoxObject.view = gMetaView;

    for (var i = 0; i < length; i++)
      gMetaView.addRow([metaNodes[i].name || metaNodes[i].httpEquiv, metaNodes[i].content]);

    metaGroup.collapsed = false;
  }

  
  var modifiedText = formatDate(gDocument.lastModified, gStrings.notSet);
  document.getElementById("modifiedtext").value = modifiedText;

  
  var cacheKey = url.replace(/#.*$/, "");
  openCacheEntry(cacheKey, function(cacheEntry) {
    var sizeText;
    if (cacheEntry) {
      var pageSize = cacheEntry.dataSize;
      var kbSize = formatNumber(Math.round(pageSize / 1024 * 100) / 100);
      sizeText = gBundle.getFormattedString("generalSize", [kbSize, formatNumber(pageSize)]);
    }
    setItemValue("sizetext", sizeText);
  });

  securityOnLoad();
}







var gFrameList = [ ];

function makeTabs(aDocument, aWindow)
{
  goThroughFrames(aDocument, aWindow);
  processFrames();
}

function goThroughFrames(aDocument, aWindow)
{
  gFrameList.push(aDocument);
  if (aWindow && aWindow.frames.length > 0) {
    var num = aWindow.frames.length;
    for (var i = 0; i < num; i++)
      goThroughFrames(aWindow.frames[i].document, aWindow.frames[i]);  
  }
}

function processFrames()
{
  if (gFrameList.length) {
    var doc = gFrameList[0];
    onProcessFrame.forEach(function(func) { func(doc); });
    var iterator = doc.createTreeWalker(doc, NodeFilter.SHOW_ELEMENT, grabAll);
    gFrameList.shift();
    setTimeout(doGrab, 10, iterator);
    onFinished.push(selectImage);
  }
  else
    onFinished.forEach(function(func) { func(); });
}

function doGrab(iterator)
{
  for (var i = 0; i < 500; ++i)
    if (!iterator.nextNode()) {
      processFrames();
      return;
    }

  setTimeout(doGrab, 10, iterator);
}

function addImage(url, type, alt, elem, isBg)
{
  if (!url)
    return;

  if (!gImageHash.hasOwnProperty(url))
    gImageHash[url] = { };
  if (!gImageHash[url].hasOwnProperty(type))
    gImageHash[url][type] = { };
  if (!gImageHash[url][type].hasOwnProperty(alt)) {
    gImageHash[url][type][alt] = gImageView.data.length;
    var row = [url, type, -1, alt, 1, elem, isBg];
    gImageView.addRow(row);

    
    openCacheEntry(url, function(cacheEntry) {
      
      if (cacheEntry) {
        row[2] = cacheEntry.dataSize;
        
        gImageView.tree.invalidateRow(gImageView.data.indexOf(row));
      }
    });

    
    if (gImageView.data.length == 1) {
      document.getElementById("mediaTab").hidden = false;
      Components.classes["@mozilla.org/observer-service;1"]
                .getService(Components.interfaces.nsIObserverService)
                .addObserver(imagePermissionObserver, "perm-changed", false);
    }
  }
  else {
    var i = gImageHash[url][type][alt];
    gImageView.data[i][COL_IMAGE_COUNT]++;
    if (elem == gImageElement)
      gImageView.data[i][COL_IMAGE_NODE] = elem;
  }
}

function grabAll(elem)
{
  
  var computedStyle = elem.ownerDocument.defaultView.getComputedStyle(elem, "");

  if (computedStyle) {
    var addImgFunc = function (label, val) {
      if (val.primitiveType == CSSPrimitiveValue.CSS_URI) {
        addImage(val.getStringValue(), label, gStrings.notSet, elem, true);
      }
      else if (val.primitiveType == CSSPrimitiveValue.CSS_STRING) {
        
        
        var strVal = val.getStringValue();
        if (strVal.search(/^.*url\(\"?/) > -1) {
          url = strVal.replace(/^.*url\(\"?/,"").replace(/\"?\).*$/,"");
          addImage(url, label, gStrings.notSet, elem, true);
        }
      }
      else if (val.cssValueType == CSSValue.CSS_VALUE_LIST) {
        
        for (var i = 0; i < val.length; i++)
          addImgFunc(label, val.item(i));
      }
    };

    addImgFunc(gStrings.mediaBGImg, computedStyle.getPropertyCSSValue("background-image"));
    addImgFunc(gStrings.mediaBorderImg, computedStyle.getPropertyCSSValue("border-image-source"));
    addImgFunc(gStrings.mediaListImg, computedStyle.getPropertyCSSValue("list-style-image"));
    addImgFunc(gStrings.mediaCursor, computedStyle.getPropertyCSSValue("cursor"));
  }

  
  if (elem instanceof HTMLImageElement)
    addImage(elem.src, gStrings.mediaImg,
             (elem.hasAttribute("alt")) ? elem.alt : gStrings.notSet, elem, false);
  else if (elem instanceof SVGImageElement) {
    try {
      
      
      var href = makeURLAbsolute(elem.baseURI, elem.href.baseVal);
      addImage(href, gStrings.mediaImg, "", elem, false);
    } catch (e) { }
  }
  else if (elem instanceof HTMLVideoElement) {
    addImage(elem.currentSrc, gStrings.mediaVideo, "", elem, false);
  }
  else if (elem instanceof HTMLAudioElement) {
    addImage(elem.currentSrc, gStrings.mediaAudio, "", elem, false);
  }
  else if (elem instanceof HTMLLinkElement) {
    if (elem.rel && /\bicon\b/i.test(elem.rel))
      addImage(elem.href, gStrings.mediaLink, "", elem, false);
  }
  else if (elem instanceof HTMLInputElement || elem instanceof HTMLButtonElement) {
    if (elem.type.toLowerCase() == "image")
      addImage(elem.src, gStrings.mediaInput,
               (elem.hasAttribute("alt")) ? elem.alt : gStrings.notSet, elem, false);
  }
  else if (elem instanceof HTMLObjectElement)
    addImage(elem.data, gStrings.mediaObject, getValueText(elem), elem, false);
  else if (elem instanceof HTMLEmbedElement)
    addImage(elem.src, gStrings.mediaEmbed, "", elem, false);

  onProcessElement.forEach(function(func) { func(elem); });

  return NodeFilter.FILTER_ACCEPT;
}


function openURL(target)
{
  var url = target.parentNode.childNodes[2].value;
  window.open(url, "_blank", "chrome");
}

function onBeginLinkDrag(event,urlField,descField)
{
  if (event.originalTarget.localName != "treechildren")
    return;

  var tree = event.target;
  if (!("treeBoxObject" in tree))
    tree = tree.parentNode;

  var row = tree.treeBoxObject.getRowAt(event.clientX, event.clientY);
  if (row == -1)
    return;

  
  var col = tree.columns[urlField];
  var url = tree.view.getCellText(row, col);
  col = tree.columns[descField];
  var desc = tree.view.getCellText(row, col);

  var dt = event.dataTransfer;
  dt.setData("text/x-moz-url", url + "\n" + desc);
  dt.setData("text/url-list", url);
  dt.setData("text/plain", url);
}


function getSelectedRows(tree)
{
  var start = { };
  var end   = { };
  var numRanges = tree.view.selection.getRangeCount();

  var rowArray = [ ];
  for (var t = 0; t < numRanges; t++) {
    tree.view.selection.getRangeAt(t, start, end);
    for (var v = start.value; v <= end.value; v++)
      rowArray.push(v);
  }

  return rowArray;
}

function getSelectedRow(tree)
{
  var rows = getSelectedRows(tree);
  return (rows.length == 1) ? rows[0] : -1;
}

function selectSaveFolder(aCallback)
{
  const nsILocalFile = Components.interfaces.nsILocalFile;
  const nsIFilePicker = Components.interfaces.nsIFilePicker;
  let titleText = gBundle.getString("mediaSelectFolder");
  let fp = Components.classes["@mozilla.org/filepicker;1"].
           createInstance(nsIFilePicker);
  let fpCallback = function fpCallback_done(aResult) {
    if (aResult == nsIFilePicker.returnOK) {
      aCallback(fp.file.QueryInterface(nsILocalFile));
    } else {
      aCallback(null);
    }
  };

  fp.init(window, titleText, nsIFilePicker.modeGetFolder);
  fp.appendFilters(nsIFilePicker.filterAll);
  try {
    let prefs = Components.classes[PREFERENCES_CONTRACTID].
                getService(Components.interfaces.nsIPrefBranch);
    let initialDir = prefs.getComplexValue("browser.download.dir", nsILocalFile);
    if (initialDir) {
      fp.displayDirectory = initialDir;
    }
  } catch (ex) {
  }
  fp.open(fpCallback);
}

function saveMedia()
{
  var tree = document.getElementById("imagetree");
  var rowArray = getSelectedRows(tree);
  if (rowArray.length == 1) {
    var row = rowArray[0];
    var item = gImageView.data[row][COL_IMAGE_NODE];
    var url = gImageView.data[row][COL_IMAGE_ADDRESS];

    if (url) {
      var titleKey = "SaveImageTitle";

      if (item instanceof HTMLVideoElement)
        titleKey = "SaveVideoTitle";
      else if (item instanceof HTMLAudioElement)
        titleKey = "SaveAudioTitle";

      saveURL(url, null, titleKey, false, false, makeURI(item.baseURI), gDocument);
    }
  } else {
    selectSaveFolder(function(aDirectory) {
      if (aDirectory) {
        var saveAnImage = function(aURIString, aChosenData, aBaseURI) {
          internalSave(aURIString, null, null, null, null, false, "SaveImageTitle",
                       aChosenData, aBaseURI, gDocument);
        };

        for (var i = 0; i < rowArray.length; i++) {
          var v = rowArray[i];
          var dir = aDirectory.clone();
          var item = gImageView.data[v][COL_IMAGE_NODE];
          var uriString = gImageView.data[v][COL_IMAGE_ADDRESS];
          var uri = makeURI(uriString);

          try {
            uri.QueryInterface(Components.interfaces.nsIURL);
            dir.append(decodeURIComponent(uri.fileName));
          } catch(ex) {
            
          }

          if (i == 0) {
            saveAnImage(uriString, new AutoChosen(dir, uri), makeURI(item.baseURI));
          } else {
            
            
            setTimeout(saveAnImage, 200, uriString, new AutoChosen(dir, uri),
                       makeURI(item.baseURI));
          }
        }
      }
    });
  }
}

function onBlockImage()
{
  var permissionManager = Components.classes[PERMISSION_CONTRACTID]
                                    .getService(nsIPermissionManager);

  var checkbox = document.getElementById("blockImage");
  var uri = makeURI(document.getElementById("imageurltext").value);
  if (checkbox.checked)
    permissionManager.add(uri, "image", nsIPermissionManager.DENY_ACTION);
  else
    permissionManager.remove(uri.host, "image");
}

function onImageSelect()
{
  var previewBox   = document.getElementById("mediaPreviewBox");
  var mediaSaveBox = document.getElementById("mediaSaveBox");
  var splitter     = document.getElementById("mediaSplitter");
  var tree = document.getElementById("imagetree");
  var count = tree.view.selection.count;
  if (count == 0) {
    previewBox.collapsed   = true;
    mediaSaveBox.collapsed = true;
    splitter.collapsed     = true;
    tree.flex = 1;
  }
  else if (count > 1) {
    splitter.collapsed     = true;
    previewBox.collapsed   = true;
    mediaSaveBox.collapsed = false;
    tree.flex = 1;
  }
  else {
    mediaSaveBox.collapsed = true;
    splitter.collapsed     = false;
    previewBox.collapsed   = false;
    tree.flex = 0;
    makePreview(getSelectedRows(tree)[0]);
  }
}

function makePreview(row)
{
  var imageTree = document.getElementById("imagetree");
  var item = gImageView.data[row][COL_IMAGE_NODE];
  var url = gImageView.data[row][COL_IMAGE_ADDRESS];
  var isBG = gImageView.data[row][COL_IMAGE_BG];
  var isAudio = false;

  setItemValue("imageurltext", url);

  var imageText;
  if (!isBG &&
      !(item instanceof SVGImageElement) &&
      !(gDocument instanceof ImageDocument)) {
    imageText = item.title || item.alt;

    if (!imageText && !(item instanceof HTMLImageElement))
      imageText = getValueText(item);
  }
  setItemValue("imagetext", imageText);

  setItemValue("imagelongdesctext", item.longDesc);

  
  var cacheKey = url.replace(/#.*$/, "");
  openCacheEntry(cacheKey, function(cacheEntry) {
    
    var sizeText;
    if (cacheEntry) {
      var imageSize = cacheEntry.dataSize;
      var kbSize = Math.round(imageSize / 1024 * 100) / 100;
      sizeText = gBundle.getFormattedString("generalSize",
                                            [formatNumber(kbSize), formatNumber(imageSize)]);
    }
    else
      sizeText = gBundle.getString("mediaUnknownNotCached");
    setItemValue("imagesizetext", sizeText);

    var mimeType;
    var numFrames = 1;
    if (item instanceof HTMLObjectElement ||
        item instanceof HTMLEmbedElement ||
        item instanceof HTMLLinkElement)
      mimeType = item.type;

    if (!mimeType && !isBG && item instanceof nsIImageLoadingContent) {
      var imageRequest = item.getRequest(nsIImageLoadingContent.CURRENT_REQUEST);
      if (imageRequest) {
        mimeType = imageRequest.mimeType;
        var image = imageRequest.image;
        if (image)
          numFrames = image.numFrames;
      }
    }

    if (!mimeType)
      mimeType = getContentTypeFromHeaders(cacheEntry);

    
    if (!mimeType && url.startsWith("data:")) {
      let dataMimeType = /^data:(image\/[^;,]+)/i.exec(url);
      if (dataMimeType)
        mimeType = dataMimeType[1].toLowerCase();
    }

    var imageType;
    if (mimeType) {
      
      let imageMimeType = /^image\/(.*)/i.exec(mimeType);
      if (imageMimeType) {
        imageType = imageMimeType[1].toUpperCase();
        if (numFrames > 1)
          imageType = gBundle.getFormattedString("mediaAnimatedImageType",
                                                 [imageType, numFrames]);
        else
          imageType = gBundle.getFormattedString("mediaImageType", [imageType]);
      }
      else {
        
        imageType = mimeType;
      }
    }
    else {
      
      imageType = gImageView.data[row][COL_IMAGE_TYPE];
    }
    setItemValue("imagetypetext", imageType);

    var imageContainer = document.getElementById("theimagecontainer");
    var oldImage = document.getElementById("thepreviewimage");

    var isProtocolAllowed = checkProtocol(gImageView.data[row]);

    var newImage = new Image;
    newImage.id = "thepreviewimage";
    var physWidth = 0, physHeight = 0;
    var width = 0, height = 0;

    if ((item instanceof HTMLLinkElement || item instanceof HTMLInputElement ||
         item instanceof HTMLImageElement ||
         item instanceof SVGImageElement ||
         (item instanceof HTMLObjectElement && mimeType && mimeType.startsWith("image/")) || isBG) && isProtocolAllowed) {
      newImage.setAttribute("src", url);
      physWidth = newImage.width || 0;
      physHeight = newImage.height || 0;

      
      
      
      if (!isBG) {
        newImage.width = ("width" in item && item.width) || newImage.naturalWidth;
        newImage.height = ("height" in item && item.height) || newImage.naturalHeight;
      }
      else {
        
        
        newImage.width = newImage.naturalWidth;
        newImage.height = newImage.naturalHeight;
      }

      if (item instanceof SVGImageElement) {
        newImage.width = item.width.baseVal.value;
        newImage.height = item.height.baseVal.value;
      }

      width = newImage.width;
      height = newImage.height;

      document.getElementById("theimagecontainer").collapsed = false
      document.getElementById("brokenimagecontainer").collapsed = true;
    }
    else if (item instanceof HTMLVideoElement && isProtocolAllowed) {
      newImage = document.createElementNS("http://www.w3.org/1999/xhtml", "video");
      newImage.id = "thepreviewimage";
      newImage.src = url;
      newImage.controls = true;
      width = physWidth = item.videoWidth;
      height = physHeight = item.videoHeight;

      document.getElementById("theimagecontainer").collapsed = false;
      document.getElementById("brokenimagecontainer").collapsed = true;
    }
    else if (item instanceof HTMLAudioElement && isProtocolAllowed) {
      newImage = new Audio;
      newImage.id = "thepreviewimage";
      newImage.src = url;
      newImage.controls = true;
      isAudio = true;

      document.getElementById("theimagecontainer").collapsed = false;
      document.getElementById("brokenimagecontainer").collapsed = true;
    }
    else {
      
      
      document.getElementById("brokenimagecontainer").collapsed = false;
      document.getElementById("theimagecontainer").collapsed = true;
    }

    var imageSize = "";
    if (url && !isAudio) {
      if (width != physWidth || height != physHeight) {
        imageSize = gBundle.getFormattedString("mediaDimensionsScaled",
                                               [formatNumber(physWidth),
                                                formatNumber(physHeight),
                                                formatNumber(width),
                                                formatNumber(height)]);
      }
      else {
        imageSize = gBundle.getFormattedString("mediaDimensions",
                                               [formatNumber(width),
                                                formatNumber(height)]);
      }
    }
    setItemValue("imagedimensiontext", imageSize);

    makeBlockImage(url);

    imageContainer.removeChild(oldImage);
    imageContainer.appendChild(newImage);

    onImagePreviewShown.forEach(function(func) { func(); });
  });
}

function makeBlockImage(url)
{
  var permissionManager = Components.classes[PERMISSION_CONTRACTID]
                                    .getService(nsIPermissionManager);
  var prefs = Components.classes[PREFERENCES_CONTRACTID]
                        .getService(Components.interfaces.nsIPrefBranch);

  var checkbox = document.getElementById("blockImage");
  var imagePref = prefs.getIntPref("permissions.default.image");
  if (!(/^https?:/.test(url)) || imagePref == 2)
    
    
    checkbox.hidden = true;
  else {
    var uri = makeURI(url);
    if (uri.host) {
      checkbox.hidden = false;
      checkbox.label = gBundle.getFormattedString("mediaBlockImage", [uri.host]);
      var perm = permissionManager.testPermission(uri, "image");
      checkbox.checked = perm == nsIPermissionManager.DENY_ACTION;
    }
    else
      checkbox.hidden = true;
  }
}

var imagePermissionObserver = {
  observe: function (aSubject, aTopic, aData)
  {
    if (document.getElementById("mediaPreviewBox").collapsed)
      return;

    if (aTopic == "perm-changed") {
      var permission = aSubject.QueryInterface(Components.interfaces.nsIPermission);
      if (permission.type == "image") {
        var imageTree = document.getElementById("imagetree");
        var row = getSelectedRow(imageTree);
        var item = gImageView.data[row][COL_IMAGE_NODE];
        var url = gImageView.data[row][COL_IMAGE_ADDRESS];
        if (makeURI(url).host == permission.host)
          makeBlockImage(url);
      }
    }
  }
}

function getContentTypeFromHeaders(cacheEntryDescriptor)
{
  if (!cacheEntryDescriptor)
    return null;

  return (/^Content-Type:\s*(.*?)\s*(?:\;|$)/mi
          .exec(cacheEntryDescriptor.getMetaDataElement("response-head")))[1];
}




function getValueText(node)
{
  var valueText = "";

  
  if (node instanceof HTMLInputElement ||
      node instanceof HTMLSelectElement ||
      node instanceof HTMLTextAreaElement)
    return valueText;

  
  var length = node.childNodes.length;
  for (var i = 0; i < length; i++) {
    var childNode = node.childNodes[i];
    var nodeType = childNode.nodeType;

    
    if (nodeType == Node.TEXT_NODE)
      valueText += " " + childNode.nodeValue;
    
    else if (nodeType == Node.ELEMENT_NODE) {
      
      if (childNode instanceof HTMLImageElement)
        valueText += " " + getAltText(childNode);
      else
        valueText += " " + getValueText(childNode);
    }
  }

  return stripWS(valueText);
}



function getAltText(node)
{
  var altText = "";

  if (node.alt)
    return node.alt;
  var length = node.childNodes.length;
  for (var i = 0; i < length; i++)
    if ((altText = getAltText(node.childNodes[i]) != undefined))  
      return altText;
  return "";
}



function stripWS(text)
{
  var middleRE = /\s+/g;
  var endRE = /(^\s+)|(\s+$)/g;

  text = text.replace(middleRE, " ");
  return text.replace(endRE, "");
}

function setItemValue(id, value)
{
  var item = document.getElementById(id);
  if (value) {
    item.parentNode.collapsed = false;
    item.value = value;
  }
  else
    item.parentNode.collapsed = true;
}

function formatNumber(number)
{
  return (+number).toLocaleString();  
}

function formatDate(datestr, unknown)
{
  
  var dateService = Components.classes["@mozilla.org/intl/scriptabledateformat;1"]
                              .getService(Components.interfaces.nsIScriptableDateFormat);

  var date = new Date(datestr);
  if (!date.valueOf())
    return unknown;

  return dateService.FormatDateTime("", dateService.dateFormatLong,
                                    dateService.timeFormatSeconds,
                                    date.getFullYear(), date.getMonth()+1, date.getDate(),
                                    date.getHours(), date.getMinutes(), date.getSeconds());
}

function doCopy()
{
  if (!gClipboardHelper)
    return;

  var elem = document.commandDispatcher.focusedElement;

  if (elem && "treeBoxObject" in elem) {
    var view = elem.view;
    var selection = view.selection;
    var text = [], tmp = '';
    var min = {}, max = {};

    var count = selection.getRangeCount();

    for (var i = 0; i < count; i++) {
      selection.getRangeAt(i, min, max);

      for (var row = min.value; row <= max.value; row++) {
        view.performActionOnRow("copy", row);

        tmp = elem.getAttribute("copybuffer");
        if (tmp)
          text.push(tmp);
        elem.removeAttribute("copybuffer");
      }
    }
    gClipboardHelper.copyString(text.join("\n"), document);
  }
}

function doSelectAll()
{
  var elem = document.commandDispatcher.focusedElement;

  if (elem && "treeBoxObject" in elem)
    elem.view.selection.selectAll();
}

function selectImage()
{
  if (!gImageElement)
    return;

  var tree = document.getElementById("imagetree");
  for (var i = 0; i < tree.view.rowCount; i++) {
    if (gImageElement == gImageView.data[i][COL_IMAGE_NODE] &&
        !gImageView.data[i][COL_IMAGE_BG]) {
      tree.view.selection.select(i);
      tree.treeBoxObject.ensureRowIsVisible(i);
      tree.focus();
      return;
    }
  }
}

function checkProtocol(img)
{
  var url = img[COL_IMAGE_ADDRESS];
  return /^data:image\//i.test(url) ||
    /^(https?|ftp|file|about|chrome|resource):/.test(url);
}
