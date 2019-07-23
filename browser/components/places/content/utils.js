









































function LOG(str) {
  dump("*** " + str + "\n");
}

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

__defineGetter__("PlacesUtils", function() {
  delete this.PlacesUtils
  var tmpScope = {};
  Components.utils.import("resource://gre/modules/utils.js", tmpScope);
  return this.PlacesUtils = tmpScope.PlacesUtils;
});

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";
const GUID_ANNO = "placesInternal/GUID";
const LMANNO_FEEDURI = "livemark/feedURI";
const LMANNO_SITEURI = "livemark/siteURI";
const ORGANIZER_FOLDER_ANNO = "PlacesOrganizer/OrganizerFolder";
const ORGANIZER_QUERY_ANNO = "PlacesOrganizer/OrganizerQuery";
const ORGANIZER_LEFTPANE_VERSION = 5;
const EXCLUDE_FROM_BACKUP_ANNO = "places/excludeFromBackup";

#ifdef XP_MACOSX


const NEWLINE= "\n";
#else

const NEWLINE = "\r\n";
#endif

function QI_node(aNode, aIID) {
  return aNode.QueryInterface(aIID);
}
function asVisit(aNode)    { return QI_node(aNode, Ci.nsINavHistoryVisitResultNode);    }
function asFullVisit(aNode){ return QI_node(aNode, Ci.nsINavHistoryFullVisitResultNode);}
function asContainer(aNode){ return QI_node(aNode, Ci.nsINavHistoryContainerResultNode);}
function asQuery(aNode)    { return QI_node(aNode, Ci.nsINavHistoryQueryResultNode);    }

