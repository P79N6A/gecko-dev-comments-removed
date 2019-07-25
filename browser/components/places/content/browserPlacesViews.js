









































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function PlacesViewBase(aPlace) {
  this.place = aPlace;
  this._controller = new PlacesController(this);
  this._viewElt.controllers.appendController(this._controller);
}

PlacesViewBase.prototype = {
  
  _viewElt: null,
  get viewElt() this._viewElt,

  get controllers() this._viewElt.controllers,

  
  _rootElt: null,

  
  
  _nativeView: false,

  QueryInterface: XPCOMUtils.generateQI(
    [Components.interfaces.nsINavHistoryResultObserver,
     Components.interfaces.nsISupportsWeakReference]),

  _place: "",
  get place() this._place,
  set place(val) {
    this._place = val;

    let history = PlacesUtils.history;
    let queries = { }, options = { };
    history.queryStringToQueries(val, queries, { }, options);
    if (!queries.value.length)
      queries.value = [history.getNewQuery()];

    let result = history.executeQueries(queries.value, queries.value.length,
                                        options.value);
    result.addObserver(this, false);
    return val;
  },

  _result: null,
  get result() this._result,
  set result(val) {
    if (this._result == val)
      return;

    if (this._result) {
      this._result.removeObserver(this);
      this._resultNode.containerOpen = false;
    }

    if (this._rootElt.localName == "menupopup")
      this._rootElt._built = false;

    this._result = val;
    if (val) {
      this._resultNode = val.root;
      this._rootElt._placesNode = this._resultNode;
      this._resultNode._DOMElement = this._rootElt;

      
      this._resultNode.containerOpen = true;
    }
    else {
      this._resultNode = null;
    }

    return val;
  },

  get controller() this._controller,

  get selType() "single",
  selectItems: function() { },
  selectAll: function() { },

  get selectedNode() {
    if (this._contextMenuShown) {
      let popup = document.popupNode;
      return popup._placesNode || popup.parentNode._placesNode || null;
    }
    return null;
  },

  get hasSelection() this.selectedNode != null,

  get selectedNodes() {
    let selectedNode = this.selectedNode;
    return selectedNode ? [selectedNode] : [];
  },

  get removableSelectionRanges() {
    
    
    
    if (document.popupNode &&
        (document.popupNode == "menupopup" || !document.popupNode._placesNode))
      return [];

    return [this.selectedNodes];
  },

  get draggableSelection() [this._draggedElt],

  get insertionPoint() {
    
    
    let resultNode = this._resultNode;
    if (PlacesUtils.nodeIsQuery(resultNode) &&
        PlacesUtils.asQuery(resultNode).queryOptions.queryType ==
          Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY)
      return null;

    
    let index = PlacesUtils.bookmarks.DEFAULT_INDEX;
    let container = this._resultNode;
    let orientation = Ci.nsITreeView.DROP_BEFORE;
    let isTag = false;

    let selectedNode = this.selectedNode;
    if (selectedNode) {
      let popup = document.popupNode;
      if (!popup._placesNode || popup._placesNode == this._resultNode) {
        
        
        container = selectedNode;
        orientation = Ci.nsITreeView.DROP_ON;
      }
      else {
        
        container = selectedNode.parent;
        index = container.getChildIndex(selectedNode);
        isTag = PlacesUtils.nodeIsTagQuery(container);
      }
    }

    if (PlacesControllerDragHelper.disallowInsertion(container))
      return null;

    return new InsertionPoint(PlacesUtils.getConcreteItemId(container),
                              index, orientation, isTag);
  },

  buildContextMenu: function PVB_buildContextMenu(aPopup) {
    this._contextMenuShown = true;
    window.updateCommands("places");
    return this.controller.buildContextMenu(aPopup);
  },

  destroyContextMenu: function PVB_destroyContextMenu(aPopup) {
    this._contextMenuShown = false;
    if (window.content)
      window.content.focus();
  },

  _cleanPopup: function PVB_cleanPopup(aPopup) {
    
    
    let start = aPopup._startMarker != -1 ? aPopup._startMarker + 1 : 0;
    let end = aPopup._endMarker != -1 ? aPopup._endMarker :
                                        aPopup.childNodes.length;
    let items = [];
    let placesNodeFound = false;
    for (let i = start; i < end; ++i) {
      let item = aPopup.childNodes[i];
      if (item.getAttribute("builder") == "end") {
        
        
        
        aPopup._endMarker = i;
        break;
      }
      if (item._placesNode) {
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

    for (let i = 0; i < items.length; ++i) {
      aPopup.removeChild(items[i]);
      if (aPopup._endMarker != -1)
        aPopup._endMarker--;
    }
  },

  _rebuildPopup: function PVB__rebuildPopup(aPopup) {
    this._cleanPopup(aPopup);

    
    
    if (PlacesUtils.nodeIsLivemarkContainer(aPopup._placesNode))
      this._ensureLivemarkStatusMenuItem(aPopup);

    let resultNode = aPopup._placesNode;
    if (!resultNode.containerOpen)
      return;

    let cc = resultNode.childCount;
    if (cc > 0) {
      aPopup.removeAttribute("emptyplacesresult");
      if (aPopup._emptyMenuItem)
        aPopup._emptyMenuItem.hidden = true;

      for (let i = 0; i < cc; ++i) {
        let child = resultNode.getChild(i);
        this._insertNewItemToPopup(child, aPopup, null);
      }
    }
    else {
      aPopup.setAttribute("emptyplacesresult", "true");
      
      
      if (aPopup._startMarker == -1 && aPopup._endMarker == -1)
        this._showEmptyMenuItem(aPopup);
    }
    aPopup._built = true;
  },

  _removeChild: function PVB__removeChild(aChild) {
    
    
    
    if (document.popupNode == aChild)
      document.popupNode = null;

    aChild.parentNode.removeChild(aChild);
  },

  _showEmptyMenuItem: function PVB__showEmptyMenuItem(aPopup) {
    if (aPopup._emptyMenuItem) {
      aPopup._emptyMenuItem.hidden = false;
      return;
    }

    let label = PlacesUIUtils.getString("bookmarksMenuEmptyFolder");
    aPopup._emptyMenuItem = document.createElement("menuitem");
    aPopup._emptyMenuItem.setAttribute("label", label);
    aPopup._emptyMenuItem.setAttribute("disabled", true);
    aPopup.appendChild(aPopup._emptyMenuItem);
  },

  _createMenuItemForPlacesNode:
  function PVB__createMenuItemForPlacesNode(aPlacesNode) {
    let element;
    let type = aPlacesNode.type;
    if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR)
      element = document.createElement("menuseparator");
    else {
      if (PlacesUtils.uriTypes.indexOf(type) != -1) {
        element = document.createElement("menuitem");
        element.className = "menuitem-iconic bookmark-item menuitem-with-favicon";
        element.setAttribute("scheme",
                             PlacesUIUtils.guessUrlSchemeForUI(aPlacesNode.uri));
      }
      else if (PlacesUtils.containerTypes.indexOf(type) != -1) {
        element = document.createElement("menu");
        element.setAttribute("container", "true");

        if (aPlacesNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY) {
          element.setAttribute("query", "true");
          if (PlacesUtils.nodeIsTagQuery(aPlacesNode))
            element.setAttribute("tagContainer", "true");
          else if (PlacesUtils.nodeIsDay(aPlacesNode))
            element.setAttribute("dayContainer", "true");
          else if (PlacesUtils.nodeIsHost(aPlacesNode))
            element.setAttribute("hostContainer", "true");
        }
        else if (aPlacesNode.itemId != -1) {
          if (PlacesUtils.nodeIsLivemarkContainer(aPlacesNode))
            element.setAttribute("livemark", "true");
        }

        let popup = document.createElement("menupopup");
        popup._placesNode = PlacesUtils.asContainer(aPlacesNode);
        if (this._nativeView) {
          popup._startMarker = -1;
          popup._endMarker = -1;
        }
        else
          popup.setAttribute("placespopup", "true");
#ifdef XP_MACOSX
        
        popup.setAttribute("context", "placesContext");
#endif
        element.appendChild(popup);
        element.className = "menu-iconic bookmark-item";

        aPlacesNode._DOMElement = popup;
      }
      else
        throw "Unexpected node";

      element.setAttribute("label", PlacesUIUtils.getBestTitle(aPlacesNode));

      let icon = aPlacesNode.icon;
      if (icon)
        element.setAttribute("image", icon);
    }

    element._placesNode = aPlacesNode;
    if (!aPlacesNode._DOMElement)
      aPlacesNode._DOMElement = element;

    return element;
  },

  _insertNewItemToPopup:
  function PVB__insertNewItemToPopup(aNewChild, aPopup, aBefore) {
    let element = this._createMenuItemForPlacesNode(aNewChild);

    if (aBefore) {
      aPopup.insertBefore(element, aBefore);
    }
    else {
      
      
      
      if (aPopup._endMarker != -1) {
        let lastElt = aPopup.childNodes[aPopup._endMarker];
        aPopup.insertBefore(element, lastElt);
      }
      else {
        aPopup.appendChild(element);
      }
    }

    if (aPopup._endMarker != -1)
      aPopup._endMarker++;

    return element;
  },

  




  _ensureLivemarkStatusMenuItem:
  function PVB_ensureLivemarkStatusMenuItem(aPopup) {
    let itemId = aPopup._placesNode.itemId;
    let as = PlacesUtils.annotations;

    let lmStatus = null;
    if (as.itemHasAnnotation(itemId, "livemark/loadfailed"))
      lmStatus = "bookmarksLivemarkFailed";
    else if (as.itemHasAnnotation(itemId, "livemark/loading"))
      lmStatus = "bookmarksLivemarkLoading";

    let lmStatusElt = aPopup._lmStatusMenuItem;
    if (lmStatus && !lmStatusElt) {
      
      lmStatusElt = document.createElement("menuitem");
      lmStatusElt.setAttribute("lmStatus", lmStatus);
      lmStatusElt.setAttribute("label", PlacesUIUtils.getString(lmStatus));
      lmStatusElt.setAttribute("disabled", true);
      aPopup.insertBefore(lmStatusElt,
                          aPopup.childNodes.item(aPopup._startMarker + 1));
      aPopup._lmStatusMenuItem = lmStatusElt;
      aPopup._startMarker++;
    }
    else if (lmStatus && lmStatusElt.getAttribute("lmStatus") != lmStatus) {
      
      lmStatusElt.setAttribute("label", this.getString(lmStatus));
    }
    else if (!lmStatus && lmStatusElt) {
      
      aPopup.removeChild(aPopup._lmStatusMenuItem);
      aPopup._lmStatusMenuItem = null;
      aPopup._startMarker--;
    }
  },

  nodeURIChanged: function PVB_nodeURIChanged(aPlacesNode, aURIString) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    elt.setAttribute("scheme", PlacesUIUtils.guessUrlSchemeForUI(aURIString));
  },

  nodeIconChanged: function PVB_nodeIconChanged(aPlacesNode) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    
    if (elt == this._rootElt)
      return;

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    let icon = aPlacesNode.icon;
    if (!icon)
      elt.removeAttribute("image");
    else if (icon != elt.getAttribute("image"))
      elt.setAttribute("image", icon);
  },

  nodeAnnotationChanged:
  function PVB_nodeAnnotationChanged(aPlacesNode, aAnno) {
    
    if (aAnno == PlacesUtils.LMANNO_FEEDURI) {
      let elt = aPlacesNode._DOMElement;
      if (!elt)
        throw "aPlacesNode must have _DOMElement set";

      let menu = elt.parentNode;
      if (!menu.hasAttribute("livemark"))
        menu.setAttribute("livemark", "true");

      
      this._ensureLivemarkStatusMenuItem(elt);
    }
  },

  nodeTitleChanged:
  function PVB_nodeTitleChanged(aPlacesNode, aNewTitle) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    
    if (elt == this._rootElt)
      return;

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (!aNewTitle && elt.localName != "toolbarbutton") {
      
      
      
      elt.setAttribute("label", PlacesUIUtils.getBestTitle(aPlacesNode));
    }
    else {
      elt.setAttribute("label", aNewTitle);
    }
  },

  nodeRemoved:
  function PVB_nodeRemoved(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = aParentPlacesNode._DOMElement;
    let elt = aPlacesNode._DOMElement;

    if (!parentElt)
      throw "aParentPlacesNode must have _DOMElement set";
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (parentElt._built) {
      parentElt.removeChild(elt);

      
      
      
      if (!parentElt.hasChildNodes() ||
          (parentElt.childNodes.length == 1 &&
          parentElt.firstChild == parentElt._emptyMenuItem))
        this._showEmptyMenuItem(parentElt);

      if (parentElt._endMarker != -1)
        parentElt._endMarker--;
    }
  },

  nodeReplaced:
  function PVB_nodeReplaced(aParentPlacesNode, aOldPlacesNode, aNewPlacesNode, aIndex) {
    let parentElt = aParentPlacesNode._DOMElement;
    if (!parentElt)
      throw "aParentPlacesNode node must have _DOMElement set";

    if (parentElt._built) {
      let elt = aOldPlacesNode._DOMElement;
      if (!elt)
        throw "aOldPlacesNode must have _DOMElement set";

      
      if (elt.localName == "menupopup")
        elt = elt.parentNode;

      parentElt.removeChild(elt);

      
      
      
      let nextElt = elt.nextSibling;
      this._insertNewItemToPopup(aNewPlacesNode, parentElt, nextElt);
    }
  },

  nodeHistoryDetailsChanged: function() { },
  nodeTagsChanged: function() { },
  nodeDateAddedChanged: function() { },
  nodeLastModifiedChanged: function() { },
  nodeKeywordChanged: function() { },
  sortingChanged: function() { },
  batching: function() { },
  
  containerOpened: function() { },
  containerClosed: function() { },

  nodeInserted:
  function PVB_nodeInserted(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = aParentPlacesNode._DOMElement;
    if (!parentElt)
      throw "aParentPlacesNode node must have _DOMElement set";

    if (!parentElt._built)
      return;

    let index = parentElt._startMarker + 1 + aIndex;
    this._insertNewItemToPopup(aPlacesNode, parentElt,
                               parentElt.childNodes[index]);
    if (parentElt._emptyMenuItem)
      parentElt._emptyMenuItem.hidden = true;
  },

  nodeMoved:
  function PBV_nodeMoved(aPlacesNode,
                         aOldParentPlacesNode, aOldIndex,
                         aNewParentPlacesNode, aNewIndex) {
    
    
    
    
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    
    
    if (elt == this._rootElt)
      return;

    let parentElt = aNewParentPlacesNode._DOMElement;
    if (!parentElt)
      throw "aNewParentPlacesNode node must have _DOMElement set";

    if (parentElt._built) {
      
      parentElt.removeChild(elt);
      let index = parentElt._startMarker + 1 + aNewIndex;
      parentElt.insertBefore(elt, parentElt.childNodes[index]);
    }
  },

  containerStateChanged:
  function PVB_containerStateChanged(aPlacesNode, aOldState, aNewState) {
    if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_OPENED ||
        aNewState == Ci.nsINavHistoryContainerResultNode.STATE_CLOSED) {
      this.invalidateContainer(aPlacesNode);
    }
    else {
      throw "Unexpected state passed to containerStateChanged";
    }
  },

  invalidateContainer: function PVB_invalidateContainer(aPlacesNode) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    elt._built = false;

    
    if (elt.parentNode.open)
      this._rebuildPopup(elt);
  },

  uninit: function PVB_uninit() {
    if (this._result) {
      this._result.removeObserver(this);
      this._resultNode.containerOpen = false;
      this._resultNode = null;
      this._result = null;
    }

    delete this._viewElt._placesView;
  },

  get isRTL() {
    if ("_isRTL" in this)
      return this._isRTL;

    return this._isRTL = document.defaultView
                                 .getComputedStyle(this.viewElt, "")
                                 .direction == "rtl";
  },

  




  _mayAddCommandsItems: function PVB__mayAddCommandsItems(aPopup) {
    
    if (aPopup == this._rootElt)
      return;

    
    let numURINodes = 0;
    let currentChild = aPopup.firstChild;
    while (currentChild) {
      if (currentChild.localName == "menuitem" && currentChild._placesNode) {
        if (++numURINodes == 2)
          break;
      }
      currentChild = currentChild.nextSibling;
    }

    let hasMultipleURIs = numURINodes > 1;
    let itemId = aPopup._placesNode.itemId;
    let siteURIString = "";
    if (itemId != -1 && PlacesUtils.itemIsLivemark(itemId)) {
      let siteURI = PlacesUtils.livemarks.getSiteURI(itemId);
      if (siteURI)
        siteURIString = siteURI.spec;
    }

    if (!siteURIString && aPopup._endOptOpenSiteURI) {
      aPopup.removeChild(aPopup._endOptOpenSiteURI);
      aPopup._endOptOpenSiteURI = null;
    }

    if (!hasMultipleURIs && aPopup._endOptOpenAllInTabs) {
      aPopup.removeChild(aPopup._endOptOpenAllInTabs);
      aPopup._endOptOpenAllInTabs = null;
    }

    if (!(hasMultipleURIs || siteURIString)) {
      
      if (aPopup._endOptSeparator) {
        aPopup.removeChild(aPopup._endOptSeparator);
        aPopup._endOptSeparator = null;
        aPopup._endMarker = -1;
      }
      return;
    }

    if (!aPopup._endOptSeparator) {
      
      aPopup._endOptSeparator = document.createElement("menuseparator");
      aPopup._endOptSeparator.className = "bookmarks-actions-menuseparator";
      aPopup._endMarker = aPopup.childNodes.length;
      aPopup.appendChild(aPopup._endOptSeparator);
    }

    if (siteURIString && !aPopup._endOptOpenSiteURI) {
      
      aPopup._endOptOpenSiteURI = document.createElement("menuitem");
      aPopup._endOptOpenSiteURI.className = "openlivemarksite-menuitem";
      aPopup._endOptOpenSiteURI.setAttribute("targetURI", siteURIString);
      aPopup._endOptOpenSiteURI.setAttribute("oncommand",
          "openUILink(this.getAttribute('targetURI'), event);");

      
      
      
      
      aPopup._endOptOpenSiteURI.setAttribute("onclick",
          "checkForMiddleClick(this, event); event.stopPropagation();");
      aPopup._endOptOpenSiteURI.setAttribute("label",
          PlacesUIUtils.getFormattedString("menuOpenLivemarkOrigin.label",
          [aPopup.parentNode.getAttribute("label")]));
      aPopup.appendChild(aPopup._endOptOpenSiteURI);
    }

    if (hasMultipleURIs && !aPopup._endOptOpenAllInTabs) {
      
      
      aPopup._endOptOpenAllInTabs = document.createElement("menuitem");
      aPopup._endOptOpenAllInTabs.className = "openintabs-menuitem";
      aPopup._endOptOpenAllInTabs.setAttribute("oncommand",
        "PlacesUIUtils.openContainerNodeInTabs(this.parentNode._placesNode, event, " +
                                               "PlacesUIUtils.getViewForNode(this));");
      aPopup._endOptOpenAllInTabs.setAttribute("onclick",
        "checkForMiddleClick(this, event); event.stopPropagation();");
      aPopup._endOptOpenAllInTabs.setAttribute("label",
        gNavigatorBundle.getString("menuOpenAllInTabs.label"));
      aPopup.appendChild(aPopup._endOptOpenAllInTabs);
    }
  },

  _onPopupShowing: function PVB__onPopupShowing(aEvent) {
    
    let popup = aEvent.originalTarget;
    if (popup._placesNode && PlacesUIUtils.getViewForNode(popup) == this) {
      if (!popup._placesNode.containerOpen)
        popup._placesNode.containerOpen = true;
      if (!popup._built)
        this._rebuildPopup(popup);

      this._mayAddCommandsItems(popup);
    }
  },

  _addEventListeners:
  function PVB__addEventListeners(aObject, aEventNames, aCapturing) {
    for (let i = 0; i < aEventNames.length; i++) {
      aObject.addEventListener(aEventNames[i], this, aCapturing);
    }
  },

  _removeEventListeners:
  function PVB__removeEventListeners(aObject, aEventNames, aCapturing) {
    for (let i = 0; i < aEventNames.length; i++) {
      aObject.removeEventListener(aEventNames[i], this, aCapturing);
    }
  },
};

