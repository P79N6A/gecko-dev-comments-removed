



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");





function PlacesViewBase(aPlace, aOptions) {
  this.place = aPlace;
  this.options = aOptions;
  this._controller = new PlacesController(this);
  this._viewElt.controllers.appendController(this._controller);
}

PlacesViewBase.prototype = {
  
  _viewElt: null,
  get viewElt() this._viewElt,

  get associatedElement() this._viewElt,

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
      return val;

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
      this._domNodes = new Map();
      this._domNodes.set(this._resultNode, this._rootElt);

      
      this._resultNode.containerOpen = true;
    }
    else {
      this._resultNode = null;
      delete this._domNodes;
    }

    return val;
  },

  _options: null,
  get options() this._options,
  set options(val) {
    if (!val)
      val = {};

    if (!("extraClasses" in val))
      val.extraClasses = {};
    this._options = val;

    return val;
  },

  






  _getDOMNodeForPlacesNode:
  function PVB__getDOMNodeForPlacesNode(aPlacesNode) {
    let node = this._domNodes.get(aPlacesNode, null);
    if (!node) {
      throw new Error("No DOM node set for aPlacesNode.\nnode.type: " +
                      aPlacesNode.type + ". node.parent: " + aPlacesNode);
    }
    return node;
  },

  get controller() this._controller,

  get selType() "single",
  selectItems: function() { },
  selectAll: function() { },

  get selectedNode() {
    if (this._contextMenuShown) {
      let anchor = this._contextMenuShown.triggerNode;
      if (!anchor)
        return null;

      if (anchor._placesNode)
        return this._rootElt == anchor ? null : anchor._placesNode;

      anchor = anchor.parentNode;
      return this._rootElt == anchor ? null : (anchor._placesNode || null);
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
    let tagName = null;

    let selectedNode = this.selectedNode;
    if (selectedNode) {
      let popup = document.popupNode;
      if (!popup._placesNode || popup._placesNode == this._resultNode ||
          popup._placesNode.itemId == -1) {
        
        
        container = selectedNode;
        orientation = Ci.nsITreeView.DROP_ON;
      }
      else {
        
        container = selectedNode.parent;
        index = container.getChildIndex(selectedNode);
        if (PlacesUtils.nodeIsTagQuery(container))
          tagName = container.title;
      }
    }

    if (PlacesControllerDragHelper.disallowInsertion(container))
      return null;

    return new InsertionPoint(PlacesUtils.getConcreteItemId(container),
                              index, orientation, tagName);
  },

  buildContextMenu: function PVB_buildContextMenu(aPopup) {
    this._contextMenuShown = aPopup;
    window.updateCommands("places");
    return this.controller.buildContextMenu(aPopup);
  },

  destroyContextMenu: function PVB_destroyContextMenu(aPopup) {
    this._contextMenuShown = null;
  },

  _cleanPopup: function PVB_cleanPopup(aPopup, aDelay) {
    
    let child = aPopup._startMarker;
    while (child.nextSibling != aPopup._endMarker) {
      let sibling = child.nextSibling;
      if (sibling._placesNode && !aDelay) {
        aPopup.removeChild(sibling);
      }
      else if (sibling._placesNode && aDelay) {
        
        
        
        if (!aPopup._delayedRemovals)
          aPopup._delayedRemovals = [];
        aPopup._delayedRemovals.push(sibling);
        child = child.nextSibling;
      }
      else {
        child = child.nextSibling;
      }
    }
  },

  _rebuildPopup: function PVB__rebuildPopup(aPopup) {
    let resultNode = aPopup._placesNode;
    if (!resultNode.containerOpen)
      return;

    if (this.controller.hasCachedLivemarkInfo(resultNode)) {
      this._setEmptyPopupStatus(aPopup, false);
      aPopup._built = true;
      this._populateLivemarkPopup(aPopup);
      return;
    }

    this._cleanPopup(aPopup);

    let cc = resultNode.childCount;
    if (cc > 0) {
      this._setEmptyPopupStatus(aPopup, false);

      for (let i = 0; i < cc; ++i) {
        let child = resultNode.getChild(i);
        this._insertNewItemToPopup(child, aPopup, null);
      }
    }
    else {
      this._setEmptyPopupStatus(aPopup, true);
    }
    aPopup._built = true;
  },

  _removeChild: function PVB__removeChild(aChild) {
    
    
    
    if (document.popupNode == aChild)
      document.popupNode = null;

    aChild.parentNode.removeChild(aChild);
  },

  _setEmptyPopupStatus:
  function PVB__setEmptyPopupStatus(aPopup, aEmpty) {
    if (!aPopup._emptyMenuitem) {
      let label = PlacesUIUtils.getString("bookmarksMenuEmptyFolder");
      aPopup._emptyMenuitem = document.createElement("menuitem");
      aPopup._emptyMenuitem.setAttribute("label", label);
      aPopup._emptyMenuitem.setAttribute("disabled", true);
      aPopup._emptyMenuitem.className = "bookmark-item";
      if (typeof this.options.extraClasses.entry == "string")
        aPopup._emptyMenuitem.classList.add(this.options.extraClasses.entry);
    }

    if (aEmpty) {
      aPopup.setAttribute("emptyplacesresult", "true");
      
      if (!aPopup._startMarker.previousSibling &&
          !aPopup._endMarker.nextSibling)
        aPopup.insertBefore(aPopup._emptyMenuitem, aPopup._endMarker);
    }
    else {
      aPopup.removeAttribute("emptyplacesresult");
      try {
        aPopup.removeChild(aPopup._emptyMenuitem);
      } catch (ex) {}
    }
  },

  _createMenuItemForPlacesNode:
  function PVB__createMenuItemForPlacesNode(aPlacesNode) {
    this._domNodes.delete(aPlacesNode);

    let element;
    let type = aPlacesNode.type;
    if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
      element = document.createElement("menuseparator");
      element.setAttribute("class", "small-separator");
    }
    else {
      let itemId = aPlacesNode.itemId;
      if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_URI) {
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
        else if (itemId != -1) {
          PlacesUtils.livemarks.getLivemark({ id: itemId })
            .then(aLivemark => {
              element.setAttribute("livemark", "true");
#ifdef XP_MACOSX
              
              
              element.setAttribute("image", "");
              element.removeAttribute("image");
#endif
              this.controller.cacheLivemarkInfo(aPlacesNode, aLivemark);
            }, () => undefined);
        }

        let popup = document.createElement("menupopup");
        popup._placesNode = PlacesUtils.asContainer(aPlacesNode);

        if (!this._nativeView) {
          popup.setAttribute("placespopup", "true");
        }

        element.appendChild(popup);
        element.className = "menu-iconic bookmark-item";
        if (typeof this.options.extraClasses.entry == "string") {
          element.classList.add(this.options.extraClasses.entry);
        }

        this._domNodes.set(aPlacesNode, popup);
      }
      else
        throw "Unexpected node";

      element.setAttribute("label", PlacesUIUtils.getBestTitle(aPlacesNode));

      let icon = aPlacesNode.icon;
      if (icon)
        element.setAttribute("image",
                             PlacesUtils.getImageURLForResolution(window, icon));
    }

    element._placesNode = aPlacesNode;
    if (!this._domNodes.has(aPlacesNode))
      this._domNodes.set(aPlacesNode, element);

    return element;
  },

  _insertNewItemToPopup:
  function PVB__insertNewItemToPopup(aNewChild, aPopup, aBefore) {
    let element = this._createMenuItemForPlacesNode(aNewChild);
    let before = aBefore || aPopup._endMarker;

    if (element.localName == "menuitem" || element.localName == "menu") {
      if (typeof this.options.extraClasses.entry == "string")
        element.classList.add(this.options.extraClasses.entry);
    }

    aPopup.insertBefore(element, before);
    return element;
  },

  _setLivemarkSiteURIMenuItem:
  function PVB__setLivemarkSiteURIMenuItem(aPopup) {
    let livemarkInfo = this.controller.getCachedLivemarkInfo(aPopup._placesNode);
    let siteUrl = livemarkInfo && livemarkInfo.siteURI ?
                  livemarkInfo.siteURI.spec : null;
    if (!siteUrl && aPopup._siteURIMenuitem) {
      aPopup.removeChild(aPopup._siteURIMenuitem);
      aPopup._siteURIMenuitem = null;
      aPopup.removeChild(aPopup._siteURIMenuseparator);
      aPopup._siteURIMenuseparator = null;
    }
    else if (siteUrl && !aPopup._siteURIMenuitem) {
      
      aPopup._siteURIMenuitem = document.createElement("menuitem");
      aPopup._siteURIMenuitem.className = "openlivemarksite-menuitem";
      if (typeof this.options.extraClasses.entry == "string") {
        aPopup._siteURIMenuitem.classList.add(this.options.extraClasses.entry);
      }
      aPopup._siteURIMenuitem.setAttribute("targetURI", siteUrl);
      aPopup._siteURIMenuitem.setAttribute("oncommand",
        "openUILink(this.getAttribute('targetURI'), event);");

      
      
      
      
      aPopup._siteURIMenuitem.setAttribute("onclick",
        "checkForMiddleClick(this, event); event.stopPropagation();");
      let label =
        PlacesUIUtils.getFormattedString("menuOpenLivemarkOrigin.label",
                                         [aPopup.parentNode.getAttribute("label")])
      aPopup._siteURIMenuitem.setAttribute("label", label);
      aPopup.insertBefore(aPopup._siteURIMenuitem, aPopup._startMarker);

      aPopup._siteURIMenuseparator = document.createElement("menuseparator");
      aPopup.insertBefore(aPopup._siteURIMenuseparator, aPopup._startMarker);
    }
  },

  






  _setLivemarkStatusMenuItem:
  function PVB_setLivemarkStatusMenuItem(aPopup, aStatus) {
    let statusMenuitem = aPopup._statusMenuitem;
    if (!statusMenuitem) {
      
      statusMenuitem = document.createElement("menuitem");
      statusMenuitem.className = "livemarkstatus-menuitem";
      if (typeof this.options.extraClasses.entry == "string") {
        statusMenuitem.classList.add(this.options.extraClasses.entry);
      }
      statusMenuitem.setAttribute("disabled", true);
      aPopup._statusMenuitem = statusMenuitem;
    }

    if (aStatus == Ci.mozILivemark.STATUS_LOADING ||
        aStatus == Ci.mozILivemark.STATUS_FAILED) {
      
      let stringId = aStatus == Ci.mozILivemark.STATUS_LOADING ?
                       "bookmarksLivemarkLoading" : "bookmarksLivemarkFailed";
      statusMenuitem.setAttribute("label", PlacesUIUtils.getString(stringId));
      if (aPopup._startMarker.nextSibling != statusMenuitem)
        aPopup.insertBefore(statusMenuitem, aPopup._startMarker.nextSibling);
    }
    else {
      
      if (aPopup._statusMenuitem.parentNode == aPopup)
        aPopup.removeChild(aPopup._statusMenuitem);
    }
  },

  toggleCutNode: function PVB_toggleCutNode(aPlacesNode, aValue) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;
    if (aValue)
      elt.setAttribute("cutting", "true");
    else
      elt.removeAttribute("cutting");
  },

  nodeURIChanged: function PVB_nodeURIChanged(aPlacesNode, aURIString) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    elt.setAttribute("scheme", PlacesUIUtils.guessUrlSchemeForUI(aURIString));
  },

  nodeIconChanged: function PVB_nodeIconChanged(aPlacesNode) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    
    if (elt == this._rootElt)
      return;

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    let icon = aPlacesNode.icon;
    if (!icon)
      elt.removeAttribute("image");
    else if (icon != elt.getAttribute("image"))
      elt.setAttribute("image",
                       PlacesUtils.getImageURLForResolution(window, icon));
  },

  nodeAnnotationChanged:
  function PVB_nodeAnnotationChanged(aPlacesNode, aAnno) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    
    if (aAnno == PlacesUtils.LMANNO_FEEDURI) {
      let menu = elt.parentNode;
      if (!menu.hasAttribute("livemark")) {
        menu.setAttribute("livemark", "true");
#ifdef XP_MACOSX
        
        
        menu.setAttribute("image", "");
        menu.removeAttribute("image");
#endif
      }

      PlacesUtils.livemarks.getLivemark({ id: aPlacesNode.itemId })
        .then(aLivemark => {
          
          this.controller.cacheLivemarkInfo(aPlacesNode, aLivemark);
          this.invalidateContainer(aPlacesNode);
        }, () => undefined);
    }
  },

  nodeTitleChanged:
  function PVB_nodeTitleChanged(aPlacesNode, aNewTitle) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    
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
    let parentElt = this._getDOMNodeForPlacesNode(aParentPlacesNode);
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (parentElt._built) {
      parentElt.removeChild(elt);

      
      
      
      if (parentElt._startMarker.nextSibling == parentElt._endMarker)
        this._setEmptyPopupStatus(parentElt, true);
    }
  },

  nodeHistoryDetailsChanged:
  function PVB_nodeHistoryDetailsChanged(aPlacesNode, aTime, aCount) {
    if (aPlacesNode.parent &&
        this.controller.hasCachedLivemarkInfo(aPlacesNode.parent)) {
      
      let popup = this._getDOMNodeForPlacesNode(aPlacesNode.parent);
      for (let child = popup._startMarker.nextSibling;
           child != popup._endMarker;
           child = child.nextSibling) {
        if (child._placesNode && child._placesNode.uri == aPlacesNode.uri) {
          if (aCount)
            child.setAttribute("visited", "true");
          else
            child.removeAttribute("visited");
          break;
        }
      }
    }
  },

  nodeTagsChanged: function() { },
  nodeDateAddedChanged: function() { },
  nodeLastModifiedChanged: function() { },
  nodeKeywordChanged: function() { },
  sortingChanged: function() { },
  batching: function() { },

  nodeInserted:
  function PVB_nodeInserted(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = this._getDOMNodeForPlacesNode(aParentPlacesNode);
    if (!parentElt._built)
      return;

    let index = Array.indexOf(parentElt.childNodes, parentElt._startMarker) +
                aIndex + 1;
    this._insertNewItemToPopup(aPlacesNode, parentElt,
                               parentElt.childNodes[index]);
    this._setEmptyPopupStatus(parentElt, false);
  },

  nodeMoved:
  function PBV_nodeMoved(aPlacesNode,
                         aOldParentPlacesNode, aOldIndex,
                         aNewParentPlacesNode, aNewIndex) {
    
    
    
    
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    
    
    if (elt == this._rootElt)
      return;

    let parentElt = this._getDOMNodeForPlacesNode(aNewParentPlacesNode);
    if (parentElt._built) {
      
      parentElt.removeChild(elt);
      let index = Array.indexOf(parentElt.childNodes, parentElt._startMarker) +
                  aNewIndex + 1;
      parentElt.insertBefore(elt, parentElt.childNodes[index]);
    }
  },

  containerStateChanged:
  function PVB_containerStateChanged(aPlacesNode, aOldState, aNewState) {
    if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_OPENED ||
        aNewState == Ci.nsINavHistoryContainerResultNode.STATE_CLOSED) {
      this.invalidateContainer(aPlacesNode);

      if (PlacesUtils.nodeIsFolder(aPlacesNode)) {
        let queryOptions = PlacesUtils.asQuery(this._result.root).queryOptions;
        if (queryOptions.excludeItems) {
          return;
        }

        PlacesUtils.livemarks.getLivemark({ id: aPlacesNode.itemId })
          .then(aLivemark => {
            let shouldInvalidate =
              !this.controller.hasCachedLivemarkInfo(aPlacesNode);
            this.controller.cacheLivemarkInfo(aPlacesNode, aLivemark);
            if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_OPENED) {
              aLivemark.registerForUpdates(aPlacesNode, this);
              
              aLivemark.reload();
              PlacesUtils.livemarks.reloadLivemarks();
              if (shouldInvalidate)
                this.invalidateContainer(aPlacesNode);
            }
            else {
              aLivemark.unregisterForUpdates(aPlacesNode);
            }
          }, () => undefined);
      }
    }
  },

  _populateLivemarkPopup: function PVB__populateLivemarkPopup(aPopup)
  {
    this._setLivemarkSiteURIMenuItem(aPopup);
    
    if (aPopup._startMarker.nextSibling == aPopup._endMarker)
      this._setLivemarkStatusMenuItem(aPopup, Ci.mozILivemark.STATUS_LOADING);

    PlacesUtils.livemarks.getLivemark({ id: aPopup._placesNode.itemId })
      .then(aLivemark => {
        let placesNode = aPopup._placesNode;
        if (!placesNode.containerOpen)
          return;

        if (aLivemark.status != Ci.mozILivemark.STATUS_LOADING)
          this._setLivemarkStatusMenuItem(aPopup, aLivemark.status);
        this._cleanPopup(aPopup,
          this._nativeView && aPopup.parentNode.hasAttribute("open"));

        let children = aLivemark.getNodesForContainer(placesNode);
        for (let i = 0; i < children.length; i++) {
          let child = children[i];
          this.nodeInserted(placesNode, child, i);
          if (child.accessCount)
            this._getDOMNodeForPlacesNode(child).setAttribute("visited", true);
          else
            this._getDOMNodeForPlacesNode(child).removeAttribute("visited");
        }
      }, Components.utils.reportError);
  },

  invalidateContainer: function PVB_invalidateContainer(aPlacesNode) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
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

    if (this._controller) {
      this._controller.terminate();
      
      
      
      
      try {
        this._viewElt.controllers.removeController(this._controller);
      } catch (ex) {
      } finally {
        this._controller = null;
      }
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

  get ownerWindow() window,

  




  _mayAddCommandsItems: function PVB__mayAddCommandsItems(aPopup) {
    
    if (aPopup == this._rootElt)
      return;

    let hasMultipleURIs = false;

    
    
    
    if (aPopup._placesNode.childCount > 0) {
      let currentChild = aPopup.firstChild;
      let numURINodes = 0;
      while (currentChild) {
        if (currentChild.localName == "menuitem" && currentChild._placesNode) {
          if (++numURINodes == 2)
            break;
        }
        currentChild = currentChild.nextSibling;
      }
      hasMultipleURIs = numURINodes > 1;
    }

    if (!hasMultipleURIs) {
      aPopup.setAttribute("singleitempopup", "true");
    } else {
      aPopup.removeAttribute("singleitempopup");
    }

    if (!hasMultipleURIs) {
      
      if (aPopup._endOptOpenAllInTabs) {
        aPopup.removeChild(aPopup._endOptOpenAllInTabs);
        aPopup._endOptOpenAllInTabs = null;

        aPopup.removeChild(aPopup._endOptSeparator);
        aPopup._endOptSeparator = null;
      }
    }
    else if (!aPopup._endOptOpenAllInTabs) {
      
      aPopup._endOptSeparator = document.createElement("menuseparator");
      aPopup._endOptSeparator.className = "bookmarks-actions-menuseparator";
      aPopup.appendChild(aPopup._endOptSeparator);

      
      aPopup._endOptOpenAllInTabs = document.createElement("menuitem");
      aPopup._endOptOpenAllInTabs.className = "openintabs-menuitem";

      if (typeof this.options.extraClasses.entry == "string")
        aPopup._endOptOpenAllInTabs.classList.add(this.options.extraClasses.entry);
      if (typeof this.options.extraClasses.footer == "string")
        aPopup._endOptOpenAllInTabs.classList.add(this.options.extraClasses.footer);

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

  _ensureMarkers: function PVB__ensureMarkers(aPopup) {
    if (aPopup._startMarker)
      return;

    
    aPopup._startMarker = document.createElement("menuseparator");
    aPopup._startMarker.hidden = true;
    aPopup.insertBefore(aPopup._startMarker, aPopup.firstChild);

    
    
    let node = ("insertionPoint" in this.options) ?
               aPopup.querySelector(this.options.insertionPoint) : null;
    if (node) {
      aPopup._endMarker = node;
    } else {
      aPopup._endMarker = document.createElement("menuseparator");
      aPopup._endMarker.hidden = true;
    }
    aPopup.appendChild(aPopup._endMarker);

    
    let firstNonStaticNodeFound = false;
    for (let i = 0; i < aPopup.childNodes.length; i++) {
      let child = aPopup.childNodes[i];
      
      
      
      if (child.getAttribute("builder") == "end") {
        aPopup.insertBefore(aPopup._endMarker, child);
        break;
      }

      if (child._placesNode && !firstNonStaticNodeFound) {
        firstNonStaticNodeFound = true;
        aPopup.insertBefore(aPopup._startMarker, child);
      }
    }
    if (!firstNonStaticNodeFound) {
      aPopup.insertBefore(aPopup._startMarker, aPopup._endMarker);
    }
  },

  _onPopupShowing: function PVB__onPopupShowing(aEvent) {
    
    let popup = aEvent.originalTarget;

    this._ensureMarkers(popup);

    
    if ("_delayedRemovals" in popup) {
      while (popup._delayedRemovals.length > 0) {
        popup.removeChild(popup._delayedRemovals.shift());
      }
    }

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
  let startTime = Date.now();
  
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

  
  
  
  
  if (this._viewElt.parentNode.parentNode == document.getElementById("TabsToolbar")) {
    this._addEventListeners(gBrowser.tabContainer, ["TabOpen", "TabClose"], false);
  }

  PlacesViewBase.call(this, aPlace);

  Services.telemetry.getHistogramById("FX_BOOKMARKS_TOOLBAR_INIT_MS")
                    .add(Date.now() - startTime);
}

PlacesToolbar.prototype = {
  __proto__: PlacesViewBase.prototype,

  _cbEvents: ["dragstart", "dragover", "dragexit", "dragend", "drop",
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
    this._removeEventListeners(gBrowser.tabContainer, ["TabOpen", "TabClose"], false);

    if (this._chevron._placesView) {
      this._chevron._placesView.uninit();
    }

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
    this._domNodes.delete(aChild);

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
        button.setAttribute("image",
                            PlacesUtils.getImageURLForResolution(window, icon));

      if (PlacesUtils.containerTypes.indexOf(type) != -1) {
        button.setAttribute("type", "menu");
        button.setAttribute("container", "true");

        if (PlacesUtils.nodeIsQuery(aChild)) {
          button.setAttribute("query", "true");
          if (PlacesUtils.nodeIsTagQuery(aChild))
            button.setAttribute("tagContainer", "true");
        }
        else if (PlacesUtils.nodeIsFolder(aChild)) {
          PlacesUtils.livemarks.getLivemark({ id: aChild.itemId })
            .then(aLivemark => {
              button.setAttribute("livemark", "true");
              this.controller.cacheLivemarkInfo(aChild, aLivemark);
            }, () => undefined);
        }

        let popup = document.createElement("menupopup");
        popup.setAttribute("placespopup", "true");
        button.appendChild(popup);
        popup._placesNode = PlacesUtils.asContainer(aChild);
        popup.setAttribute("context", "placesContext");

        this._domNodes.set(aChild, popup);
      }
      else if (PlacesUtils.nodeIsURI(aChild)) {
        button.setAttribute("scheme",
                            PlacesUIUtils.guessUrlSchemeForUI(aChild.uri));
      }
    }

    button._placesNode = aChild;
    if (!this._domNodes.has(aChild))
      this._domNodes.set(aChild, button);

    if (aBefore)
      this._rootElt.insertBefore(button, aBefore);
    else
      this._rootElt.appendChild(button);
  },

  _updateChevronPopupNodesVisibility:
  function PT__updateChevronPopupNodesVisibility() {
    for (let i = 0, node = this._chevronPopup._startMarker.nextSibling;
         node != this._chevronPopup._endMarker;
         i++, node = node.nextSibling) {
      node.hidden = this._rootElt.childNodes[i].style.visibility != "hidden";
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
        if (!this._isOverflowStateEventRelevant(aEvent))
          return;
        this._onOverflow();
        break;
      case "underflow":
        if (!this._isOverflowStateEventRelevant(aEvent))
          return;
        this._onUnderflow();
        break;
      case "TabOpen":
      case "TabClose":
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

  updateOverflowStatus: function() {
    if (this._rootElt.scrollLeftMax > 0) {
      this._onOverflow();
    } else {
      this._onUnderflow();
    }
  },

  _isOverflowStateEventRelevant: function PT_isOverflowStateEventRelevant(aEvent) {
    
    return aEvent.target == aEvent.currentTarget && aEvent.detail > 0;
  },

  _onOverflow: function PT_onOverflow() {
    
    
    if (!this._chevronPopup.hasAttribute("type")) {
      this._chevronPopup.setAttribute("place", this.place);
      this._chevronPopup.setAttribute("type", "places");
    }
    this._chevron.collapsed = false;
    this.updateChevron();
  },

  _onUnderflow: function PT_onUnderflow() {
    this.updateChevron();
    this._chevron.collapsed = true;
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
    let parentElt = this._getDOMNodeForPlacesNode(aParentPlacesNode);
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
    let parentElt = this._getDOMNodeForPlacesNode(aParentPlacesNode);
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
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
    let parentElt = this._getDOMNodeForPlacesNode(aNewParentPlacesNode);
    if (parentElt == this._rootElt) {
      

      
      let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

      
      if (elt.localName == "menupopup")
        elt = elt.parentNode;

      this._removeChild(elt);
      this._rootElt.insertBefore(elt, this._rootElt.childNodes[aNewIndex]);

      
      
      
      
      
      

      this.updateChevron();
      return;
    }

    PlacesViewBase.prototype.nodeMoved.apply(this, arguments);
  },

  nodeAnnotationChanged:
  function PT_nodeAnnotationChanged(aPlacesNode, aAnno) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
    if (elt == this._rootElt)
      return;

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (elt.parentNode == this._rootElt) {
      

      
      if (aAnno == PlacesUtils.LMANNO_FEEDURI) {
        elt.setAttribute("livemark", true);

        PlacesUtils.livemarks.getLivemark({ id: aPlacesNode.itemId })
          .then(aLivemark => {
            this.controller.cacheLivemarkInfo(aPlacesNode, aLivemark);
            this.invalidateContainer(aPlacesNode);
          }, Components.utils.reportError);
      }
    }
    else {
      
      PlacesViewBase.prototype.nodeAnnotationChanged.apply(this, arguments);
    }
  },

  nodeTitleChanged: function PT_nodeTitleChanged(aPlacesNode, aNewTitle) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    
    if (elt == this._rootElt)
      return;

    PlacesViewBase.prototype.nodeTitleChanged.apply(this, arguments);

    
    if (elt.localName == "menupopup")
      elt = elt.parentNode;

    if (elt.parentNode == this._rootElt) {
      
      this.updateChevron();
    }
  },

  invalidateContainer: function PT_invalidateContainer(aPlacesNode) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
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
          
          let tagName = PlacesUtils.nodeIsTagQuery(elt._placesNode) ?
                        elt._placesNode.title : null;
          dropPoint.ip =
            new InsertionPoint(PlacesUtils.getConcreteItemId(elt._placesNode),
                               -1, Ci.nsITreeView.DROP_ON,
                               tagName);
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
      ind.parentNode.collapsed = false;
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

      ind.style.transform = "translate(" + Math.round(translateX) + "px)";
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
                                .then(null, Components.utils.reportError);
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

    PlacesViewBase.prototype._onPopupShowing.apply(this, arguments);
  },

  _onPopupHidden: function PT__onPopupHidden(aEvent) {
    let popup = aEvent.target;
    let placesNode = popup._placesNode;
    
    if (placesNode && PlacesUIUtils.getViewForNode(popup) == this) {
      
      
      
      
      if (!PlacesUtils.nodeIsFolder(placesNode) ||
          this.controller.hasCachedLivemarkInfo(placesNode)) {
        placesNode.containerOpen = false;
      }
    }

    let parent = popup.parentNode;
    if (parent.localName == "toolbarbutton") {
      this._openedMenuButton = null;
      
      
      
      if (parent.hasAttribute("dragover"))
        parent.removeAttribute("dragover");
    }
  },

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





function PlacesMenu(aPopupShowingEvent, aPlace, aOptions) {
  this._rootElt = aPopupShowingEvent.target; 
  this._viewElt = this._rootElt.parentNode;   
  this._viewElt._placesView = this;
  this._addEventListeners(this._rootElt, ["popupshowing", "popuphidden"], true);
  this._addEventListeners(window, ["unload"], false);

#ifdef XP_MACOSX
  
  for (let elt = this._viewElt.parentNode; elt; elt = elt.parentNode) {
    if (elt.localName == "menubar") {
      this._nativeView = true;
      break;
    }
  }
#endif

  PlacesViewBase.call(this, aPlace, aOptions);
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
    let placesNode = popup._placesNode;
    if (!placesNode || PlacesUIUtils.getViewForNode(popup) != this)
      return;

    
    
    
    
    if (!PlacesUtils.nodeIsFolder(placesNode) ||
        this.controller.hasCachedLivemarkInfo(placesNode))
      placesNode.containerOpen = false;

    
    
    
    popup.removeAttribute("autoopened");
    popup.removeAttribute("dragstart");
  }
};

function PlacesPanelMenuView(aPlace, aViewId, aRootId, aOptions) {
  this._viewElt = document.getElementById(aViewId);
  this._rootElt = document.getElementById(aRootId);
  this._viewElt._placesView = this;
  this.options = aOptions;

  PlacesViewBase.call(this, aPlace, aOptions);
}

PlacesPanelMenuView.prototype = {
  __proto__: PlacesViewBase.prototype,

  QueryInterface: function PAMV_QueryInterface(aIID) {
    return PlacesViewBase.prototype.QueryInterface.apply(this, arguments);
  },

  uninit: function PAMV_uninit() {
    PlacesViewBase.prototype.uninit.apply(this, arguments);
  },

  _insertNewItem:
  function PAMV__insertNewItem(aChild, aBefore) {
    this._domNodes.delete(aChild);

    let type = aChild.type;
    let button;
    if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
      button = document.createElement("toolbarseparator");
      button.setAttribute("class", "small-separator");
    }
    else {
      button = document.createElement("toolbarbutton");
      button.className = "bookmark-item";
      if (typeof this.options.extraClasses.entry == "string")
        button.classList.add(this.options.extraClasses.entry);
      button.setAttribute("label", aChild.title);
      let icon = aChild.icon;
      if (icon)
        button.setAttribute("image",
                            PlacesUtils.getImageURLForResolution(window, icon));

      if (PlacesUtils.containerTypes.indexOf(type) != -1) {
        button.setAttribute("container", "true");

        if (PlacesUtils.nodeIsQuery(aChild)) {
          button.setAttribute("query", "true");
          if (PlacesUtils.nodeIsTagQuery(aChild))
            button.setAttribute("tagContainer", "true");
        }
        else if (PlacesUtils.nodeIsFolder(aChild)) {
          PlacesUtils.livemarks.getLivemark({ id: aChild.itemId })
            .then(aLivemark => {
              button.setAttribute("livemark", "true");
              this.controller.cacheLivemarkInfo(aChild, aLivemark);
            }, () => undefined);
        }
      }
      else if (PlacesUtils.nodeIsURI(aChild)) {
        button.setAttribute("scheme",
                            PlacesUIUtils.guessUrlSchemeForUI(aChild.uri));
      }
    }

    button._placesNode = aChild;
    if (!this._domNodes.has(aChild))
      this._domNodes.set(aChild, button);

    this._rootElt.insertBefore(button, aBefore);
  },

  nodeInserted:
  function PAMV_nodeInserted(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = this._getDOMNodeForPlacesNode(aParentPlacesNode);
    if (parentElt != this._rootElt)
      return;

    let children = this._rootElt.childNodes;
    this._insertNewItem(aPlacesNode,
      aIndex < children.length ? children[aIndex] : null);
  },

  nodeRemoved:
  function PAMV_nodeRemoved(aParentPlacesNode, aPlacesNode, aIndex) {
    let parentElt = this._getDOMNodeForPlacesNode(aParentPlacesNode);
    if (parentElt != this._rootElt)
      return;

    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
    this._removeChild(elt);
  },

  nodeMoved:
  function PAMV_nodeMoved(aPlacesNode,
                          aOldParentPlacesNode, aOldIndex,
                          aNewParentPlacesNode, aNewIndex) {
    let parentElt = this._getDOMNodeForPlacesNode(aNewParentPlacesNode);
    if (parentElt != this._rootElt)
      return;

    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
    this._removeChild(elt);
    this._rootElt.insertBefore(elt, this._rootElt.childNodes[aNewIndex]);
  },

  nodeAnnotationChanged:
  function PAMV_nodeAnnotationChanged(aPlacesNode, aAnno) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
    
    if (elt == this._rootElt)
      return;

    if (elt.parentNode != this._rootElt)
      return;

    
    if (aAnno == PlacesUtils.LMANNO_FEEDURI) {
      elt.setAttribute("livemark", true);

      PlacesUtils.livemarks.getLivemark({ id: aPlacesNode.itemId })
        .then(aLivemark => {
          this.controller.cacheLivemarkInfo(aPlacesNode, aLivemark);
          this.invalidateContainer(aPlacesNode);
        }, Components.utils.reportError);
    }
  },

  nodeTitleChanged: function PAMV_nodeTitleChanged(aPlacesNode, aNewTitle) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);

    
    if (elt == this._rootElt)
      return;

    PlacesViewBase.prototype.nodeTitleChanged.apply(this, arguments);
  },

  invalidateContainer: function PAMV_invalidateContainer(aPlacesNode) {
    let elt = this._getDOMNodeForPlacesNode(aPlacesNode);
    if (elt != this._rootElt)
      return;

    
    while (this._rootElt.hasChildNodes()) {
      this._rootElt.removeChild(this._rootElt.firstChild);
    }

    for (let i = 0; i < this._resultNode.childCount; ++i) {
      this._insertNewItem(this._resultNode.getChild(i), null);
    }
  }
};