var PlacesUIUtils = {
  


  get microsummaries() {
    delete this.microsummaries;
    return this.microsummaries = Cc["@mozilla.org/microsummary/service;1"].
                                 getService(Ci.nsIMicrosummaryService);
  },

  get RDF() {
    delete this.RDF;
    return this.RDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                      getService(Ci.nsIRDFService);
  },

  get localStore() {
    delete this.localStore;
    return this.localStore = this.RDF.GetDataSource("rdf:local-store");
  },

  get ptm() {
    delete this.ptm;
    return this.ptm = Cc["@mozilla.org/browser/placesTransactionsService;1"].
                      getService(Ci.nsIPlacesTransactionsService);
  },

  get clipboard() {
    delete this.clipboard;
    return this.clipboard = Cc["@mozilla.org/widget/clipboard;1"].
                            getService(Ci.nsIClipboard);
  },

  get URIFixup() {
    delete this.URIFixup;
    return this.URIFixup = Cc["@mozilla.org/docshell/urifixup;1"].
                           getService(Ci.nsIURIFixup);
  },

  get ellipsis() {
    delete this.ellipsis;
    var pref = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefBranch);
    return this.ellipsis = pref.getComplexValue("intl.ellipsis",
                                                Ci.nsIPrefLocalizedString).data;
  },

  





  createFixedURI: function PU_createFixedURI(aSpec) {
    return this.URIFixup.createFixupURI(aSpec, 0);
  },

  





  _wrapString: function PU__wrapString(aString) {
    var s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  


  get _bundle() {
    const PLACES_STRING_BUNDLE_URI =
        "chrome://browser/locale/places/places.properties";
    delete this._bundle;
    return this._bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                          getService(Ci.nsIStringBundleService).
                          createBundle(PLACES_STRING_BUNDLE_URI);
  },

  getFormattedString: function PU_getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },

  getString: function PU_getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  










  _getURIItemCopyTransaction: function (aData, aContainer, aIndex) {
    return this.ptm.createItem(PlacesUtils._uri(aData.uri), aContainer, aIndex,
                               aData.title, "");
  },

  













  _getBookmarkItemCopyTransaction:
  function PU__getBookmarkItemCopyTransaction(aData, aContainer, aIndex,
                                              aExcludeAnnotations) {
    var itemURL = PlacesUtils._uri(aData.uri);
    var itemTitle = aData.title;
    var keyword = aData.keyword || null;
    var annos = aData.annos || [];
    
    var excludeAnnos = [GUID_ANNO];
    if (aExcludeAnnotations)
      excludeAnnos = excludeAnnos.concat(aExcludeAnnotations);
    annos = annos.filter(function(aValue, aIndex, aArray) {
      return excludeAnnos.indexOf(aValue.name) == -1;
    });
    var childTxns = [];
    if (aData.dateAdded)
      childTxns.push(this.ptm.editItemDateAdded(null, aData.dateAdded));
    if (aData.lastModified)
      childTxns.push(this.ptm.editItemLastModified(null, aData.lastModified));
    if (aData.tags) {
      var tags = aData.tags.split(", ");
      
      
      var storedTags = PlacesUtils.tagging.getTagsForURI(itemURL, {});
      tags = tags.filter(function (aTag) {
        return (storedTags.indexOf(aTag) == -1);
      }, this);
      if (tags.length)
        childTxns.push(this.ptm.tagURI(itemURL, tags));
    }

    return this.ptm.createItem(itemURL, aContainer, aIndex, itemTitle, keyword,
                               annos, childTxns);
  },

  











  _getFolderCopyTransaction:
  function PU__getFolderCopyTransaction(aData, aContainer, aIndex) {
    var self = this;
    function getChildItemsTransactions(aChildren) {
      var childItemsTransactions = [];
      var cc = aChildren.length;
      var index = aIndex;
      for (var i = 0; i < cc; ++i) {
        var txn = null;
        var node = aChildren[i];

        
        
        
        if (aIndex != PlacesUtils.bookmarks.DEFAULT_INDEX)
          index = i;

        if (node.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER) {
          if (node.livemark && node.annos) 
            txn = self._getLivemarkCopyTransaction(node, aContainer, index);
          else
            txn = self._getFolderCopyTransaction(node, aContainer, index);
        }
        else if (node.type == PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR)
          txn = self.ptm.createSeparator(-1, index);
        else if (node.type == PlacesUtils.TYPE_X_MOZ_PLACE)
          txn = self._getBookmarkItemCopyTransaction(node, -1, index);

        NS_ASSERT(txn, "Unexpected item under a bookmarks folder");
        if (txn)
          childItemsTransactions.push(txn);
      }
      return childItemsTransactions;
    }

    
    if (aContainer == PlacesUtils.bookmarks.tagsFolder) {
      var txns = [];
      if (aData.children) {
        aData.children.forEach(function(aChild) {
          txns.push(this.ptm.tagURI(PlacesUtils._uri(aChild.uri), [aData.title]));
        }, this);
      }
      return this.ptm.aggregateTransactions("addTags", txns);
    }
    else if (aData.livemark && aData.annos) {
      
      return this._getLivemarkCopyTransaction(aData, aContainer, aIndex);
    }
    else {
      var childItems = getChildItemsTransactions(aData.children);
      if (aData.dateAdded)
        childItems.push(this.ptm.editItemDateAdded(null, aData.dateAdded));
      if (aData.lastModified)
        childItems.push(this.ptm.editItemLastModified(null, aData.lastModified));

      var annos = aData.annos || [];
      annos = annos.filter(function(aAnno) {
        
        return aAnno.name != GUID_ANNO;
      });
      return this.ptm.createFolder(aData.title, aContainer, aIndex, annos, childItems);
    }
  },

  _getLivemarkCopyTransaction:
  function PU__getLivemarkCopyTransaction(aData, aContainer, aIndex) {
    NS_ASSERT(aData.livemark && aData.annos, "node is not a livemark");
    
    var feedURI = null;
    var siteURI = null;
    aData.annos = aData.annos.filter(function(aAnno) {
      if (aAnno.name == LMANNO_FEEDURI) {
        feedURI = PlacesUtils._uri(aAnno.value);
        return false;
      }
      else if (aAnno.name == LMANNO_SITEURI) {
        siteURI = PlacesUtils._uri(aAnno.value);
        return false;
      }
      
      return aAnno.name != GUID_ANNO;
    });
    return this.ptm.createLivemark(feedURI, siteURI, aData.title, aContainer,
                                   aIndex, aData.annos);
  },

  















  makeTransaction: function PU_makeTransaction(data, type, container,
                                               index, copy) {
    switch (data.type) {
      case PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER:
        if (copy)
          return this._getFolderCopyTransaction(data, container, index);
        else { 
          var id = data.folder ? data.folder.id : data.id;
          return this.ptm.moveItem(id, container, index);
        }
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE:
        if (data.id <= 0) 
          return this._getURIItemCopyTransaction(data, container, index);
  
        if (copy) {
          
          
          return this._getBookmarkItemCopyTransaction(data, container, index,
                                                      ["livemark/bookmarkFeedURI"]);
        }
        else
          return this.ptm.moveItem(data.id, container, index);
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR:
        
        
        if (copy)
          return this.ptm.createSeparator(container, index);
        
        return this.ptm.moveItem(data.id, container, index);
        break;
      default:
        if (type == PlacesUtils.TYPE_X_MOZ_URL ||
            type == PlacesUtils.TYPE_UNICODE ||
            type == TAB_DROP_TYPE) {
          var title = (type != PlacesUtils.TYPE_UNICODE) ? data.title :
                                                             data.uri;
          return this.ptm.createItem(PlacesUtils._uri(data.uri),
                                     container, index, title);
        }
    }
    return null;
  },

  








  


































  showAddBookmarkUI: function PU_showAddBookmarkUI(aURI,
                                                   aTitle,
                                                   aDescription,
                                                   aDefaultInsertionPoint,
                                                   aShowPicker,
                                                   aLoadInSidebar,
                                                   aKeyword,
                                                   aPostData,
                                                   aCharSet) {
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

    return this._showBookmarkDialog(info);
  },

  










  showMinimalAddBookmarkUI:
  function PU_showMinimalAddBookmarkUI(aURI, aTitle, aDescription,
                                       aDefaultInsertionPoint, aShowPicker,
                                       aLoadInSidebar, aKeyword, aPostData,
                                       aCharSet) {
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

    this._showBookmarkDialog(info, true);
  },

  





















  showAddLivemarkUI: function PU_showAddLivemarkURI(aFeedURI,
                                                    aSiteURI,
                                                    aTitle,
                                                    aDescription,
                                                    aDefaultInsertionPoint,
                                                    aShowPicker) {
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
    return this._showBookmarkDialog(info);
  },

  







  showMinimalAddLivemarkUI:
  function PU_showMinimalAddLivemarkURI(aFeedURI, aSiteURI, aTitle,
                                        aDescription, aDefaultInsertionPoint,
                                        aShowPicker) {
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
    this._showBookmarkDialog(info, true);
  },

  








  showMinimalAddMultiBookmarkUI: function PU_showAddMultiBookmarkUI(aURIList) {
    NS_ASSERT(aURIList.length,
              "showAddMultiBookmarkUI expects a list of nsIURI objects");
    var info = {
      action: "add",
      type: "folder",
      hiddenRows: ["description"],
      URIList: aURIList
    };
    this._showBookmarkDialog(info, true);
  },

  








  showItemProperties: function PU_showItemProperties(aItemId, aType) {
    var info = {
      action: "edit",
      type: aType,
      itemId: aItemId
    };
    return this._showBookmarkDialog(info);
  },

  













  showAddFolderUI:
  function PU_showAddFolderUI(aTitle, aDefaultInsertionPoint, aShowPicker) {
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
    return this._showBookmarkDialog(info);
  },

  











  _showBookmarkDialog: function PU__showBookmarkDialog(aInfo, aMinimalUI) {
    var dialogURL = aMinimalUI ?
                    "chrome://browser/content/places/bookmarkProperties2.xul" :
                    "chrome://browser/content/places/bookmarkProperties.xul";

    var features;
    if (aMinimalUI)
      features = "centerscreen,chrome,dialog,resizable,modal";
    else
      features = "centerscreen,chrome,modal,resizable=no";
    window.openDialog(dialogURL, "",  features, aInfo);
    return ("performed" in aInfo && aInfo.performed);
  },

  





  getViewForNode: function PU_getViewForNode(aNode) {
    var node = aNode;

    
    
    if (node.localName == "menu" && !node.node &&
        node.firstChild.getAttribute("type") == "places")
      return node.firstChild;

    while (node) {
      
      if (node.getAttribute("type") == "places")
        return node;

      node = node.parentNode;
    }

    return null;
  },

  







  markPageAsTyped: function PU_markPageAsTyped(aURL) {
    PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory)
               .markPageAsTyped(this.createFixedURI(aURL));
  },

  






  markPageAsFollowedBookmark: function PU_markPageAsFollowedBookmark(aURL) {
    PlacesUtils.history.markPageAsFollowedBookmark(this.createFixedURI(aURL));
  },

  







  checkURLSecurity: function PU_checkURLSecurity(aURINode) {
    if (!PlacesUtils.nodeIsBookmark(aURINode)) {
      var uri = PlacesUtils._uri(aURINode.uri);
      if (uri.schemeIs("javascript") || uri.schemeIs("data")) {
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        var errorStr = this.getString("load-js-data-url-error");
        promptService.alert(window, brandShortName, errorStr);
        return false;
      }
    }
    return true;
  },

  







  getDescriptionFromDocument: function PU_getDescriptionFromDocument(doc) {
    var metaElements = doc.getElementsByTagName("META");
    for (var i = 0; i < metaElements.length; ++i) {
      if (metaElements[i].name.toLowerCase() == "description" ||
          metaElements[i].httpEquiv.toLowerCase() == "description") {
        return metaElements[i].content;
      }
    }
    return "";
  },

  






  getItemDescription: function PU_getItemDescription(aItemId) {
    if (PlacesUtils.annotations.itemHasAnnotation(aItemId, DESCRIPTION_ANNO))
      return PlacesUtils.annotations.getItemAnnotation(aItemId, DESCRIPTION_ANNO);
    return "";
  },

  


  _confirmOpenInTabs: function PU__confirmOpenInTabs(numTabsToOpen) {
    var pref = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefBranch);

    const kWarnOnOpenPref = "browser.tabs.warnOnOpen";
    var reallyOpen = true;
    if (pref.getBoolPref(kWarnOnOpenPref)) {
      if (numTabsToOpen >= pref.getIntPref("browser.tabs.maxOpenBeforeWarn")) {
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        
        var warnOnOpen = { value: true };

        var messageKey = "tabs.openWarningMultipleBranded";
        var openKey = "tabs.openButtonMultiple";
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");

        var buttonPressed = promptService.confirmEx(window,
          this.getString("tabs.openWarningTitle"),
          this.getFormattedString(messageKey, [numTabsToOpen, brandShortName]),
          (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0)
           + (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1),
          this.getString(openKey), null, null,
          this.getFormattedString("tabs.openWarningPromptMeBranded",
                                  [brandShortName]), warnOnOpen);

        reallyOpen = (buttonPressed == 0);
        
        if (reallyOpen && !warnOnOpen.value)
          pref.setBoolPref(kWarnOnOpenPref, false);
      }
    }
    return reallyOpen;
  },

  


  _openTabset: function PU__openTabset(aItemsToOpen, aEvent) {
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

    var browserWindow = getTopWin();
    var where = browserWindow ?
                whereToOpenLink(aEvent, false, true) : "window";
    if (where == "window") {
      window.openDialog(getBrowserURL(), "_blank",
                        "chrome,all,dialog=no", urls.join("|"));
      return;
    }

    var loadInBackground = where == "tabshifted" ? true : false;
    var replaceCurrentTab = where == "tab" ? false : true;
    browserWindow.getBrowser().loadTabs(urls, loadInBackground,
                                        replaceCurrentTab);
  },

  openContainerNodeInTabs: function PU_openContainerInTabs(aNode, aEvent) {
    var urlsToOpen = PlacesUtils.getURLsForContainerNode(aNode);
    if (!this._confirmOpenInTabs(urlsToOpen.length))
      return;

    this._openTabset(urlsToOpen, aEvent);
  },

  openURINodesInTabs: function PU_openURINodesInTabs(aNodes, aEvent) {
    var urlsToOpen = [];
    for (var i=0; i < aNodes.length; i++) {
      
      if (PlacesUtils.nodeIsURI(aNodes[i]))
        urlsToOpen.push({uri: aNodes[i].uri, isBookmark: PlacesUtils.nodeIsBookmark(aNodes[i])});
    }
    this._openTabset(urlsToOpen, aEvent);
  },

  









  openNodeWithEvent: function PU_openNodeWithEvent(aNode, aEvent) {
    this.openNodeIn(aNode, whereToOpenLink(aEvent));
  },
  
  




  openNodeIn: function PU_openNodeIn(aNode, aWhere) {
    if (aNode && PlacesUtils.nodeIsURI(aNode) &&
        this.checkURLSecurity(aNode)) {
      var isBookmark = PlacesUtils.nodeIsBookmark(aNode);

      if (isBookmark)
        this.markPageAsFollowedBookmark(aNode.uri);
      else
        this.markPageAsTyped(aNode.uri);

      
      
      if (aWhere == "current" && isBookmark) {
        if (PlacesUtils.annotations
                       .itemHasAnnotation(aNode.itemId, LOAD_IN_SIDEBAR_ANNO)) {
          var w = getTopWin();
          if (w) {
            w.openWebPanel(aNode.title, aNode.uri);
            return;
          }
        }
      }
      openUILinkIn(aNode.uri, aWhere);
    }
  },

  


  createMenuItemForNode:
  function PUU_createMenuItemForNode(aNode, aContainersMap) {
    var element;
    var type = aNode.type;
    if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR)
      element = document.createElement("menuseparator");
    else {
      var iconURI = aNode.icon;
      var iconURISpec = "";
      if (iconURI)
        iconURISpec = iconURI.spec;

      if (PlacesUtils.uriTypes.indexOf(type) != -1) {
        element = document.createElement("menuitem");
        element.className = "menuitem-iconic bookmark-item";

        if (aNode.uri.lastIndexOf("javascript:", 0) == 0)
          element.setAttribute("bookmarklet", "true");
      }
      else if (PlacesUtils.containerTypes.indexOf(type) != -1) {
        element = document.createElement("menu");
        element.setAttribute("container", "true");

        if (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY) {
          element.setAttribute("query", "true");
          if (PlacesUtils.nodeIsTagQuery(aNode))
            element.setAttribute("tagContainer", "true");
          else if (PlacesUtils.nodeIsDay(aNode))
            element.setAttribute("dayContainer", "true");
          else if (PlacesUtils.nodeIsHost(aNode))
            element.setAttribute("hostContainer", "true");
        }
        else if (aNode.itemId != -1) {
          if (PlacesUtils.nodeIsLivemarkContainer(aNode))
            element.setAttribute("livemark", "true");
        }

        var popup = document.createElement("menupopup");
        popup.setAttribute("placespopup", "true");
        popup._resultNode = asContainer(aNode);
#ifdef XP_MACOSX
        
        
        
        
        popup._startMarker = -1;
        popup._endMarker = -1;
#else
        
        popup.setAttribute("context", "placesContext");
#endif
        element.appendChild(popup);
        if (aContainersMap)
          aContainersMap.push({ resultNode: aNode, domNode: popup });
        element.className = "menu-iconic bookmark-item";
      }
      else
        throw "Unexpected node";

      element.setAttribute("label", this.getBestTitle(aNode));

      if (iconURISpec)
        element.setAttribute("image", iconURISpec);
    }
    element.node = aNode;
    element.node.viewIndex = 0;

    return element;
  },

  cleanPlacesPopup: function PU_cleanPlacesPopup(aPopup) {
    
    
    var start = aPopup._startMarker != -1 ? aPopup._startMarker + 1 : 0;
    var end = aPopup._endMarker != -1 ? aPopup._endMarker :
                                        aPopup.childNodes.length;
    var items = [];
    var placesNodeFound = false;
    for (var i = start; i < end; ++i) {
      var item = aPopup.childNodes[i];
      if (item.getAttribute("builder") == "end") {
        
        
        
        aPopup._endMarker = i;
        break;
      }
      if (item.node) {
        items.push(item);
        placesNodeFound = true;
      }
      else {
        
        if (!placesNodeFound)
          
          
          aPopup._startMarker++;
        else {
          
          aPopup._endMarker = i;
          break;
        }
      }
    }

    for (var i = 0; i < items.length; ++i) {
      aPopup.removeChild(items[i]);
      if (aPopup._endMarker != -1)
        aPopup._endMarker--;
    }
  },

  getBestTitle: function PU_getBestTitle(aNode) {
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
    var leftPaneRoot = -1;
    var allBookmarksId;
    var items = PlacesUtils.annotations
                           .getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO, {});
    if (items.length > 1) {
      
      
      items.forEach(function(aItem) {
        PlacesUtils.bookmarks.removeItem(aItem);
      });
    }
    else if (items.length == 1 && items[0] != -1) {
      leftPaneRoot = items[0];
      
      var version = PlacesUtils.annotations
                               .getItemAnnotation(leftPaneRoot, ORGANIZER_FOLDER_ANNO);
      if (version != ORGANIZER_LEFTPANE_VERSION) {
        
        PlacesUtils.bookmarks.removeItem(leftPaneRoot);
        leftPaneRoot = -1;
      }
    }

    if (leftPaneRoot != -1) {
      
      delete this.leftPaneQueries;
      this.leftPaneQueries = {};
      var items = PlacesUtils.annotations
                             .getItemsWithAnnotation(ORGANIZER_QUERY_ANNO, {});
      for (var i=0; i < items.length; i++) {
        var queryName = PlacesUtils.annotations
                                   .getItemAnnotation(items[i], ORGANIZER_QUERY_ANNO);
        this.leftPaneQueries[queryName] = items[i];
      }
      delete this.leftPaneFolderId;
      return this.leftPaneFolderId = leftPaneRoot;
    }

    var self = this;
    const EXPIRE_NEVER = PlacesUtils.annotations.EXPIRE_NEVER;
    var callback = {
      runBatched: function(aUserData) {
        delete self.leftPaneQueries;
        self.leftPaneQueries = { };

        
        leftPaneRoot = PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId, "", -1);
        
        PlacesUtils.bookmarks.setFolderReadonly(leftPaneRoot, true);

        
        let uri = PlacesUtils._uri("place:sort=4&");
        let title = self.getString("OrganizerQueryHistory");
        let itemId = PlacesUtils.bookmarks.insertBookmark(leftPaneRoot, uri, -1, title);
        PlacesUtils.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                                  "History", 0, EXPIRE_NEVER);
        PlacesUtils.annotations.setItemAnnotation(itemId,
                                                  EXCLUDE_FROM_BACKUP_ANNO,
                                                  1, 0, EXPIRE_NEVER);
        self.leftPaneQueries["History"] = itemId;

        

        
        uri = PlacesUtils._uri("place:type=" +
                          Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY +
                          "&sort=" +
                          Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING);
        title = PlacesUtils.bookmarks.getItemTitle(PlacesUtils.tagsFolderId);
        itemId = PlacesUtils.bookmarks.insertBookmark(leftPaneRoot, uri, -1, title);
        PlacesUtils.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                                  "Tags", 0, EXPIRE_NEVER);
        PlacesUtils.annotations.setItemAnnotation(itemId,
                                                  EXCLUDE_FROM_BACKUP_ANNO,
                                                  1, 0, EXPIRE_NEVER);
        self.leftPaneQueries["Tags"] = itemId;

        
        title = self.getString("OrganizerQueryAllBookmarks");
        itemId = PlacesUtils.bookmarks.createFolder(leftPaneRoot, title, -1);
        allBookmarksId = itemId;
        PlacesUtils.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                                  "AllBookmarks", 0, EXPIRE_NEVER);
        PlacesUtils.annotations.setItemAnnotation(itemId,
                                                  EXCLUDE_FROM_BACKUP_ANNO,
                                                  1, 0, EXPIRE_NEVER);
        self.leftPaneQueries["AllBookmarks"] = itemId;

        
        PlacesUtils.bookmarks.setFolderReadonly(allBookmarksId, true);

        
        uri = PlacesUtils._uri("place:folder=TOOLBAR");
        itemId = PlacesUtils.bookmarks.insertBookmark(allBookmarksId, uri, -1, null);
        PlacesUtils.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                                  "BookmarksToolbar", 0, EXPIRE_NEVER);
        PlacesUtils.annotations.setItemAnnotation(itemId,
                                                  EXCLUDE_FROM_BACKUP_ANNO,
                                                  1, 0, EXPIRE_NEVER);
        self.leftPaneQueries["BookmarksToolbar"] = itemId;

        
        uri = PlacesUtils._uri("place:folder=BOOKMARKS_MENU");
        itemId = PlacesUtils.bookmarks.insertBookmark(allBookmarksId, uri, -1, null);
        PlacesUtils.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                                  "BookmarksMenu", 0, EXPIRE_NEVER);
        PlacesUtils.annotations.setItemAnnotation(itemId,
                                                  EXCLUDE_FROM_BACKUP_ANNO,
                                                  1, 0, EXPIRE_NEVER);
        self.leftPaneQueries["BookmarksMenu"] = itemId;

        
        uri = PlacesUtils._uri("place:folder=UNFILED_BOOKMARKS");
        itemId = PlacesUtils.bookmarks.insertBookmark(allBookmarksId, uri, -1, null);
        PlacesUtils.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                                  "UnfiledBookmarks", 0,
                                                  EXPIRE_NEVER);
        PlacesUtils.annotations.setItemAnnotation(itemId,
                                                  EXCLUDE_FROM_BACKUP_ANNO,
                                                  1, 0, EXPIRE_NEVER);
        self.leftPaneQueries["UnfiledBookmarks"] = itemId;

        
        PlacesUtils.bookmarks.setFolderReadonly(leftPaneRoot, true);
      }
    };
    PlacesUtils.bookmarks.runInBatchMode(callback, null);
    PlacesUtils.annotations.setItemAnnotation(leftPaneRoot,
                                              ORGANIZER_FOLDER_ANNO,
                                              ORGANIZER_LEFTPANE_VERSION,
                                              0, EXPIRE_NEVER);
    PlacesUtils.annotations.setItemAnnotation(leftPaneRoot,
                                              EXCLUDE_FROM_BACKUP_ANNO,
                                              1, 0, EXPIRE_NEVER);
    delete this.leftPaneFolderId;
    return this.leftPaneFolderId = leftPaneRoot;
  },

  get allBookmarksFolderId() {
    
    this.leftPaneFolderId;
    delete this.allBookmarksFolderId;
    return this.allBookmarksFolderId = this.leftPaneQueries["AllBookmarks"];
  },

  




  ensureLivemarkStatusMenuItem:
  function PU_ensureLivemarkStatusMenuItem(aPopup) {
    var itemId = aPopup._resultNode.itemId;

    var lmStatus = null;
    if (PlacesUtils.annotations
                   .itemHasAnnotation(itemId, "livemark/loadfailed"))
      lmStatus = "bookmarksLivemarkFailed";
    else if (PlacesUtils.annotations
                        .itemHasAnnotation(itemId, "livemark/loading"))
      lmStatus = "bookmarksLivemarkLoading";

    if (lmStatus && !aPopup._lmStatusMenuItem) {
      
      aPopup._lmStatusMenuItem = document.createElement("menuitem");
      aPopup._lmStatusMenuItem.setAttribute("lmStatus", lmStatus);
      aPopup._lmStatusMenuItem.setAttribute("label", this.getString(lmStatus));
      aPopup._lmStatusMenuItem.setAttribute("disabled", true);
      aPopup.insertBefore(aPopup._lmStatusMenuItem,
                          aPopup.childNodes[aPopup._startMarker + 1]);
      aPopup._startMarker++;
    }
    else if (lmStatus &&
             aPopup._lmStatusMenuItem.getAttribute("lmStatus") != lmStatus) {
      
      aPopup._lmStatusMenuItem.setAttribute("label",
                                            this.getString(lmStatus));
    }
    else if (!lmStatus && aPopup._lmStatusMenuItem){
      
      aPopup.removeChild(aPopup._lmStatusMenuItem);
      aPopup._lmStatusMenuItem = null;
      aPopup._startMarker--;
    }
  }
};