function PlacesToolbar(aPlace) {
  
  let thisView = this;
  [
    ["_viewElt",              "PlacesToolbar"],
    ["_rootElt",              "PlacesToolbarItems"],
    ["_dropIndicator",        "PlacesToolbarDropIndicator"],
    ["_chevron",              "PlacesChevron"],
    ["_chevronPopup",         "PlacesChevronPopup"]
  ].forEach(function (elementGlobal) {
    let [name, id] = elementGlobal;
    thisView.__defineGetter__(name, function () {
      let element = document.getElementById(id);
      if (!element)
        return null;

      delete thisView[name];
      return thisView[name] = element;
    });
  });

  this._viewElt._placesView = this;

  this._addEventListeners(this._viewElt, this._cbEvents, false);
  this._addEventListeners(this._rootElt, ["popupshowing", "popuphidden"], true);
  this._addEventListeners(this._rootElt, ["overflow", "underflow"], true);
  this._addEventListeners(window, ["resize", "unload"], false);

  PlacesViewBase.call(this, aPlace);
}

PlacesToolbar.prototype = {
  __proto__: PlacesViewBase.prototype,

  _cbEvents: ["dragstart", "dragover", "dragexit", "dragend", "drop",
#ifdef XP_UNIX
#ifndef XP_MACOSX
              "mousedown", "mouseup",
#endif
#endif
              "mousemove", "mouseover", "mouseout"],

  QueryInterface: function PT_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIDOMEventListener) ||
        aIID.equals(Ci.nsITimerCallback))
      return this;

    return PlacesViewBase.prototype.QueryInterface.apply(this, arguments);
  },

  uninit: function PT_uninit() {
    this._removeEventListeners(this._viewElt, this._cbEvents, false);
    this._removeEventListeners(this._rootElt, ["popupshowing", "popuphidden"],
                               true);
    this._removeEventListeners(this._rootElt, ["overflow", "underflow"], true);
    this._removeEventListeners(window, ["resize", "unload"], false);

    PlacesViewBase.prototype.uninit.apply(this, arguments);
  },

  _openedMenuButton: null,
  _allowPopupShowing: true,

  _rebuild: function PT__rebuild() {
    
    
    if (this._overFolder.elt)
      this._clearOverFolder();

    this._openedMenuButton = null;
    while (this._rootElt.hasChildNodes()) {
      this._rootElt.removeChild(this._rootElt.firstChild);
    }

    let cc = this._resultNode.childCount;
    for (let i = 0; i < cc; ++i) {
      this._insertNewItem(this._resultNode.getChild(i), null);
    }

    if (this._chevronPopup.hasAttribute("type")) {
      
      
      
      this._chevronPopup.place = this.place;
    }
  },

  _insertNewItem:
  function PT__insertNewItem(aChild, aBefore) {
    let type = aChild.type;
    let button;
    if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
      button = document.createElement("toolbarseparator");
    }
    else {
      button = document.createElement("toolbarbutton");
      button.className = "bookmark-item";
      button.setAttribute("label", aChild.title);
      let icon = aChild.icon;
      if (icon)
        button.setAttribute("image", icon);

      if (PlacesUtils.containerTypes.indexOf(type) != -1) {
        button.setAttribute("type", "menu");
        button.setAttribute("container", "true");

        if (PlacesUtils.nodeIsQuery(aChild)) {
          button.setAttribute("query", "true");
          if (PlacesUtils.nodeIsTagQuery(aChild))
            button.setAttribute("tagContainer", "true");
        }
        else if (PlacesUtils.nodeIsLivemarkContainer(aChild)) {
          button.setAttribute("livemark", "true");
        }

        let popup = document.createElement("menupopup");
        popup.setAttribute("placespopup", "true");
        button.appendChild(popup);
        popup._placesNode = PlacesUtils.asContainer(aChild);
#ifndef XP_MACOSX
        popup.setAttribute("context", "placesContext");
#endif

        aChild._DOMElement = popup;
      }
      else if (PlacesUtils.nodeIsURI(aChild)) {
        button.setAttribute("scheme",
                            PlacesUIUtils.guessUrlSchemeForUI(aChild.uri));
      }
    }

    button._placesNode = aChild;
    if (!aChild._DOMElement)
      aChild._DOMElement = button;

    if (aBefore)
      this._rootElt.insertBefore(button, aBefore);
    else
      this._rootElt.appendChild(button);
  },

  _updateChevronPopupNodesVisibility:
  function PT__updateChevronPopupNodesVisibility() {
    for (let i = 0; i < this._chevronPopup.childNodes.length; i++) {
      this._chevronPopup.childNodes[i].hidden =
        this._rootElt.childNodes[i].style.visibility != "hidden";
    }
  },

  _onChevronPopupShowing:
  function PT__onChevronPopupShowing(aEvent) {
    
    if (aEvent.target != this._chevronPopup)
      return;

    if (!this._chevron._placesView)
      this._chevron._placesView = new PlacesMenu(aEvent, this.place);

    this._updateChevronPopupNodesVisibility();
  },

  handleEvent: function PT_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "unload":
        this.uninit();
        break;
      case "resize":
        
        
        
        this.updateChevron();
        break;
      case "overflow":
        if (aEvent.target != aEvent.currentTarget)
          return;

        
        if (aEvent.detail == 0)
          return;

        
        
        if (!this._chevronPopup.hasAttribute("type")) {
          this._chevronPopup.setAttribute("place", this.place);
          this._chevronPopup.setAttribute("type", "places");
        }
        this._chevron.collapsed = false;
        this.updateChevron();
        break;
      case "underflow":
        if (aEvent.target != aEvent.currentTarget)
          return;

        
        if (aEvent.detail == 0)
          return;

        this._chevron.collapsed = true;
        this.updateChevron();
        break;
      case "dragstart":
        this._onDragStart(aEvent);
        break;
      case "dragover":
        this._onDragOver(aEvent);
        break;
      case "dragexit":
        this._onDragExit(aEvent);
        break;
      case "dragend":
        this._onDragEnd(aEvent);
        break;
      case "drop":
        this._onDrop(aEvent);
        break;
      case "mouseover":
        this._onMouseOver(aEvent);
        break;
      case "mousemove":
        this._onMouseMove(aEvent);
        break;
      case "mouseout":
        this._onMouseOut(aEvent);
        break;
