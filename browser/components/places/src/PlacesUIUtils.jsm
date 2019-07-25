









































var EXPORTED_SYMBOLS = ["PlacesUIUtils"];

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;
var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "PlacesUtils", function() {
  Cu.import("resource://gre/modules/PlacesUtils.jsm");
  return PlacesUtils;
});

var PlacesUIUtils = {
  ORGANIZER_LEFTPANE_VERSION: 6,
  ORGANIZER_FOLDER_ANNO: "PlacesOrganizer/OrganizerFolder",
  ORGANIZER_QUERY_ANNO: "PlacesOrganizer/OrganizerQuery",

  LOAD_IN_SIDEBAR_ANNO: "bookmarkProperties/loadInSidebar",
  DESCRIPTION_ANNO: "bookmarkProperties/description",

  TYPE_TAB_DROP: "application/x-moz-tabbrowser-tab",

  





  createFixedURI: function PUIU_createFixedURI(aSpec) {
    return URIFixup.createFixupURI(aSpec, Ci.nsIURIFixup.FIXUP_FLAG_NONE);
  },

  





  _wrapString: function PUIU__wrapString(aString) {
    var s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  getFormattedString: function PUIU_getFormattedString(key, params) {
    return bundle.formatStringFromName(key, params, params.length);
  },

  getString: function PUIU_getString(key) {
    return bundle.GetStringFromName(key);
  },

  get _copyableAnnotations() [
    this.DESCRIPTION_ANNO,
    this.LOAD_IN_SIDEBAR_ANNO,
    PlacesUtils.POST_DATA_ANNO,
    PlacesUtils.READ_ONLY_ANNO,
  ],

  















  _getURIItemCopyTransaction:
  function PUIU__getURIItemCopyTransaction(aData, aContainer, aIndex)
  {
    let transactions = [];
    if (aData.dateAdded) {
      transactions.push(
        new PlacesEditItemDateAddedTransaction(null, aData.dateAdded)
      );
    }
    if (aData.lastModified) {
      transactions.push(
        new PlacesEditItemLastModifiedTransaction(null, aData.lastModified)
      );
    }

    let keyword = aData.keyword || null;
    let annos = [];
    if (aData.annos) {
      annos = aData.annos.filter(function (aAnno) {
        return this._copyableAnnotations.indexOf(aAnno.name) != -1;
      }, this);
    }

    return new PlacesCreateBookmarkTransaction(PlacesUtils._uri(aData.uri),
                                               aContainer, aIndex, aData.title,
                                               keyword, annos, transactions);
  },

  















  _getFolderCopyTransaction:
  function PUIU__getFolderCopyTransaction(aData, aContainer, aIndex)
  {
    function getChildItemsTransactions(aChildren)
    {
      let transactions = [];
      let index = aIndex;
      aChildren.forEach(function (node, i) {
        
        
        
        if (aIndex != PlacesUtils.bookmarks.DEFAULT_INDEX) {
          index = i;
        }

        if (node.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER) {
          if (node.livemark && node.annos) {
            transactions.push(
              PlacesUIUtils._getLivemarkCopyTransaction(node, aContainer, index)
            );
          }
          else {
            transactions.push(
              PlacesUIUtils._getFolderCopyTransaction(node, aContainer, index)
            );
          }
        }
        else if (node.type == PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR) {
          transactions.push(new PlacesCreateSeparatorTransaction(-1, index));
        }
        else if (node.type == PlacesUtils.TYPE_X_MOZ_PLACE) {
          transactions.push(
            PlacesUIUtils._getURIItemCopyTransaction(node, -1, index)
          );
        }
        else {
          throw new Error("Unexpected item under a bookmarks folder");
        }
      });
      return transactions;
    }

    if (aContainer == PlacesUtils.tagsFolderId) { 
      let transactions = [];
      if (aData.children) {
        aData.children.forEach(function(aChild) {
          transactions.push(
            new PlacesTagURITransaction(PlacesUtils._uri(aChild.uri),
                                        [aData.title])
          );
        });
      }
      return new PlacesAggregatedTransaction("addTags", transactions);
    }

    if (aData.livemark && aData.annos) { 
      return this._getLivemarkCopyTransaction(aData, aContainer, aIndex);
    }

    let transactions = getChildItemsTransactions(aData.children);
    if (aData.dateAdded) {
      transactions.push(
        new PlacesEditItemDateAddedTransaction(null, aData.dateAdded)
      );
    }
    if (aData.lastModified) {
      transactions.push(
        new PlacesEditItemLastModifiedTransaction(null, aData.lastModified)
      );
    }

    let annos = [];
    if (aData.annos) {
      annos = aData.annos.filter(function (aAnno) {
        return this._copyableAnnotations.indexOf(aAnno.name) != -1;
      }, this);
    }

    return new PlacesCreateFolderTransaction(aData.title, aContainer, aIndex,
                                             annos, transactions);
  },

  















  _getLivemarkCopyTransaction:
  function PUIU__getLivemarkCopyTransaction(aData, aContainer, aIndex)
  {
    if (!aData.livemark || !aData.annos) {
      throw new Error("node is not a livemark");
    }

    let feedURI, siteURI;
    let annos = [];
    if (aData.annos) {
      annos = aData.annos.filter(function (aAnno) {
        if (aAnno.name == PlacesUtils.LMANNO_FEEDURI) {
          feedURI = PlacesUtils._uri(aAnno.value);
        }
        else if (aAnno.name == PlacesUtils.LMANNO_SITEURI) {
          siteURI = PlacesUtils._uri(aAnno.value);
        }
        return this._copyableAnnotations.indexOf(aAnno.name) != -1
      }, this);
    }

    return new PlacesCreateLivemarkTransaction(feedURI, siteURI, aData.title,
                                               aContainer, aIndex, annos);
  },

  















  makeTransaction:
  function PUIU_makeTransaction(data, type, container, index, copy)
  {
    switch (data.type) {
      case PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER:
        if (copy) {
          return this._getFolderCopyTransaction(data, container, index);
        }

        
        return new PlacesMoveItemTransaction(data.id, container, index);
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE:
        if (copy || data.id == -1) { 
          return this._getURIItemCopyTransaction(data, container, index);
        }

        
        return new PlacesMoveItemTransaction(data.id, container, index);
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR:
        if (copy) {
          
          
          return new PlacesCreateSeparatorTransaction(container, index);
        }

        
        return new PlacesMoveItemTransaction(data.id, container, index);
        break;
      default:
        if (type == PlacesUtils.TYPE_X_MOZ_URL ||
            type == PlacesUtils.TYPE_UNICODE ||
            type == this.TYPE_TAB_DROP) {
          let title = type != PlacesUtils.TYPE_UNICODE ? data.title
                                                       : data.uri;
          return new PlacesCreateBookmarkTransaction(PlacesUtils._uri(data.uri),
                                                     container, index, title);
        }
    }
    return null;
  },

  _reportDeprecatedAddBookmarkMethod:
  function PUIU__reportDeprecatedAddBookmarkMethod() {
    
    let oldFuncName = arguments.callee.caller.name.slice(5);
    Cu.reportError(oldFuncName + " is deprecated and will be removed in a " +
                   "future release. Use showBookmarkDialog instead.");
  },

  


  showAddBookmarkUI: function PUIU_showAddBookmarkUI(
    aURI, aTitle, aDescription, aDefaultInsertionPoint, aShowPicker,
    aLoadInSidebar, aKeyword, aPostData, aCharSet) {
    this._reportDeprecatedAddBookmarkMethod();

    var info = {
      action: "add",
      type: "bookmark"
    };

    if (aURI)
      info.uri = aURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows = ["folderPicker"];
    }

    if (aLoadInSidebar)
      info.loadBookmarkInSidebar = true;

    if (typeof(aKeyword) == "string") {
      info.keyword = aKeyword;
      if (typeof(aPostData) == "string")
        info.postData = aPostData;
      if (typeof(aCharSet) == "string")
        info.charSet = aCharSet;
    }

    return this.showBookmarkDialog(info);
  },

  


  showMinimalAddBookmarkUI:
  function PUIU_showMinimalAddBookmarkUI(
    aURI, aTitle, aDescription, aDefaultInsertionPoint, aShowPicker,
    aLoadInSidebar, aKeyword, aPostData, aCharSet) {
    this._reportDeprecatedAddBookmarkMethod();

    var info = {
      action: "add",
      type: "bookmark",
      hiddenRows: ["description"]
    };
    if (aURI)
      info.uri = aURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows.push("folderPicker");
    }

    if (aLoadInSidebar)
      info.loadBookmarkInSidebar = true;
    else
      info.hiddenRows = info.hiddenRows.concat(["location", "loadInSidebar"]);

    if (typeof(aKeyword) == "string") {
      info.keyword = aKeyword;
      
      info.hiddenRows.push("tags");
      
      if (typeof(aPostData) == "string")
        info.postData = aPostData;
      if (typeof(aCharSet) == "string")
        info.charSet = aCharSet;
    }
    else
      info.hiddenRows.push("keyword");

    return this.showBookmarkDialog(info, undefined, true);
  },

  


  showAddLivemarkUI: function PUIU_showAddLivemarkURI(aFeedURI,
                                                      aSiteURI,
                                                      aTitle,
                                                      aDescription,
                                                      aDefaultInsertionPoint,
                                                      aShowPicker) {
    this._reportDeprecatedAddBookmarkMethod();

    var info = {
      action: "add",
      type: "livemark"
    };

    if (aFeedURI)
      info.feedURI = aFeedURI;
    if (aSiteURI)
      info.siteURI = aSiteURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows = ["folderPicker"];
    }
    return this.showBookmarkDialog(info);
  },

  


  showMinimalAddLivemarkUI:
  function PUIU_showMinimalAddLivemarkURI(
    aFeedURI, aSiteURI, aTitle, aDescription, aDefaultInsertionPoint,
    aShowPicker) {

    this._reportDeprecatedAddBookmarkMethod();

    var info = {
      action: "add",
      type: "livemark",
      hiddenRows: ["feedLocation", "siteLocation", "description"]
    };

    if (aFeedURI)
      info.feedURI = aFeedURI;
    if (aSiteURI)
      info.siteURI = aSiteURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows.push("folderPicker");
    }
    return this.showBookmarkDialog(info, undefined, true);
  },

  


  showMinimalAddMultiBookmarkUI: function PUIU_showAddMultiBookmarkUI(aURIList) {
    this._reportDeprecatedAddBookmarkMethod();

    if (aURIList.length == 0)
      throw("showAddMultiBookmarkUI expects a list of nsIURI objects");
    var info = {
      action: "add",
      type: "folder",
      hiddenRows: ["description"],
      URIList: aURIList
    };
    return this.showBookmarkDialog(info, undefined, true);
  },

  


  showItemProperties: function PUIU_showItemProperties(aItemId, aType, aReadOnly) {
    this._reportDeprecatedAddBookmarkMethod();

    var info = {
      action: "edit",
      type: aType,
      itemId: aItemId,
      readOnly: aReadOnly
    };
    return this.showBookmarkDialog(info);
  },

  


  showAddFolderUI:
  function PUIU_showAddFolderUI(aTitle, aDefaultInsertionPoint, aShowPicker) {
    this._reportDeprecatedAddBookmarkMethod();

    var info = {
      action: "add",
      type: "folder",
      hiddenRows: []
    };

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows.push("folderPicker");
    }
    return this.showBookmarkDialog(info);
  },


  














  showBookmarkDialog:
  function PUIU_showBookmarkDialog(aInfo, aParentWindow, aMinimalUI) {
    if (!aParentWindow) {
      aParentWindow = this._getWindow(null);
    }

    
    
    let minimalUI = "hiddenRows" in aInfo &&
                    aInfo.hiddenRows.indexOf("folderPicker") != -1;
    let dialogURL = aMinimalUI ?
                    "chrome://browser/content/places/bookmarkProperties2.xul" :
                    "chrome://browser/content/places/bookmarkProperties.xul";

    let features =
      "centerscreen,chrome,modal,resizable=" + (aMinimalUI ? "yes" : "no");

    aParentWindow.openDialog(dialogURL, "",  features, aInfo);
    return ("performed" in aInfo && aInfo.performed);
  },

  _getTopBrowserWin: function PUIU__getTopBrowserWin() {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  





  getViewForNode: function PUIU_getViewForNode(aNode) {
    let node = aNode;

    
    
    if (node.localName == "menu" && !node._placesNode &&
        node.lastChild._placesView)
      return node.lastChild._placesView;

    while (node instanceof Ci.nsIDOMElement) {
      if (node._placesView)
        return node._placesView;
      if (node.localName == "tree" && node.getAttribute("type") == "places")
        return node;

      node = node.parentNode;
    }

    return null;
  },

  







  markPageAsTyped: function PUIU_markPageAsTyped(aURL) {
    PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory)
               .markPageAsTyped(this.createFixedURI(aURL));
  },

  






  markPageAsFollowedBookmark: function PUIU_markPageAsFollowedBookmark(aURL) {
    PlacesUtils.history.markPageAsFollowedBookmark(this.createFixedURI(aURL));
  },

  





  markPageAsFollowedLink: function PUIU_markPageAsFollowedLink(aURL) {
    PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory)
               .markPageAsFollowedLink(this.createFixedURI(aURL));
  },

  









  checkURLSecurity: function PUIU_checkURLSecurity(aURINode, aWindow) {
    if (PlacesUtils.nodeIsBookmark(aURINode))
      return true;

    var uri = PlacesUtils._uri(aURINode.uri);
    if (uri.schemeIs("javascript") || uri.schemeIs("data")) {
      const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
      var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                           getService(Ci.nsIStringBundleService).
                           createBundle(BRANDING_BUNDLE_URI).
                           GetStringFromName("brandShortName");

      var errorStr = this.getString("load-js-data-url-error");
      Services.prompt.alert(aWindow, brandShortName, errorStr);
      return false;
    }
    return true;
  },

  







  getDescriptionFromDocument: function PUIU_getDescriptionFromDocument(doc) {
    var metaElements = doc.getElementsByTagName("META");
    for (var i = 0; i < metaElements.length; ++i) {
      if (metaElements[i].name.toLowerCase() == "description" ||
          metaElements[i].httpEquiv.toLowerCase() == "description") {
        return metaElements[i].content;
      }
    }
    return "";
  },

  






  getItemDescription: function PUIU_getItemDescription(aItemId) {
    if (PlacesUtils.annotations.itemHasAnnotation(aItemId, this.DESCRIPTION_ANNO))
      return PlacesUtils.annotations.getItemAnnotation(aItemId, this.DESCRIPTION_ANNO);
    return "";
  },

  


  _confirmOpenInTabs:
  function PUIU__confirmOpenInTabs(numTabsToOpen, aWindow) {
    const WARN_ON_OPEN_PREF = "browser.tabs.warnOnOpen";
    var reallyOpen = true;

    if (Services.prefs.getBoolPref(WARN_ON_OPEN_PREF)) {
      if (numTabsToOpen >= Services.prefs.getIntPref("browser.tabs.maxOpenBeforeWarn")) {
        
        var warnOnOpen = { value: true };

        var messageKey = "tabs.openWarningMultipleBranded";
        var openKey = "tabs.openButtonMultiple";
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");

        var buttonPressed = Services.prompt.confirmEx(
          aWindow,
          this.getString("tabs.openWarningTitle"),
          this.getFormattedString(messageKey, [numTabsToOpen, brandShortName]),
          (Services.prompt.BUTTON_TITLE_IS_STRING * Services.prompt.BUTTON_POS_0) +
            (Services.prompt.BUTTON_TITLE_CANCEL * Services.prompt.BUTTON_POS_1),
          this.getString(openKey), null, null,
          this.getFormattedString("tabs.openWarningPromptMeBranded",
                                  [brandShortName]),
          warnOnOpen
        );

        reallyOpen = (buttonPressed == 0);
        
        if (reallyOpen && !warnOnOpen.value)
          Services.prefs.setBoolPref(WARN_ON_OPEN_PREF, false);
      }
    }

    return reallyOpen;
  },

  


  _openTabset: function PUIU__openTabset(aItemsToOpen, aEvent, aWindow) {
    if (!aItemsToOpen.length)
      return;

    var urls = [];
    for (var i = 0; i < aItemsToOpen.length; i++) {
      var item = aItemsToOpen[i];
      if (item.isBookmark)
        this.markPageAsFollowedBookmark(item.uri);
      else
        this.markPageAsTyped(item.uri);

      urls.push(item.uri);
    }

    
    
    var browserWindow = null;
    browserWindow =
      aWindow && aWindow.document.documentElement.getAttribute("windowtype") == "navigator:browser" ?
      aWindow : this._getTopBrowserWin();

    
    
    var where = browserWindow ?
                browserWindow.whereToOpenLink(aEvent, false, true) : "window";
    if (where == "window") {
      
      var uriList = Cc["@mozilla.org/supports-string;1"].
                  createInstance(Ci.nsISupportsString);
      uriList.data = urls.join("|");
      var args = Cc["@mozilla.org/supports-array;1"].
                  createInstance(Ci.nsISupportsArray);
      args.AppendElement(uriList);      
      browserWindow = Services.ww.openWindow(aWindow,
                                             "chrome://browser/content/browser.xul",
                                             null, "chrome,dialog=no,all", args);
      return;
    }

    var loadInBackground = where == "tabshifted" ? true : false;
    var replaceCurrentTab = where == "tab" ? false : true;
    browserWindow.gBrowser.loadTabs(urls, loadInBackground, replaceCurrentTab);
  },

  



  _getWindow: function PUIU__getWindow(aView) {
    if (aView) {
      
      if (aView instanceof Components.interfaces.nsIDOMNode)
        return aView.ownerDocument.defaultView;

      return Cu.getGlobalForObject(aView);
    }

    let caller = arguments.callee.caller;

    
    if (aView === null) {
      Components.utils.reportError("The api has changed. A window should be " +
                                   "passed to " + caller.name + ".  Not " +
                                   "passing a window will throw in a future " +
                                   "release.");
    }
    else {
      Components.utils.reportError("The api has changed. A places view " +
                                   "should be passed to " + caller.name + ". " +
                                   "Not passing a view will throw in a future " +
                                   "release.");
    }

    
    
    let topBrowserWin = this._getTopBrowserWin();
    return topBrowserWin ? topBrowserWin : focusManager.focusedWindow;
  },

  openContainerNodeInTabs:
  function PUIU_openContainerInTabs(aNode, aEvent, aView) {
    let window = this._getWindow(aView);

    let urlsToOpen = PlacesUtils.getURLsForContainerNode(aNode);
    if (!this._confirmOpenInTabs(urlsToOpen.length, window))
      return;

    this._openTabset(urlsToOpen, aEvent, window);
  },

  openURINodesInTabs: function PUIU_openURINodesInTabs(aNodes, aEvent, aView) {
    let window = this._getWindow(aView);

    let urlsToOpen = [];
    for (var i=0; i < aNodes.length; i++) {
      
      if (PlacesUtils.nodeIsURI(aNodes[i]))
        urlsToOpen.push({uri: aNodes[i].uri, isBookmark: PlacesUtils.nodeIsBookmark(aNodes[i])});
    }
    this._openTabset(urlsToOpen, aEvent, window);
  },

  











  openNodeWithEvent:
  function PUIU_openNodeWithEvent(aNode, aEvent, aView) {
    let window = this._getWindow(aView);
    this._openNodeIn(aNode, window.whereToOpenLink(aEvent), window);
  },

  




  openNodeIn: function PUIU_openNodeIn(aNode, aWhere, aView) {
    let window = this._getWindow(aView);
    this._openNodeIn(aNode, aWhere, window);
  },

  _openNodeIn: function PUIU_openNodeIn(aNode, aWhere, aWindow) {
    if (aNode && PlacesUtils.nodeIsURI(aNode) &&
        this.checkURLSecurity(aNode, aWindow)) {
      let isBookmark = PlacesUtils.nodeIsBookmark(aNode);

      if (isBookmark)
        this.markPageAsFollowedBookmark(aNode.uri);
      else
        this.markPageAsTyped(aNode.uri);

      
      
      if (aWhere == "current" && isBookmark) {
        if (PlacesUtils.annotations
                       .itemHasAnnotation(aNode.itemId, this.LOAD_IN_SIDEBAR_ANNO)) {
          let browserWin = this._getTopBrowserWin();
          if (browserWin) {
            browserWin.openWebPanel(aNode.title, aNode.uri);
            return;
          }
        }
      }
      aWindow.openUILinkIn(aNode.uri, aWhere);
    }
  },

  









  guessUrlSchemeForUI: function PUIU_guessUrlSchemeForUI(aUrlString) {
    return aUrlString.substr(0, aUrlString.indexOf(":"));
  },

  getBestTitle: function PUIU_getBestTitle(aNode) {
    var title;
    if (!aNode.title && PlacesUtils.uriTypes.indexOf(aNode.type) != -1) {
      
      
      try {
        var uri = PlacesUtils._uri(aNode.uri);
        var host = uri.host;
        var fileName = uri.QueryInterface(Ci.nsIURL).fileName;
        
        title = host + (fileName ?
                        (host ? "/" + this.ellipsis + "/" : "") + fileName :
                        uri.path);
      }
      catch (e) {
        
        title = "";
      }
    }
    else
      title = aNode.title;

    return title || this.getString("noTitle");
  },

  get leftPaneQueries() {
    
    this.leftPaneFolderId;
    return this.leftPaneQueries;
  },

  
  get leftPaneFolderId() {
    let leftPaneRoot = -1;
    let allBookmarksId;

    
    let bs = PlacesUtils.bookmarks;
    let as = PlacesUtils.annotations;

    
    let queries = {
      "PlacesRoot": { title: "" },
      "History": { title: this.getString("OrganizerQueryHistory") },
      "Tags": { title: this.getString("OrganizerQueryTags") },
      "AllBookmarks": { title: this.getString("OrganizerQueryAllBookmarks") },
      "BookmarksToolbar":
        { title: null,
          concreteTitle: PlacesUtils.getString("BookmarksToolbarFolderTitle"),
          concreteId: PlacesUtils.toolbarFolderId },
      "BookmarksMenu":
        { title: null,
          concreteTitle: PlacesUtils.getString("BookmarksMenuFolderTitle"),
          concreteId: PlacesUtils.bookmarksMenuFolderId },
      "UnfiledBookmarks":
        { title: null,
          concreteTitle: PlacesUtils.getString("UnsortedBookmarksFolderTitle"),
          concreteId: PlacesUtils.unfiledBookmarksFolderId },
    };
    
    const EXPECTED_QUERY_COUNT = 6;

    
    function safeRemoveItem(aItemId) {
      try {
        if (as.itemHasAnnotation(aItemId, PlacesUIUtils.ORGANIZER_QUERY_ANNO) &&
            !(as.getItemAnnotation(aItemId, PlacesUIUtils.ORGANIZER_QUERY_ANNO) in queries)) {
          
          
          return;
        }
        
        
        as.removeItemAnnotation(aItemId, PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
        as.removeItemAnnotation(aItemId, PlacesUIUtils.ORGANIZER_QUERY_ANNO);
        
        bs.removeItem(aItemId);
      }
      catch(e) {  }
    }

    
    function itemExists(aItemId) {
      try {
        bs.getItemIndex(aItemId);
        return true;
      }
      catch(e) {
        return false;
      }
    }

    
    let items = as.getItemsWithAnnotation(this.ORGANIZER_FOLDER_ANNO);
    if (items.length > 1) {
      
      
      items.forEach(safeRemoveItem);
    }
    else if (items.length == 1 && items[0] != -1) {
      leftPaneRoot = items[0];
      
      let version = as.getItemAnnotation(leftPaneRoot, this.ORGANIZER_FOLDER_ANNO);
      if (version != this.ORGANIZER_LEFTPANE_VERSION ||
          !itemExists(leftPaneRoot)) {
        
        safeRemoveItem(leftPaneRoot);
        leftPaneRoot = -1;
      }
    }

    if (leftPaneRoot != -1) {
      
      
      
      delete this.leftPaneQueries;
      this.leftPaneQueries = {};

      let items = as.getItemsWithAnnotation(this.ORGANIZER_QUERY_ANNO);
      
      let queriesCount = 0;
      for (let i = 0; i < items.length; i++) {
        let queryName = as.getItemAnnotation(items[i], this.ORGANIZER_QUERY_ANNO);

        
        
        if (!(queryName in queries))
          continue;

        let query = queries[queryName];
        query.itemId = items[i];

        if (!itemExists(query.itemId)) {
          
          break;
        }

        
        let parentId = bs.getFolderIdForItem(query.itemId);
        if (items.indexOf(parentId) == -1 && parentId != leftPaneRoot) {
          
          
          break;
        }

        
        
        if (bs.getItemTitle(query.itemId) != query.title)
          bs.setItemTitle(query.itemId, query.title);
        if ("concreteId" in query) {
          if (bs.getItemTitle(query.concreteId) != query.concreteTitle)
            bs.setItemTitle(query.concreteId, query.concreteTitle);
        }

        
        this.leftPaneQueries[queryName] = query.itemId;
        queriesCount++;
      }

      if (queriesCount != EXPECTED_QUERY_COUNT) {
        
        
        
        items.forEach(safeRemoveItem);
        safeRemoveItem(leftPaneRoot);
      }
      else {
        
        delete this.leftPaneFolderId;
        return this.leftPaneFolderId = leftPaneRoot;
      }
    }

    
    var callback = {
      
      create_query: function CB_create_query(aQueryName, aParentId, aQueryUrl) {
        let itemId = bs.insertBookmark(aParentId,
                                       PlacesUtils._uri(aQueryUrl),
                                       bs.DEFAULT_INDEX,
                                       queries[aQueryName].title);
        
        as.setItemAnnotation(itemId, PlacesUIUtils.ORGANIZER_QUERY_ANNO, aQueryName,
                             0, as.EXPIRE_NEVER);
        
        as.setItemAnnotation(itemId, PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO, 1,
                             0, as.EXPIRE_NEVER);
        
        PlacesUIUtils.leftPaneQueries[aQueryName] = itemId;
        return itemId;
      },

      
      create_folder: function CB_create_folder(aFolderName, aParentId, aIsRoot) {
              
        let folderId = bs.createFolder(aParentId,
                                       queries[aFolderName].title,
                                       bs.DEFAULT_INDEX);
        
        as.setItemAnnotation(folderId, PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO, 1,
                             0, as.EXPIRE_NEVER);
        
        bs.setFolderReadonly(folderId, true);

        if (aIsRoot) {
          
          as.setItemAnnotation(folderId, PlacesUIUtils.ORGANIZER_FOLDER_ANNO,
                               PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION,
                               0, as.EXPIRE_NEVER);
        }
        else {
          
          as.setItemAnnotation(folderId, PlacesUIUtils.ORGANIZER_QUERY_ANNO, aFolderName,
                           0, as.EXPIRE_NEVER);
          PlacesUIUtils.leftPaneQueries[aFolderName] = folderId;
        }
        return folderId;
      },

      runBatched: function CB_runBatched(aUserData) {
        delete PlacesUIUtils.leftPaneQueries;
        PlacesUIUtils.leftPaneQueries = { };

        
        leftPaneRoot = this.create_folder("PlacesRoot", bs.placesRoot, true);

        
        this.create_query("History", leftPaneRoot,
                          "place:type=" +
                          Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY +
                          "&sort=" +
                          Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING);

        

        
        this.create_query("Tags", leftPaneRoot,
                          "place:type=" +
                          Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY +
                          "&sort=" +
                          Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING);

        
        allBookmarksId = this.create_folder("AllBookmarks", leftPaneRoot, false);

        
        this.create_query("BookmarksToolbar", allBookmarksId,
                          "place:folder=TOOLBAR");

        
        this.create_query("BookmarksMenu", allBookmarksId,
                          "place:folder=BOOKMARKS_MENU");

        
        this.create_query("UnfiledBookmarks", allBookmarksId,
                          "place:folder=UNFILED_BOOKMARKS");
      }
    };
    bs.runInBatchMode(callback, null);

    delete this.leftPaneFolderId;
    return this.leftPaneFolderId = leftPaneRoot;
  },

  


  get allBookmarksFolderId() {
    
    this.leftPaneFolderId;
    delete this.allBookmarksFolderId;
    return this.allBookmarksFolderId = this.leftPaneQueries["AllBookmarks"];
  },

  






  getLeftPaneQueryNameFromId: function PUIU_getLeftPaneQueryNameFromId(aItemId) {
    var queryName = "";
    
    
    if (Object.getOwnPropertyDescriptor(this, "leftPaneFolderId").value === undefined) {
      try {
        queryName = PlacesUtils.annotations.
                                getItemAnnotation(aItemId, this.ORGANIZER_QUERY_ANNO);
      }
      catch (ex) {
        
        queryName = "";
      }
    }
    else {
      
      
      for (let [name, id] in Iterator(this.leftPaneQueries)) {
        if (aItemId == id)
          queryName = name;
      }
    }
    return queryName;
  }
};

