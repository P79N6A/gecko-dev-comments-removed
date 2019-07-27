




this.EXPORTED_SYMBOLS = ["PlacesUIUtils"];

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;
var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");


Cu.import("resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesTransactions",
                                  "resource://gre/modules/PlacesTransactions.jsm");

#ifdef MOZ_SERVICES_CLOUDSYNC
XPCOMUtils.defineLazyModuleGetter(this, "CloudSync",
                                  "resource://gre/modules/CloudSync.jsm");
#else
let CloudSync = null;
#endif

#ifdef MOZ_SERVICES_SYNC
XPCOMUtils.defineLazyModuleGetter(this, "Weave",
                                  "resource://services-sync/main.js");
#endif


const TAB_DROP_TYPE = "application/x-moz-tabbrowser-tab";



function IsLivemark(aItemId) {
  
  
  let self = IsLivemark;
  if (!("ids" in self)) {
    const LIVEMARK_ANNO = PlacesUtils.LMANNO_FEEDURI;

    let idsVec = PlacesUtils.annotations.getItemsWithAnnotation(LIVEMARK_ANNO);
    self.ids = new Set(idsVec);

    let obs = Object.freeze({
      QueryInterface: XPCOMUtils.generateQI(Ci.nsIAnnotationObserver),

      onItemAnnotationSet(itemId, annoName) {
        if (annoName == LIVEMARK_ANNO)
          self.ids.add(itemId);
      },

      onItemAnnotationRemoved(itemId, annoName) {
        
        if (annoName == LIVEMARK_ANNO || annoName == "")
          self.ids.delete(itemId);
      },

      onPageAnnotationSet() { },
      onPageAnnotationRemoved() { },
    });
    PlacesUtils.annotations.addObserver(obs);
    PlacesUtils.registerShutdownFunction(() => {
      PlacesUtils.annotations.removeObserver(obs);
    });
  }
  return self.ids.has(aItemId);
}