#ifdef XP_UNIX
#ifndef XP_MACOSX
      case "mouseup":
        this._onMouseUp(aEvent);
        break;
      case "mousedown":
        this._onMouseDown(aEvent);
        break;
#endif
#endif
      case "popupshowing":
        this._onPopupShowing(aEvent);
        break;
      case "popuphidden":
        this._onPopupHidden(aEvent);
        break;
      default:
        throw "Trying to handle unexpected event.";
    }
  },

  updateChevron: function PT_updateChevron() {
    
    if (this._chevron.collapsed)
      return;

    
    
    if (this._updateChevronTimer)
      this._updateChevronTimer.cancel();

    this._updateChevronTimer = this._setTimer(100);
  },

  _updateChevronTimerCallback: function PT__updateChevronTimerCallback() {
    let scrollRect = this._rootElt.getBoundingClientRect();
    let childOverflowed = false;
    for (let i = 0; i < this._rootElt.childNodes.length; i++) {
      let child = this._rootElt.childNodes[i];
      
      if (!childOverflowed) {
        let childRect = child.getBoundingClientRect();
        childOverflowed = this.isRTL ? (childRect.left < scrollRect.left)
                                     : (childRect.right > scrollRect.right);
                                      
      }
      child.style.visibility = childOverflowed ? "hidden" : "visible";
    }

    
    
    if (this._chevron.open)
      this._updateChevronPopupNodesVisibility();
  },

  nodeInserted:
  function PT_nodeInserted(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = aParentPlacesNode._DOMElement;
    if (!parentElt) 
      throw "aParentPlacesNode node must have _DOMElement set";

    if (parentElt == this._rootElt) {
      let children = this._rootElt.childNodes;
      this._insertNewItem(aPlacesNode,
        aIndex < children.length ? children[aIndex] : null);
      this.updateChevron();
      return;
    }

    PlacesViewBase.prototype.nodeInserted.apply(this, arguments);
  },

  nodeRemoved:
  function PT_nodeRemoved(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = aParentPlacesNode._DOMElement;
    let elt = aPlacesNode._DOMElement;

    if (!parentElt)
      throw "aParentPlacesNode node must have _DOMElement set";
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (parentElt == this._rootElt) {
      this._removeChild(elt);
      this.updateChevron();
      return;
    }

    PlacesViewBase.prototype.nodeRemoved.apply(this, arguments);
  },

  nodeMoved:
  function PT_nodeMoved(aPlacesNode,
                        aOldParentPlacesNode, aOldIndex,
                        aNewParentPlacesNode, aNewIndex) {
    let parentElt = aNewParentPlacesNode._DOMElement;
    if (!parentElt) 
      throw "aNewParentPlacesNode node must have _DOMElement set";

    if (parentElt == this._rootElt) {
      

      
      let elt = aPlacesNode._DOMElement;
      if (!elt)
        throw "aPlacesNode must have _DOMElement set";

      
      if (elt.localName == "menupopup")
        elt = elt.parentNode;

      this._removeChild(elt);
      this._rootElt.insertBefore(elt, this._rootElt.childNodes[aNewIndex]);

      
      if (this._chevron.open) {
        let chevronPopup = this._chevronPopup;
        let menuitem = chevronPopup.childNodes[aOldIndex];
        chevronPopup.removeChild(menuitem);
        chevronPopup.insertBefore(menuitem,
                                  chevronPopup.childNodes[aNewIndex]);
      }
      this.updateChevron();
      return;
    }

    PlacesViewBase.prototype.nodeMoved.apply(this, arguments);
  },

  nodeAnnotationChanged:
  function PT_nodeAnnotationChanged(aPlacesNode, aAnno) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    if (elt == this._rootElt)
      return;

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (elt.parentNode == this._rootElt) {
      

      
      if (aAnno == PlacesUtils.LMANNO_FEEDURI) {
        elt.setAttribute("livemark", true);
      }
      return;
    }

    PlacesViewBase.prototype.nodeAnnotationChanged.apply(this, arguments);
  },

  nodeTitleChanged: function PT_nodeTitleChanged(aPlacesNode, aNewTitle) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    
    
    if (elt == this._rootElt)
      return;

    PlacesViewBase.prototype.nodeTitleChanged.apply(this, arguments);

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (elt.parentNode == this._rootElt) {
      
      this.updateChevron();
    }
  },

  nodeReplaced:
  function PT_nodeReplaced(aParentPlacesNode,
                           aOldPlacesNode, aNewPlacesNode, aIndex) {
    let parentElt = aParentPlacesNode._DOMElement;
    if (!parentElt) 
      throw "aParentPlacesNode node must have _DOMElement set";

    if (parentElt == this._rootElt) {
      let elt = aOldPlacesNode._DOMElement;
      if (!elt)
        throw "aOldPlacesNode must have _DOMElement set";

      
      if (elt.localName == "menupopup")
        elt = elt.parentNode;

      this._removeChild(elt);

      
      
      
      let next = elt.nextSibling;
      this._insertNewItem(aNewPlacesNode, next);
      this.updateChevron();
      return;
    }

    PlacesViewBase.prototype.nodeReplaced.apply(this, arguments);
  },

  invalidateContainer: function PT_invalidateContainer(aPlacesNode) {
    let elt = aPlacesNode._DOMElement;
    if (!elt)
      throw "aPlacesNode must have _DOMElement set";

    if (elt == this._rootElt) {
      
      this._rebuild();
      return;
    }

    PlacesViewBase.prototype.invalidateContainer.apply(this, arguments);
  },

  _overFolder: { elt: null,
                 openTimer: null,
                 hoverTime: 350,
                 closeTimer: null },

  _clearOverFolder: function PT__clearOverFolder() {
    
    
    
    if (this._overFolder.elt && this._overFolder.elt.lastChild) {
      if (!this._overFolder.elt.lastChild.hasAttribute("dragover")) {
        this._overFolder.elt.lastChild.hidePopup();
      }
      this._overFolder.elt.removeAttribute("dragover");
      this._overFolder.elt = null;
    }
    if (this._overFolder.openTimer) {
      this._overFolder.openTimer.cancel();
      this._overFolder.openTimer = null;
    }
    if (this._overFolder.closeTimer) {
      this._overFolder.closeTimer.cancel();
      this._overFolder.closeTimer = null;
    }
  },

  






  _getDropPoint: function PT__getDropPoint(aEvent) {
    let result = this.result;
    if (!PlacesUtils.nodeIsFolder(this._resultNode))
      return null;

    let dropPoint = { ip: null, beforeIndex: null, folderElt: null };
    let elt = aEvent.target;
    if (elt._placesNode && elt != this._rootElt &&
        elt.localName != "menupopup") {
      let eltRect = elt.getBoundingClientRect();
      let eltIndex = Array.indexOf(this._rootElt.childNodes, elt);
      if (PlacesUtils.nodeIsFolder(elt._placesNode) &&
          !PlacesUtils.nodeIsReadOnly(elt._placesNode)) {
        
        
        
        let threshold = eltRect.width * 0.25;
        if (this.isRTL ? (aEvent.clientX > eltRect.right - threshold)
                       : (aEvent.clientX < eltRect.left + threshold)) {
          
          dropPoint.ip =
            new InsertionPoint(PlacesUtils.getConcreteItemId(this._resultNode),
                               eltIndex, Ci.nsITreeView.DROP_BEFORE);
          dropPoint.beforeIndex = eltIndex;
        }
        else if (this.isRTL ? (aEvent.clientX > eltRect.left + threshold)
                            : (aEvent.clientX < eltRect.right - threshold)) {
          
          dropPoint.ip =
            new InsertionPoint(PlacesUtils.getConcreteItemId(elt._placesNode),
                               -1, Ci.nsITreeView.DROP_ON,
                               PlacesUtils.nodeIsTagQuery(elt._placesNode));
          dropPoint.beforeIndex = eltIndex;
          dropPoint.folderElt = elt;
        }
        else {
          
          let beforeIndex =
            (eltIndex == this._rootElt.childNodes.length - 1) ?
            -1 : eltIndex + 1;

          dropPoint.ip =
            new InsertionPoint(PlacesUtils.getConcreteItemId(this._resultNode),
                               beforeIndex, Ci.nsITreeView.DROP_BEFORE);
          dropPoint.beforeIndex = beforeIndex;
        }
      }
      else {
        
        
        let threshold = eltRect.width * 0.5;
        if (this.isRTL ? (aEvent.clientX > eltRect.left + threshold)
                       : (aEvent.clientX < eltRect.left + threshold)) {
          
          dropPoint.ip =
            new InsertionPoint(PlacesUtils.getConcreteItemId(this._resultNode),
                               eltIndex, Ci.nsITreeView.DROP_BEFORE);
          dropPoint.beforeIndex = eltIndex;
        }
        else {
          
          let beforeIndex =
            eltIndex == this._rootElt.childNodes.length - 1 ?
            -1 : eltIndex + 1;
          dropPoint.ip =
            new InsertionPoint(PlacesUtils.getConcreteItemId(this._resultNode),
                               beforeIndex, Ci.nsITreeView.DROP_BEFORE);
          dropPoint.beforeIndex = beforeIndex;
        }
      }
    }
    else {
      
      
      dropPoint.ip =
        new InsertionPoint(PlacesUtils.getConcreteItemId(this._resultNode),
                           -1, Ci.nsITreeView.DROP_BEFORE);
      dropPoint.beforeIndex = -1;
    }

    return dropPoint;
  },

  _setTimer: function PT_setTimer(aTime) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, aTime, timer.TYPE_ONE_SHOT);
    return timer;
  },

  notify: function PT_notify(aTimer) {
    if (aTimer == this._updateChevronTimer) {
      this._updateChevronTimer = null;
      this._updateChevronTimerCallback();
    }

    
    else if (aTimer == this._ibTimer) {
      this._dropIndicator.collapsed = true;
      this._ibTimer = null;
    }

    
    else if (aTimer == this._overFolder.openTimer) {
      
      
      this._overFolder.elt.lastChild.setAttribute("autoopened", "true");
      this._overFolder.elt.open = true;
      this._overFolder.openTimer = null;
    }

    
    else if (aTimer == this._overFolder.closeTimer) {
      
      
      
      let currentPlacesNode = PlacesControllerDragHelper.currentDropTarget;
      let inHierarchy = false;
      while (currentPlacesNode) {
        if (currentPlacesNode == this._rootElt) {
          inHierarchy = true;
          break;
        }
        currentPlacesNode = currentPlacesNode.parentNode;
      }
      
      
      if (inHierarchy)
        this._overFolder.elt = null;

      
      this._clearOverFolder();
    }
  },

  _onMouseOver: function PT__onMouseOver(aEvent) {
    let button = aEvent.target;
    if (button.parentNode == this._rootElt && button._placesNode &&
        PlacesUtils.nodeIsURI(button._placesNode))
      window.XULBrowserWindow.setOverLink(aEvent.target._placesNode.uri, null);
  },

  _onMouseOut: function PT__onMouseOut(aEvent) {
    window.XULBrowserWindow.setOverLink("", null);
  },

  _cleanupDragDetails: function PT__cleanupDragDetails() {
    
    PlacesControllerDragHelper.currentDropTarget = null;
    this._draggedElt = null;
    if (this._ibTimer)
      this._ibTimer.cancel();

    this._dropIndicator.collapsed = true;
  },

  _onDragStart: function PT__onDragStart(aEvent) {
    
    let draggedElt = aEvent.target;
    if (draggedElt.parentNode != this._rootElt || !draggedElt._placesNode)
      return;

    if (draggedElt.localName == "toolbarbutton" &&
        draggedElt.getAttribute("type") == "menu") {
      
      
#ifdef XP_UNIX
#ifndef XP_MACOSX
      if (this._mouseDownTimer) {
        this._mouseDownTimer.cancel();
        this._mouseDownTimer = null;
      }
#endif
#endif
      let translateY = this._cachedMouseMoveEvent.clientY - aEvent.clientY;
      let translateX = this._cachedMouseMoveEvent.clientX - aEvent.clientX;
      if ((translateY) >= Math.abs(translateX/2)) {
        
        aEvent.preventDefault();
        
        draggedElt.open = true;
        return;
      }

      
      if (draggedElt.open) {
        draggedElt.lastChild.hidePopup();
        draggedElt.open = false;
      }
    }

    
    this._draggedElt = draggedElt._placesNode;
    this._rootElt.focus();

    this._controller.setDataTransfer(aEvent);
    aEvent.stopPropagation();
  },

  _onDragOver: function PT__onDragOver(aEvent) {
    
    PlacesControllerDragHelper.currentDropTarget = aEvent.target;
    let dt = aEvent.dataTransfer;

    let dropPoint = this._getDropPoint(aEvent);
    if (!dropPoint || !dropPoint.ip ||
        !PlacesControllerDragHelper.canDrop(dropPoint.ip, dt)) {
      this._dropIndicator.collapsed = true;
      aEvent.stopPropagation();
      return;
    }

    if (this._ibTimer) {
      this._ibTimer.cancel();
      this._ibTimer = null;
    }

    if (dropPoint.folderElt || aEvent.originalTarget == this._chevron) {
      
      
      let overElt = dropPoint.folderElt || this._chevron;
      if (this._overFolder.elt != overElt) {
        this._clearOverFolder();
        this._overFolder.elt = overElt;
        this._overFolder.openTimer = this._setTimer(this._overFolder.hoverTime);
      }
      if (!this._overFolder.elt.hasAttribute("dragover"))
        this._overFolder.elt.setAttribute("dragover", "true");

      this._dropIndicator.collapsed = true;
    }
    else {
      
      
      let ind = this._dropIndicator;
      let halfInd = ind.clientWidth / 2;
      let translateX;
      if (this.isRTL) {
        halfInd = Math.ceil(halfInd);
        translateX = 0 - this._rootElt.getBoundingClientRect().right - halfInd;
        if (this._rootElt.firstChild) {
          if (dropPoint.beforeIndex == -1)
            translateX += this._rootElt.lastChild.getBoundingClientRect().left;
          else {
            translateX += this._rootElt.childNodes[dropPoint.beforeIndex]
                              .getBoundingClientRect().right;
          }
        }
      }
      else {
        halfInd = Math.floor(halfInd);
        translateX = 0 - this._rootElt.getBoundingClientRect().left +
                     halfInd;
        if (this._rootElt.firstChild) {
          if (dropPoint.beforeIndex == -1)
            translateX += this._rootElt.lastChild.getBoundingClientRect().right;
          else {
            translateX += this._rootElt.childNodes[dropPoint.beforeIndex]
                              .getBoundingClientRect().left;
          }
        }
      }

      ind.style.MozTransform = "translate(" + Math.round(translateX) + "px)";
      ind.style.MozMarginStart = (-ind.clientWidth) + "px";
      ind.collapsed = false;

      
      this._clearOverFolder();
    }

    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  _onDrop: function PT__onDrop(aEvent) {
    PlacesControllerDragHelper.currentDropTarget = aEvent.target;

    let dropPoint = this._getDropPoint(aEvent);
    if (dropPoint && dropPoint.ip) {
      PlacesControllerDragHelper.onDrop(dropPoint.ip, aEvent.dataTransfer)
      aEvent.preventDefault();
    }

    this._cleanupDragDetails();
    aEvent.stopPropagation();
  },

  _onDragExit: function PT__onDragExit(aEvent) {
    PlacesControllerDragHelper.currentDropTarget = null;

    
    
    
    if (this._ibTimer)
      this._ibTimer.cancel();
    this._ibTimer = this._setTimer(10);

    
    if (this._overFolder.elt)
        this._overFolder.closeTimer = this._setTimer(this._overFolder.hoverTime);
  },

  _onDragEnd: function PT_onDragEnd(aEvent) {
    this._cleanupDragDetails();
  },

  _onPopupShowing: function PT__onPopupShowing(aEvent) {
    if (!this._allowPopupShowing) {
      this._allowPopupShowing = true;
      aEvent.preventDefault();
      return;
    }

    let parent = aEvent.target.parentNode;
    if (parent.localName == "toolbarbutton")
      this._openedMenuButton = parent;

    return PlacesViewBase.prototype._onPopupShowing.apply(this, arguments);
  },

  _onPopupHidden: function PT__onPopupHidden(aEvent) {
    let popup = aEvent.target;

    
    if (popup._placesNode && PlacesUIUtils.getViewForNode(popup) == this) {
      
      
      if (!PlacesUtils.nodeIsFolder(popup._placesNode))
        popup._placesNode.containerOpen = false;
    }

    let parent = popup.parentNode;
    if (parent.localName == "toolbarbutton") {
      this._openedMenuButton = null;
      
      
      
      if (parent.hasAttribute("dragover"))
        parent.removeAttribute("dragover");
    }
  },

#ifdef XP_UNIX
#ifndef XP_MACOSX
  _onMouseDown: function PT__onMouseDown(aEvent) {
    let target = aEvent.target;
    if (aEvent.button == 0 &&
        target.localName == "toolbarbutton" &&
        target.getAttribute("type") == "menu") {
      this._allowPopupShowing = false;
      
      
      
      this._mouseDownTimer = Cc["@mozilla.org/timer;1"].
                             createInstance(Ci.nsITimer);
      let callback = {
        _self: this,
        _target: target,
        notify: function(timer) {
          this._target.open = true;
          this._mouseDownTimer = null;
        }
      };

      this._mouseDownTimer.initWithCallback(callback, 300,
                                            Ci.nsITimer.TYPE_ONE_SHOT);
    }
  },

  _onMouseUp: function PT__onMouseUp(aEvent) {
    if (aEvent.button != 0)
      return;

    if (this._mouseDownTimer) {
      
      this._mouseDownTimer.cancel();
      this._mouseDownTimer = null;
      aEvent.target.open = true;
    }
  },
#endif
#endif

  _onMouseMove: function PT__onMouseMove(aEvent) {
    
    this._cachedMouseMoveEvent = aEvent;

    if (this._openedMenuButton == null ||
        PlacesControllerDragHelper.getSession())
      return;

    let target = aEvent.originalTarget;
    if (this._openedMenuButton != target &&
        target.localName == "toolbarbutton" &&
        target.type == "menu") {
      this._openedMenuButton.open = false;
      target.open = true;
    }
  }
};