XPCOMUtils.defineLazyServiceGetter(PlacesUIUtils, "RDF",
                                   "@mozilla.org/rdf/rdf-service;1",
                                   "nsIRDFService");

XPCOMUtils.defineLazyGetter(PlacesUIUtils, "localStore", function() {
  return PlacesUIUtils.RDF.GetDataSource("rdf:local-store");
});

XPCOMUtils.defineLazyGetter(PlacesUIUtils, "ellipsis", function() {
  return Services.prefs.getComplexValue("intl.ellipsis",
                                        Ci.nsIPrefLocalizedString).data;
});

XPCOMUtils.defineLazyServiceGetter(PlacesUIUtils, "privateBrowsing",
                                   "@mozilla.org/privatebrowsing;1",
                                   "nsIPrivateBrowsingService");

XPCOMUtils.defineLazyServiceGetter(this, "URIFixup",
                                   "@mozilla.org/docshell/urifixup;1",
                                   "nsIURIFixup");

XPCOMUtils.defineLazyGetter(this, "bundle", function() {
  const PLACES_STRING_BUNDLE_URI =
    "chrome://browser/locale/places/places.properties";
  return Cc["@mozilla.org/intl/stringbundle;1"].
         getService(Ci.nsIStringBundleService).
         createBundle(PLACES_STRING_BUNDLE_URI);
});

