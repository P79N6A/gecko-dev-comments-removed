



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizeMode"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const kPrefCustomizationDebug = "browser.uiCustomization.debug";
const kPaletteId = "customization-palette";
const kAboutURI = "about:customizing";
const kDragDataTypePrefix = "text/toolbarwrapper-id/";
const kPlaceholderClass = "panel-customization-placeholder";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/CustomizableUI.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let gModuleName = "[CustomizeMode]";
#include logging.js

function CustomizeMode(aWindow) {
  this.window = aWindow;
  this.document = aWindow.document;
  this.browser = aWindow.gBrowser;
};

CustomizeMode.prototype = {
  _changed: false,
  window: null,
  document: null,
  
  areas: null,
  
  
  
  
  
  
  
  _stowedPalette: null,
  _dragOverItem: null,
  _customizing: false,

  get resetButton() {
    return this.document.getElementById("customization-reset-button");
  },

  get panelUIContents() {
    return this.document.getElementById("PanelUI-contents");
  },

  init: function() {
    
    
    
    
    this.visiblePalette = this.document.getElementById(kPaletteId);

    this.browser.tabContainer.addEventListener("TabSelect", this, false);
    this.browser.addTabsProgressListener(this);
  },

  uninit: function() {
    this.browser.tabContainer.removeEventListener("TabSelect", this, false);
    this.browser.removeTabsProgressListener(this);
  },

  enter: function() {
    if (this._customizing) {
      return;
    }

    
    
    if (this.browser.selectedBrowser.currentURI.spec != kAboutURI) {
      this.window.switchToTabHavingURI(kAboutURI, true);
      return;
    }

    this.dispatchToolboxEvent("beforecustomization");

    let window = this.window;
    let document = this.document;

    let customizer = document.getElementById("customization-container");
    customizer.hidden = false;


    CustomizableUI.addListener(this);

    
    
    
    window.PanelUI.ensureRegistered();

    
    
    
    let deck = document.getElementById("tab-view-deck");
    deck.addEventListener("keypress", this, false);
    deck.addEventListener("click", this, false);

    
    
    window.PanelUI.menuButton.addEventListener("click", this, false);
    window.PanelUI.menuButton.disabled = true;

    
    this.dispatchToolboxEvent("customizationstarting");

    customizer.parentNode.selectedPanel = customizer;

    window.PanelUI.hide();
    
    
    let panelHolder = document.getElementById("customization-panelHolder");
    panelHolder.appendChild(window.PanelUI.mainView);
    this._showPanelCustomizationPlaceholders();

    let self = this;
    deck.addEventListener("transitionend", function customizeTransitionEnd() {
      deck.removeEventListener("transitionend", customizeTransitionEnd);
      self.dispatchToolboxEvent("customizationready");
    });

    this._wrapToolbarItems();
    this.populatePalette();

    this.visiblePalette.addEventListener("dragstart", this, true);
    this.visiblePalette.addEventListener("dragover", this, true);
    this.visiblePalette.addEventListener("dragexit", this, true);
    this.visiblePalette.addEventListener("drop", this, true);
    this.visiblePalette.addEventListener("dragend", this, true);

    this.resetButton.hidden = CustomizableUI.inDefaultState;

    let customizableToolbars = document.querySelectorAll("toolbar[customizable=true]:not([autohide=true]):not([collapsed=true])");
    for (let toolbar of customizableToolbars)
      toolbar.setAttribute("customizing", true);

    document.documentElement.setAttribute("customizing", true);
    this._customizing = true;
  },

  exit: function() {
    if (!this._customizing) {
      return;
    }

    CustomizableUI.removeListener(this);

    let deck = this.document.getElementById("tab-view-deck");
    deck.removeEventListener("keypress", this, false);
    deck.removeEventListener("click", this, false);
    this.window.PanelUI.menuButton.removeEventListener("click", this, false);
    this.window.PanelUI.menuButton.disabled = false;

    this._removePanelCustomizationPlaceholders();
    this.depopulatePalette();

    this.visiblePalette.removeEventListener("dragstart", this, true);
    this.visiblePalette.removeEventListener("dragover", this, true);
    this.visiblePalette.removeEventListener("dragexit", this, true);
    this.visiblePalette.removeEventListener("drop", this, true);
    this.visiblePalette.removeEventListener("dragend", this, true);

    let window = this.window;
    let document = this.document;

    let documentElement = document.documentElement;
    documentElement.setAttribute("customize-exiting", "true");
    let tabViewDeck = document.getElementById("tab-view-deck");
    tabViewDeck.addEventListener("transitionend", function onTransitionEnd(evt) {
      if (evt.propertyName != "padding-top")
        return;
      tabViewDeck.removeEventListener("transitionend", onTransitionEnd);
      documentElement.removeAttribute("customize-exiting");
    });

    this._unwrapToolbarItems();

    if (this._changed) {
      
      
      
      
      this.persistCurrentSets();
    }

    
    this.areas = [];

    
    
    this.dispatchToolboxEvent("customizationending");
    window.PanelUI.setMainView(window.PanelUI.mainView);

    let browser = document.getElementById("browser");
    browser.parentNode.selectedPanel = browser;

    
    
    
    this._customizing = false;

    if (this.browser.selectedBrowser.currentURI.spec == kAboutURI) {
      let custBrowser = this.browser.selectedBrowser;
      if (custBrowser.canGoBack) {
        
        custBrowser.goBack();
      } else {
        
        
        
        if (this.window.getTopWin(true) == this.window) {
          let customizationTab = this.browser.selectedTab;
          if (this.browser.browsers.length == 1) {
            this.window.BrowserOpenTab();
          }
          this.browser.removeTab(customizationTab);
        }
      }
    }

    let deck = document.getElementById("tab-view-deck");
    let self = this;
    deck.addEventListener("transitionend", function customizeTransitionEnd() {
      deck.removeEventListener("transitionend", customizeTransitionEnd);
      self.dispatchToolboxEvent("aftercustomization");
    });
    documentElement.removeAttribute("customizing");

    let customizableToolbars = document.querySelectorAll("toolbar[customizable=true]:not([autohide=true])");
    for (let toolbar of customizableToolbars)
      toolbar.removeAttribute("customizing");

    let customizer = document.getElementById("customization-container");
    customizer.hidden = true;

    this._changed = false;
  },

  dispatchToolboxEvent: function(aEventType, aDetails={}) {
    let evt = this.document.createEvent("CustomEvent");
    evt.initCustomEvent(aEventType, true, true, {changed: this._changed});
    let result = this.window.gNavToolbox.dispatchEvent(evt);
  },

  populatePalette: function() {
    let toolboxPalette = this.window.gNavToolbox.palette;

    let unusedWidgets = CustomizableUI.getUnusedWidgets(toolboxPalette);
    for (let widget of unusedWidgets) {
      let paletteItem = this.makePaletteItem(widget, "palette");
      this.visiblePalette.appendChild(paletteItem);
    }

    this._stowedPalette = this.window.gNavToolbox.palette;
    this.window.gNavToolbox.palette = this.visiblePalette;
  },

  
  
  
  
  makePaletteItem: function(aWidget, aPlace) {
    let widgetNode = aWidget.forWindow(this.window).node;
    let wrapper = this.createWrapper(widgetNode, aPlace);
    wrapper.appendChild(widgetNode);
    return wrapper;
  },

  depopulatePalette: function() {
    let paletteChild = this.visiblePalette.firstChild;
    let nextChild;
    while (paletteChild) {
      nextChild = paletteChild.nextElementSibling;
      let provider = CustomizableUI.getWidget(paletteChild.id).provider;
      if (provider == CustomizableUI.PROVIDER_XUL) {
        let unwrappedPaletteItem = this.unwrapToolbarItem(paletteChild);
        this._stowedPalette.appendChild(unwrappedPaletteItem);
      } else if (provider == CustomizableUI.PROVIDER_API) {
        
        
        
        
        
        
      } else if (provider == CustomizableUI.PROVIDER_SPECIAL) {
        this.visiblePalette.removeChild(paletteChild);
      }

      paletteChild = nextChild;
    }
    this.window.gNavToolbox.palette = this._stowedPalette;
  },

  isCustomizableItem: function(aNode) {
    return aNode.localName == "toolbarbutton" ||
           aNode.localName == "toolbaritem" ||
           aNode.localName == "toolbarseparator" ||
           aNode.localName == "toolbarspring" ||
           aNode.localName == "toolbarspacer";
  },

  isWrappedToolbarItem: function(aNode) {
    return aNode.localName == "toolbarpaletteitem";
  },

  wrapToolbarItem: function(aNode, aPlace) {
    if (!this.isCustomizableItem(aNode)) {
      return aNode;
    }
    let wrapper = this.createWrapper(aNode, aPlace);
    
    
    
    
    
    if (aNode.parentNode) {
      aNode = aNode.parentNode.replaceChild(wrapper, aNode);
    }
    wrapper.appendChild(aNode);
    return wrapper;
  },

  createWrapper: function(aNode, aPlace) {
    let wrapper = this.document.createElement("toolbarpaletteitem");

    
    
    
    wrapper.setAttribute("place", aPlace);

    
    
    if (aNode.hasAttribute("command")) {
      wrapper.setAttribute("itemcommand", aNode.getAttribute("command"));
      aNode.removeAttribute("command");
    }

    if (aNode.checked) {
      wrapper.setAttribute("itemchecked", "true");
      aNode.checked = false;
    }

    if (aNode.disabled) {
      wrapper.setAttribute("itemdisabled", "true");
      aNode.disabled = false;
    }

    if (aNode.hasAttribute("id")) {
      wrapper.setAttribute("id", "wrapper-" + aNode.getAttribute("id"));
    }

    if (aNode.hasAttribute("title")) {
      wrapper.setAttribute("title", aNode.getAttribute("title"));
    } else if (aNode.hasAttribute("label")) {
      wrapper.setAttribute("title", aNode.getAttribute("label"));
    }

    if (aNode.hasAttribute("flex")) {
      wrapper.setAttribute("flex", aNode.getAttribute("flex"));
    }

    wrapper.addEventListener("mousedown", this);
    wrapper.addEventListener("mouseup", this);

    return wrapper;
  },

  unwrapToolbarItem: function(aWrapper) {
    if (aWrapper.nodeName != "toolbarpaletteitem") {
      return aWrapper;
    }
    aWrapper.removeEventListener("mousedown", this);
    aWrapper.removeEventListener("mouseup", this);

    let toolbarItem = aWrapper.firstChild;

    if (aWrapper.hasAttribute("itemdisabled")) {
      toolbarItem.disabled = true;
    }

    if (aWrapper.hasAttribute("itemchecked")) {
      toolbarItem.checked = true;
    }

    if (aWrapper.hasAttribute("itemcommand")) {
      let commandID = aWrapper.getAttribute("itemcommand");
      toolbarItem.setAttribute("command", commandID);

      
      let command = this.document.getElementById(commandID);
      if (command && command.hasAttribute("disabled")) {
        toolbarItem.setAttribute("disabled", command.getAttribute("disabled"));
      }
    }

    if (aWrapper.parentNode) {
      aWrapper.parentNode.replaceChild(toolbarItem, aWrapper);
    }
    return toolbarItem;
  },

  _wrapToolbarItems: function() {
    let window = this.window;
    
    this.areas = [];
    for (let area of CustomizableUI.areas) {
      let target = CustomizableUI.getCustomizeTargetForArea(area, window);
      target.addEventListener("dragstart", this, true);
      target.addEventListener("dragover", this, true);
      target.addEventListener("dragexit", this, true);
      target.addEventListener("drop", this, true);
      target.addEventListener("dragend", this, true);
      for (let child of target.children) {
        if (this.isCustomizableItem(child)) {
          this.wrapToolbarItem(child, getPlaceForItem(child));
        }
      }
      this.areas.push(target);
    }
  },

  _unwrapToolbarItems: function() {
    for (let target of this.areas) {
      for (let toolbarItem of target.children) {
        if (this.isWrappedToolbarItem(toolbarItem)) {
          this.unwrapToolbarItem(toolbarItem);
        }
      }
      target.removeEventListener("dragstart", this, true);
      target.removeEventListener("dragover", this, true);
      target.removeEventListener("dragexit", this, true);
      target.removeEventListener("drop", this, true);
      target.removeEventListener("dragend", this, true);
    }
  },

  persistCurrentSets: function()  {
    let document = this.document;
    let toolbars = document.querySelectorAll("toolbar[customizable='true']");
    for (let toolbar of toolbars) {
      let set = toolbar.currentSet;
      toolbar.setAttribute("currentset", set);
      LOG("Setting currentset of " + toolbar.id + " as " + set);
      
      document.persist(toolbar.id, "currentset");
    }
  },

  reset: function() {
    this._removePanelCustomizationPlaceholders();
    this.depopulatePalette();
    this._unwrapToolbarItems();

    CustomizableUI.reset();

    this._wrapToolbarItems();
    this.populatePalette();

    let document = this.document;
    let toolbars = document.querySelectorAll("toolbar[customizable='true']");
    for (let toolbar of toolbars) {
      let set = toolbar.currentSet;
      toolbar.removeAttribute("currentset");
      LOG("[RESET] Removing currentset of " + toolbar.id);
      
      document.persist(toolbar.id, "currentset");
    }

    this.resetButton.hidden = CustomizableUI.inDefaultState;
    this._showPanelCustomizationPlaceholders();
  },

  onWidgetMoved: function(aWidgetId, aArea, aOldPosition, aNewPosition) {
    this._onUIChange();
  },

  onWidgetAdded: function(aWidgetId, aArea, aPosition) {
    this._onUIChange();
  },

  onWidgetRemoved: function(aWidgetId, aArea) {
    this._onUIChange();
  },

  onWidgetCreated: function(aWidgetId) {
  },

  onWidgetDestroyed: function(aWidgetId) {
  },

  _onUIChange: function() {
    this._changed = true;
    this.resetButton.hidden = CustomizableUI.inDefaultState;
    this.dispatchToolboxEvent("customizationchange");
  },

  handleEvent: function(aEvent) {
    switch(aEvent.type) {
      case "dragstart":
        this._onDragStart(aEvent);
        break;
      case "dragover":
        this._onDragOver(aEvent);
        break;
      case "drop":
        this._onDragDrop(aEvent);
        break;
      case "dragexit":
        this._onDragExit(aEvent);
        break;
      case "dragend":
        this._onDragEnd(aEvent);
        break;
      case "mousedown":
        this._onMouseDown(aEvent);
        break;
      case "mouseup":
        this._onMouseUp(aEvent);
        break;
      case "keypress":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
          this.exit();
        }
        break;
      case "click":
        if (aEvent.button == 0 &&
            (aEvent.originalTarget == this.window.PanelUI.menuButton) ||
            (aEvent.originalTarget == this.document.getElementById("tab-view-deck"))) {
          this.exit();
          aEvent.preventDefault();
        }
        break;
      case "TabSelect":
        this._onTabSelect(aEvent);
        break;
    }
  },

  _onDragStart: function(aEvent) {
    __dumpDragData(aEvent);
    let item = aEvent.target;
    while (item && item.localName != "toolbarpaletteitem") {
      if (item.localName == "toolbar" ||
          item.classList.contains(kPlaceholderClass)) {
        return;
      }
      item = item.parentNode;
    }

    let dt = aEvent.dataTransfer;
    let documentId = aEvent.target.ownerDocument.documentElement.id;
    let draggedItem = item.firstChild;
    let draggedItemWidth = draggedItem.getBoundingClientRect().width + "px";

    let data = {
      id: draggedItem.id,
      width: draggedItemWidth,
    };

    dt.mozSetDataAt(kDragDataTypePrefix + documentId, data, 0);
    dt.effectAllowed = "move";

    
    
    let win = aEvent.target.ownerDocument.defaultView;
    win.setTimeout(function() {
      item.hidden = true;
      this._showPanelCustomizationPlaceholders();
    }.bind(this), 0);
  },

  _onDragOver: function(aEvent) {
    __dumpDragData(aEvent);

    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    if (!aEvent.dataTransfer.mozTypesAt(0)) {
      return;
    }

    let {id: draggedItemId, width: draggedItemWidth} =
      aEvent.dataTransfer.mozGetDataAt(kDragDataTypePrefix + documentId, 0);
    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);
    let targetArea = this._getCustomizableParent(aEvent.currentTarget);
    let originArea = this._getCustomizableParent(draggedWrapper);

    
    if (!targetArea || !originArea) {
      return;
    }

    
    if (targetArea.id == kPaletteId &&
       !CustomizableUI.isWidgetRemovable(draggedItemId)) {
      return;
    }

    
    if (targetArea.id != kPaletteId &&
        !CustomizableUI.canWidgetMoveToArea(draggedItemId, targetArea.id)) {
      return;
    }

    let targetNode = this._getDragOverNode(aEvent.target, targetArea);
    let targetParent = targetNode.parentNode;

    
    
    let dragOverItem;
    let atEnd = false;
    if (targetNode == targetArea.customizationTarget) {
      dragOverItem = targetNode.lastChild;
      atEnd = true;
    } else {
      let position = Array.indexOf(targetParent.children, targetNode);
      dragOverItem = position == -1 ? targetParent.lastChild : targetParent.children[position];
    }

    if (this._dragOverItem && dragOverItem != this._dragOverItem) {
      this._setDragActive(this._dragOverItem, false);
    }

    if (dragOverItem != this._dragOverItem) {
      this._setDragActive(dragOverItem, true, draggedItemWidth, atEnd);
      this._dragOverItem = dragOverItem;
    }

    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  _onDragDrop: function(aEvent) {
    __dumpDragData(aEvent);

    this._setDragActive(this._dragOverItem, false);
    this._removePanelCustomizationPlaceholders();

    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    let {id: draggedItemId} =
      aEvent.dataTransfer.mozGetDataAt(kDragDataTypePrefix + documentId, 0);
    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);

    draggedWrapper.hidden = false;
    draggedWrapper.removeAttribute("mousedown");

    let targetArea = this._getCustomizableParent(aEvent.currentTarget);
    let originArea = this._getCustomizableParent(draggedWrapper);

    
    if (!targetArea || !originArea) {
      return;
    }

    let targetNode = this._getDragOverNode(aEvent.target, targetArea);

    
    
    if (draggedWrapper == targetNode) {
      return;
    }

    
    
    if (targetArea.id == kPaletteId) {
      if (originArea.id !== kPaletteId) {
        if (!CustomizableUI.isWidgetRemovable(draggedItemId)) {
          return;
        }

        let widget = this.unwrapToolbarItem(draggedWrapper);
        CustomizableUI.removeWidgetFromArea(draggedItemId);
        draggedWrapper = this.wrapToolbarItem(widget, "palette");
      }

      
      if (targetNode == this.visiblePalette) {
        this.visiblePalette.appendChild(draggedWrapper);
      } else {
        this.visiblePalette.insertBefore(draggedWrapper, targetNode);
      }
      this._showPanelCustomizationPlaceholders();
      return;
    }

    if (!CustomizableUI.canWidgetMoveToArea(draggedItemId, targetArea.id)) {
      return;
    }

    
    
    if (targetNode == targetArea.customizationTarget) {
      let widget = this.unwrapToolbarItem(draggedWrapper);
      CustomizableUI.addWidgetToArea(draggedItemId, targetArea.id);
      this.wrapToolbarItem(widget, getPlaceForItem(targetNode));
      this._showPanelCustomizationPlaceholders();
      return;
    }

    
    
    let placement;
    if (!targetNode.classList.contains(kPlaceholderClass)) {
      let targetNodeId = (targetNode.nodeName == "toolbarpaletteitem") ?
                            targetNode.firstChild && targetNode.firstChild.id :
                            targetNode.id;
      placement = CustomizableUI.getPlacementOfWidget(targetNodeId);
    }
    if (!placement) {
      LOG("Could not get a position for " + targetNode + "#" + targetNode.id + "." + targetNode.className);
    }
    let position = placement ? placement.position :
                               targetArea.childElementCount;


    
    
    
    if (targetArea == originArea) {
      let properPlace = getPlaceForItem(targetNode);
      
      
      
      let widget = this.unwrapToolbarItem(draggedWrapper);
      let targetWidget = this.unwrapToolbarItem(targetNode);
      CustomizableUI.moveWidgetWithinArea(draggedItemId, position);
      this.wrapToolbarItem(targetWidget, properPlace);
      this.wrapToolbarItem(widget, properPlace);
      this._showPanelCustomizationPlaceholders();
      return;
    }

    
    
    
    
    let properPlace = getPlaceForItem(targetNode);
    let widget = this.unwrapToolbarItem(draggedWrapper);
    let targetWidget = this.unwrapToolbarItem(targetNode);
    CustomizableUI.addWidgetToArea(draggedItemId, targetArea.id, position);
    this.wrapToolbarItem(targetWidget, properPlace);
    draggedWrapper = this.wrapToolbarItem(widget, properPlace);
    this._showPanelCustomizationPlaceholders();
  },

  _onDragExit: function(aEvent) {
    __dumpDragData(aEvent);
    if (this._dragOverItem) {
      this._setDragActive(this._dragOverItem, false);
    }
  },

  _onDragEnd: function(aEvent) {
    __dumpDragData(aEvent);
    let document = aEvent.target.ownerDocument;
    document.documentElement.removeAttribute("customizing-movingItem");

    let documentId = document.documentElement.id;
    if (!aEvent.dataTransfer.mozTypesAt(0)) {
      return;
    }

    let {id: draggedItemId} =
      aEvent.dataTransfer.mozGetDataAt(kDragDataTypePrefix + documentId, 0);

    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);
    draggedWrapper.hidden = false;
    draggedWrapper.removeAttribute("mousedown");
    this._showPanelCustomizationPlaceholders();
  },

  _setDragActive: function(aItem, aValue, aWidth, aAtEnd) {
    if (!aItem) {
      return;
    }
    let node = aItem;
    let window = aItem.ownerDocument.defaultView;
    let direction = window.getComputedStyle(aItem, null).direction;
    let value = direction == "ltr" ? "left" : "right";
    if (aItem.localName == "toolbar" || aAtEnd) {
      value = direction == "ltr"? "right" : "left";
      if (aItem.localName == "toolbar") {
        node = aItem.lastChild;
      }
    }

    if (!node) {
      return;
    }

    if (aValue) {
      if (!node.hasAttribute("dragover")) {
        node.setAttribute("dragover", value);

        if (aWidth) {
          if (value == "left") {
            node.style.borderLeftWidth = aWidth;
          } else {
            node.style.borderRightWidth = aWidth;
          }
        }
      }
    } else {
      node.removeAttribute("dragover");
      
      
      node.style.removeProperty("border-left-width");
      node.style.removeProperty("border-right-width");
    }
  },

  _getCustomizableParent: function(aElement) {
    let areas = CustomizableUI.areas;
    areas.push(kPaletteId);
    while (aElement) {
      if (areas.indexOf(aElement.id) != -1) {
        return aElement;
      }
      aElement = aElement.parentNode;
    }
    return null;
  },

  _getDragOverNode: function(aElement, aAreaElement) {
    let expectedParent = aAreaElement.customizationTarget || aAreaElement;
    let targetNode = aElement;
    while (targetNode && targetNode.parentNode != expectedParent) {
      targetNode = targetNode.parentNode;
    }
    return targetNode || aElement;
  },

  _onMouseDown: function(aEvent) {
    LOG("_onMouseDown");
    let doc = aEvent.target.ownerDocument;
    doc.documentElement.setAttribute("customizing-movingItem", true);
    let item = this._getWrapper(aEvent.target);
    if (item) {
      item.setAttribute("mousedown", "true");
    }
  },

  _onMouseUp: function(aEvent) {
    LOG("_onMouseUp");
    let doc = aEvent.target.ownerDocument;
    doc.documentElement.removeAttribute("customizing-movingItem");
    let item = this._getWrapper(aEvent.target);
    if (item) {
      item.removeAttribute("mousedown");
    }
  },

  _getWrapper: function(aElement) {
    while (aElement && aElement.localName != "toolbarpaletteitem") {
      if (aElement.localName == "toolbar")
        return null;
      aElement = aElement.parentNode;
    }
    return aElement;
  },

  _onTabSelect: function(aEvent) {
    this._toggleCustomizationModeIfNecessary();
  },

  onLocationChange: function(aBrowser, aProgress, aRequest, aLocation, aFlags) {
    if (this.browser.selectedBrowser != aBrowser) {
      return;
    }

    this._toggleCustomizationModeIfNecessary();
  },

  




  _toggleCustomizationModeIfNecessary: function() {
    let browser = this.browser.selectedBrowser;
    if (browser.currentURI.spec == kAboutURI &&
        !this._customizing) {
      this.enter();
    } else if (browser.currentURI.spec != kAboutURI &&
               this._customizing) {
      this.exit();
    }
  },

  _showPanelCustomizationPlaceholders: function() {
    const kColumns = 3;
    this._removePanelCustomizationPlaceholders();
    let doc = this.document;
    let contents = this.panelUIContents;
    let visibleChildren = contents.querySelectorAll("toolbarpaletteitem:not([hidden])");
    let hangingItems = visibleChildren.length % kColumns;
    let newPlaceholders = kColumns - hangingItems;
    while (newPlaceholders--) {
      let placeholder = doc.createElement("toolbarpaletteitem");
      placeholder.classList.add(kPlaceholderClass);
      
      
      let placeholderChild = doc.createElement("toolbarbutton");
      placeholderChild.classList.add(kPlaceholderClass + "-child");
      placeholder.appendChild(placeholderChild);
      contents.appendChild(placeholder);
    }
  },

  _removePanelCustomizationPlaceholders: function() {
    let contents = this.panelUIContents;
    let oldPlaceholders = contents.getElementsByClassName(kPlaceholderClass);
    while (oldPlaceholders.length) {
      contents.removeChild(oldPlaceholders[0]);
    }
  }
};

function getPlaceForItem(aElement) {
  let place;
  let node = aElement;
  while (node && !place) {
    if (node.localName == "toolbar")
      place = "toolbar";
    else if (node.id == CustomizableUI.AREA_PANEL)
      place = "panel";
    else if (node.id == kPaletteId)
      place = "palette";

    node = node.parentNode;
  }
  return place;
}

function __dumpDragData(aEvent, caller) {
  let str = "Dumping drag data (CustomizeMode.jsm) {\n";
  str += "  type: " + aEvent["type"] + "\n";
  for (let el of ["target", "currentTarget", "relatedTarget"]) {
    if (aEvent[el]) {
      str += "  " + el + ": " + aEvent[el] + "(localName=" + aEvent[el].localName + "; id=" + aEvent[el].id + ")\n";
    }
  }
  for (let prop in aEvent.dataTransfer) {
    if (typeof aEvent.dataTransfer[prop] != "function") {
      str += "  dataTransfer[" + prop + "]: " + aEvent.dataTransfer[prop] + "\n";
    }
  }
  str += "}";
  LOG(str);
}