function PlacesMenu(aPopupShowingEvent, aPlace) {
  this._rootElt = aPopupShowingEvent.target; 
  this._viewElt = this._rootElt.parentNode;   
  this._viewElt._placesView = this;
  this._addEventListeners(this._rootElt, ["popupshowing", "popuphidden"], true);
  this._addEventListeners(window, ["unload"], false);

#ifdef XP_MACOSX
  if (this._viewElt.parentNode.localName == "menubar") {
    this._nativeView = true;
    this._rootElt._startMarker = -1;
    this._rootElt._endMarker = -1;
  }
#endif

  PlacesViewBase.call(this, aPlace);
  this._onPopupShowing(aPopupShowingEvent);
}

PlacesMenu.prototype = {
  __proto__: PlacesViewBase.prototype,

  QueryInterface: function PM_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIDOMEventListener))
      return this;

    return PlacesViewBase.prototype.QueryInterface.apply(this, arguments);
  },

  _removeChild: function PM_removeChild(aChild) {
    PlacesViewBase.prototype._removeChild.apply(this, arguments);
    if (this._endMarker != -1)
      this._endMarker--;
  },

  uninit: function PM_uninit() {
    this._removeEventListeners(this._rootElt, ["popupshowing", "popuphidden"],
                               true);
    this._removeEventListeners(window, ["unload"], false);

    PlacesViewBase.prototype.uninit.apply(this, arguments);
  },

  handleEvent: function PM_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "unload":
        this.uninit();
        break;
      case "popupshowing":
        this._onPopupShowing(aEvent);
        break;
      case "popuphidden":
        this._onPopupHidden(aEvent);
        break;
    }
  },

  _onPopupHidden: function PM__onPopupHidden(aEvent) {
    
    let popup = aEvent.originalTarget;
    if (!popup._placesNode || PlacesUIUtils.getViewForNode(popup) != this)
      return;

    
    
    if (!PlacesUtils.nodeIsFolder(popup._placesNode))
      popup._placesNode.containerOpen = false;

    
    
    
    popup.removeAttribute("autoopened");
    popup.removeAttribute("dragstart");
  }
};