XPCOMUtils.defineLazyServiceGetter(this, "focusManager",
                                   "@mozilla.org/focus-manager;1",
                                   "nsIFocusManager");









XPCOMUtils.defineLazyGetter(PlacesUIUtils, "ptm", function() {
  
  PlacesUtils;

  return {
    aggregateTransactions: function(aName, aTransactions)
      new PlacesAggregatedTransaction(aName, aTransactions),

    createFolder: function(aName, aContainer, aIndex, aAnnotations,
                           aChildItemsTransactions)
      new PlacesCreateFolderTransaction(aName, aContainer, aIndex, aAnnotations,
                                        aChildItemsTransactions),

    createItem: function(aURI, aContainer, aIndex, aTitle, aKeyword,
                         aAnnotations, aChildTransactions)
      new PlacesCreateBookmarkTransaction(aURI, aContainer, aIndex, aTitle,
                                          aKeyword, aAnnotations,
                                          aChildTransactions),

    createSeparator: function(aContainer, aIndex)
      new PlacesCreateSeparatorTransaction(aContainer, aIndex),

    createLivemark: function(aFeedURI, aSiteURI, aName, aContainer, aIndex,
                             aAnnotations)
      new PlacesCreateLivemarkTransaction(aFeedURI, aSiteURI, aName, aContainer,
                                          aIndex, aAnnotations),

    moveItem: function(aItemId, aNewContainer, aNewIndex)
      new PlacesMoveItemTransaction(aItemId, aNewContainer, aNewIndex),

    removeItem: function(aItemId)
      new PlacesRemoveItemTransaction(aItemId),

    editItemTitle: function(aItemId, aNewTitle)
      new PlacesEditItemTitleTransaction(aItemId, aNewTitle),

    editBookmarkURI: function(aItemId, aNewURI)
      new PlacesEditBookmarkURITransaction(aItemId, aNewURI),

    setItemAnnotation: function(aItemId, aAnnotationObject)
      new PlacesSetItemAnnotationTransaction(aItemId, aAnnotationObject),

    setPageAnnotation: function(aURI, aAnnotationObject)
      new PlacesSetPageAnnotationTransaction(aURI, aAnnotationObject),

    editBookmarkKeyword: function(aItemId, aNewKeyword)
      new PlacesEditBookmarkKeywordTransaction(aItemId, aNewKeyword),

    editBookmarkPostData: function(aItemId, aPostData)
      new PlacesEditBookmarkPostDataTransaction(aItemId, aPostData),

    editLivemarkSiteURI: function(aLivemarkId, aSiteURI)
      new PlacesEditLivemarkSiteURITransaction(aLivemarkId, aSiteURI),

    editLivemarkFeedURI: function(aLivemarkId, aFeedURI)
      new PlacesEditLivemarkFeedURITransaction(aLivemarkId, aFeedURI),

    editItemDateAdded: function(aItemId, aNewDateAdded)
      new PlacesEditItemDateAddedTransaction(aItemId, aNewDateAdded),

    editItemLastModified: function(aItemId, aNewLastModified)
      new PlacesEditItemLastModifiedTransaction(aItemId, aNewLastModified),

    sortFolderByName: function(aFolderId)
      new PlacesSortFolderByNameTransaction(aFolderId),

    tagURI: function(aURI, aTags)
      new PlacesTagURITransaction(aURI, aTags),

    untagURI: function(aURI, aTags)
      new PlacesUntagURITransaction(aURI, aTags),

    








    setLoadInSidebar: function(aItemId, aLoadInSidebar)
    {
      let annoObj = { name: PlacesUIUtils.LOAD_IN_SIDEBAR_ANNO,
                      type: Ci.nsIAnnotationService.TYPE_INT32,
                      flags: 0,
                      value: aLoadInSidebar,
                      expires: Ci.nsIAnnotationService.EXPIRE_NEVER };
      return new PlacesSetItemAnnotationTransaction(aItemId, annoObj);
    },

   








    editItemDescription: function(aItemId, aDescription)
    {
      let annoObj = { name: PlacesUIUtils.DESCRIPTION_ANNO,
                      type: Ci.nsIAnnotationService.TYPE_STRING,
                      flags: 0,
                      value: aDescription,
                      expires: Ci.nsIAnnotationService.EXPIRE_NEVER };
      return new PlacesSetItemAnnotationTransaction(aItemId, annoObj);
    },

    
    

    beginBatch: function()
      PlacesUtils.transactionManager.beginBatch(),

    endBatch: function()
      PlacesUtils.transactionManager.endBatch(),

    doTransaction: function(txn)
      PlacesUtils.transactionManager.doTransaction(txn),

    undoTransaction: function()
      PlacesUtils.transactionManager.undoTransaction(),

    redoTransaction: function()
      PlacesUtils.transactionManager.redoTransaction(),

    get numberOfUndoItems()
      PlacesUtils.transactionManager.numberOfUndoItems,
    get numberOfRedoItems()
      PlacesUtils.transactionManager.numberOfRedoItems,
    get maxTransactionCount()
      PlacesUtils.transactionManager.maxTransactionCount,
    set maxTransactionCount(val)
      PlacesUtils.transactionManager.maxTransactionCount = val,

    clear: function()
      PlacesUtils.transactionManager.clear(),

    peekUndoStack: function()
      PlacesUtils.transactionManager.peekUndoStack(),

    peekRedoStack: function()
      PlacesUtils.transactionManager.peekRedoStack(),

    getUndoStack: function()
      PlacesUtils.transactionManager.getUndoStack(),

    getRedoStack: function()
      PlacesUtils.transactionManager.getRedoStack(),

    AddListener: function(aListener)
      PlacesUtils.transactionManager.AddListener(aListener),

    RemoveListener: function(aListener)
      PlacesUtils.transactionManager.RemoveListener(aListener)
  }
});