this.PlacesUIUtils = {
  ORGANIZER_LEFTPANE_VERSION: 7,
  ORGANIZER_FOLDER_ANNO: "PlacesOrganizer/OrganizerFolder",
  ORGANIZER_QUERY_ANNO: "PlacesOrganizer/OrganizerQuery",

  LOAD_IN_SIDEBAR_ANNO: "bookmarkProperties/loadInSidebar",
  DESCRIPTION_ANNO: "bookmarkProperties/description",

  





  createFixedURI: function PUIU_createFixedURI(aSpec) {
    return URIFixup.createFixupURI(aSpec, Ci.nsIURIFixup.FIXUP_FLAG_NONE);
  },

  getFormattedString: function PUIU_getFormattedString(key, params) {
    return bundle.formatStringFromName(key, params, params.length);
  },

  














  getPluralString: function PUIU_getPluralString(aKey, aNumber, aParams) {
    let str = PluralForm.get(aNumber, bundle.GetStringFromName(aKey));

    
    return str.replace(/\#(\d+)/g, function (matchedId, matchedNumber) {
      let param = aParams[parseInt(matchedNumber, 10) - 1];
      return param !== undefined ? param : matchedId;
    });
  },

  getString: function PUIU_getString(key) {
    return bundle.GetStringFromName(key);
  },

  get _copyableAnnotations() [
    this.DESCRIPTION_ANNO,
    this.LOAD_IN_SIDEBAR_ANNO,
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

    let annos = [];
    if (aData.annos) {
      annos = aData.annos.filter(function (aAnno) {
        return this._copyableAnnotations.indexOf(aAnno.name) != -1;
      }, this);
    }

    
    return new PlacesCreateBookmarkTransaction(PlacesUtils._uri(aData.uri),
                                               aContainer, aIndex, aData.title,
                                               null, annos, transactions);
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
            type == TAB_DROP_TYPE) {
          let title = type != PlacesUtils.TYPE_UNICODE ? data.title
                                                       : data.uri;
          return new PlacesCreateBookmarkTransaction(PlacesUtils._uri(data.uri),
                                                     container, index, title);
        }
    }
    return null;
  },

  



















  getTransactionForData: function(aData, aType, aNewParentGuid, aIndex, aCopy) {
    if (this.SUPPORTED_FLAVORS.indexOf(aData.type) == -1)
      throw new Error(`Unsupported '${aData.type}' data type`);

    if ("itemGuid" in aData) {
      if (this.PLACES_FLAVORS.indexOf(aData.type) == -1)
        throw new Error (`itemGuid unexpectedly set on ${aData.type} data`);

      let info = { guid: aData.itemGuid
                 , newParentGuid: aNewParentGuid
                 , newIndex: aIndex };
      if (aCopy) {
        info.excludingAnnotation = "Places/SmartBookmark";
        return PlacesTransactions.Copy(info);
      }
      return PlacesTransactions.Move(info);
    }

    
    
    
    
    if (aData.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER)
      throw new Error("Can't copy a container from a legacy-transactions build");

    if (aData.type == PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR) {
      return PlacesTransactions.NewSeparator({ parentGuid: aNewParentGuid
                                             , index: aIndex });
    }

    let title = aData.type != PlacesUtils.TYPE_UNICODE ? aData.title
                                                       : aData.uri;
    return PlacesTransactions.NewBookmark({ uri: NetUtil.newURI(aData.uri)
                                          , title: title
                                          , parentGuid: aNewParentGuid
                                          , index: aIndex });
  },

  











  showBookmarkDialog:
  function PUIU_showBookmarkDialog(aInfo, aParentWindow) {
    
    
    
    
    let hasFolderPicker = !("hiddenRows" in aInfo) ||
                          aInfo.hiddenRows.indexOf("folderPicker") == -1;
    
    
    let dialogURL = hasFolderPicker ?
                    "chrome://browser/content/places/bookmarkProperties2.xul" :
                    "chrome://browser/content/places/bookmarkProperties.xul";

    let features =
      "centerscreen,chrome,modal,resizable=" + (hasFolderPicker ? "yes" : "no");

    aParentWindow.openDialog(dialogURL, "",  features, aInfo);
    return ("performed" in aInfo && aInfo.performed);
  },

  _getTopBrowserWin: function PUIU__getTopBrowserWin() {
    return RecentWindow.getMostRecentBrowserWindow();
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
    PlacesUtils.history.markPageAsTyped(this.createFixedURI(aURL));
  },

  






  markPageAsFollowedBookmark: function PUIU_markPageAsFollowedBookmark(aURL) {
    PlacesUtils.history.markPageAsFollowedBookmark(this.createFixedURI(aURL));
  },

  





  markPageAsFollowedLink: function PUIU_markPageAsFollowedLink(aURL) {
    PlacesUtils.history.markPageAsFollowedLink(this.createFixedURI(aURL));
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

  







  canUserRemove: function (aNode) {
    let parentNode = aNode.parent;
    if (!parentNode)
      throw new Error("canUserRemove doesn't accept root nodes");

    
    
    if (aNode.itemId == -1) {
      
      
      
      return !PlacesUtils.nodeIsFolder(parentNode);
    }

    
    if (PlacesUtils.nodeIsQuery(parentNode))
      return true;

    
    return !this.isContentsReadOnly(parentNode);
  },

  























  isContentsReadOnly: function (aNodeOrItemId) {
    let itemId;
    if (typeof(aNodeOrItemId) == "number") {
      itemId = aNodeOrItemId;
    }
    else if (PlacesUtils.nodeIsFolder(aNodeOrItemId)) {
      itemId = PlacesUtils.getConcreteItemId(aNodeOrItemId);
    }
    else {
      throw new Error("invalid value for aNodeOrItemId");
    }

    if (itemId == PlacesUtils.placesRootId || IsLivemark(itemId))
      return true;

    
    
    
    
    
    
    
    
    
    
    
    if ("get" in Object.getOwnPropertyDescriptor(this, "leftPaneFolderId"))
      return false;

    return itemId == this.leftPaneFolderId ||
           itemId == this.allBookmarksFolderId;
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

    
    
    var browserWindow = null;
    browserWindow =
      aWindow && aWindow.document.documentElement.getAttribute("windowtype") == "navigator:browser" ?
      aWindow : this._getTopBrowserWin();

    var urls = [];
    let skipMarking = browserWindow && PrivateBrowsingUtils.isWindowPrivate(browserWindow);
    for (let item of aItemsToOpen) {
      urls.push(item.uri);
      if (skipMarking) {
        continue;
      }

      if (item.isBookmark)
        this.markPageAsFollowedBookmark(item.uri);
      else
        this.markPageAsTyped(item.uri);
    }

    
    
    var where = browserWindow ?
                browserWindow.whereToOpenLink(aEvent, false, true) : "window";
    if (where == "window") {
      
      var uriList = PlacesUtils.toISupportsString(urls.join("|"));
      var args = Cc["@mozilla.org/supports-array;1"].
                  createInstance(Ci.nsISupportsArray);
      args.AppendElement(uriList);
      browserWindow = Services.ww.openWindow(aWindow,
                                             "chrome://browser/content/browser.xul",
                                             null, "chrome,dialog=no,all", args);
      return;
    }

    var loadInBackground = where == "tabshifted" ? true : false;
    
    
    
    browserWindow.gBrowser.loadTabs(urls, loadInBackground, false);
  },

  openContainerNodeInTabs:
  function PUIU_openContainerInTabs(aNode, aEvent, aView) {
    let window = aView.ownerWindow;

    let urlsToOpen = PlacesUtils.getURLsForContainerNode(aNode);
    if (!this._confirmOpenInTabs(urlsToOpen.length, window))
      return;

    this._openTabset(urlsToOpen, aEvent, window);
  },

  openURINodesInTabs: function PUIU_openURINodesInTabs(aNodes, aEvent, aView) {
    let window = aView.ownerWindow;

    let urlsToOpen = [];
    for (var i=0; i < aNodes.length; i++) {
      
      if (PlacesUtils.nodeIsURI(aNodes[i]))
        urlsToOpen.push({uri: aNodes[i].uri, isBookmark: PlacesUtils.nodeIsBookmark(aNodes[i])});
    }
    this._openTabset(urlsToOpen, aEvent, window);
  },

  











  openNodeWithEvent:
  function PUIU_openNodeWithEvent(aNode, aEvent, aView) {
    let window = aView.ownerWindow;
    this._openNodeIn(aNode, window.whereToOpenLink(aEvent, false, true), window);
  },

  




  openNodeIn: function PUIU_openNodeIn(aNode, aWhere, aView, aPrivate) {
    let window = aView.ownerWindow;
    this._openNodeIn(aNode, aWhere, window, aPrivate);
  },

  _openNodeIn: function PUIU_openNodeIn(aNode, aWhere, aWindow, aPrivate=false) {
    if (aNode && PlacesUtils.nodeIsURI(aNode) &&
        this.checkURLSecurity(aNode, aWindow)) {
      let isBookmark = PlacesUtils.nodeIsBookmark(aNode);

      if (!PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
        if (isBookmark)
          this.markPageAsFollowedBookmark(aNode.uri);
        else
          this.markPageAsTyped(aNode.uri);
      }

      
      
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

      aWindow.openUILinkIn(aNode.uri, aWhere, {
        inBackground: Services.prefs.getBoolPref("browser.tabs.loadBookmarksInBackground"),
        private: aPrivate,
      });
    }
  },

  









  guessUrlSchemeForUI: function PUIU_guessUrlSchemeForUI(aUrlString) {
    return aUrlString.substr(0, aUrlString.indexOf(":"));
  },

  getBestTitle: function PUIU_getBestTitle(aNode, aDoNotCutTitle) {
    var title;
    if (!aNode.title && PlacesUtils.nodeIsURI(aNode)) {
      
      
      try {
        var uri = PlacesUtils._uri(aNode.uri);
        var host = uri.host;
        var fileName = uri.QueryInterface(Ci.nsIURL).fileName;
        
        if (aDoNotCutTitle) {
          title = host + uri.path;
        } else {
          title = host + (fileName ?
                           (host ? "/" + this.ellipsis + "/" : "") + fileName :
                           uri.path);
        }
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
      "Downloads": { title: this.getString("OrganizerQueryDownloads") },
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
    
    const EXPECTED_QUERY_COUNT = 7;

    
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
      let corrupt = false;
      for (let i = 0; i < items.length; i++) {
        let queryName = as.getItemAnnotation(items[i], this.ORGANIZER_QUERY_ANNO);

        
        
        if (!(queryName in queries))
          continue;

        let query = queries[queryName];
        query.itemId = items[i];

        if (!itemExists(query.itemId)) {
          
          corrupt = true;
          break;
        }

        
        let parentId = bs.getFolderIdForItem(query.itemId);
        if (items.indexOf(parentId) == -1 && parentId != leftPaneRoot) {
          
          
          corrupt = true;
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

      
      
      
      
      if (corrupt || queriesCount != EXPECTED_QUERY_COUNT) {
        
        
        
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

        
        this.create_query("Downloads", leftPaneRoot,
                          "place:transition=" +
                          Ci.nsINavHistoryService.TRANSITION_DOWNLOAD +
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
  },

  shouldShowTabsFromOtherComputersMenuitem: function() {
    let weaveOK = Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED &&
                  Weave.Svc.Prefs.get("firstSync", "") != "notReady";
    let cloudSyncOK = CloudSync && CloudSync.ready && CloudSync().tabsReady && CloudSync().tabs.hasRemoteTabs();
    return weaveOK || cloudSyncOK;
  },

  shouldEnableTabsFromOtherComputersMenuitem: function() {
    let weaveEnabled = Weave.Service.isLoggedIn &&
                       Weave.Service.engineManager.get("tabs") &&
                       Weave.Service.engineManager.get("tabs").enabled;
    let cloudSyncEnabled = CloudSync && CloudSync.ready && CloudSync().tabsReady && CloudSync().tabs.hasRemoteTabs();
    return weaveEnabled || cloudSyncEnabled;
  },
};


PlacesUIUtils.PLACES_FLAVORS = [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                                PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                                PlacesUtils.TYPE_X_MOZ_PLACE];

PlacesUIUtils.URI_FLAVORS = [PlacesUtils.TYPE_X_MOZ_URL,
                             TAB_DROP_TYPE,
                             PlacesUtils.TYPE_UNICODE],

PlacesUIUtils.SUPPORTED_FLAVORS = [...PlacesUIUtils.PLACES_FLAVORS,
                                   ...PlacesUIUtils.URI_FLAVORS];

XPCOMUtils.defineLazyServiceGetter(PlacesUIUtils, "RDF",
                                   "@mozilla.org/rdf/rdf-service;1",
                                   "nsIRDFService");

XPCOMUtils.defineLazyGetter(PlacesUIUtils, "ellipsis", function() {
  return Services.prefs.getComplexValue("intl.ellipsis",
                                        Ci.nsIPrefLocalizedString).data;
});

XPCOMUtils.defineLazyGetter(PlacesUIUtils, "useAsyncTransactions", function() {
  try {
    return Services.prefs.getBoolPref("browser.places.useAsyncTransactions");
  }
  catch(ex) { }
  return false;
});

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
      PlacesUtils.transactionManager.beginBatch(null),

    endBatch: function()
      PlacesUtils.transactionManager.endBatch(false),

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
