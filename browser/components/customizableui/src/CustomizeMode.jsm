



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizeMode"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const kPrefCustomizationDebug = "browser.uiCustomization.debug";
const kPaletteId = "customization-palette";
const kAboutURI = "about:customizing";
const kDragDataTypePrefix = "text/toolbarwrapper-id/";
const kPlaceholderClass = "panel-customization-placeholder";


const kColumnsInMenuPanel = 3;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/CustomizableUI.jsm");
Cu.import("resource://gre/modules/LightweightThemeManager.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

let gModuleName = "[CustomizeMode]";
#include logging.js

function CustomizeMode(aWindow) {
  this.window = aWindow;
  this.document = aWindow.document;
  this.browser = aWindow.gBrowser;

  
  
  
  
  this.visiblePalette = this.document.getElementById(kPaletteId);
};

CustomizeMode.prototype = {
  _changed: false,
  _transitioning: false,
  window: null,
  document: null,
  
  areas: null,
  
  
  
  
  
  
  
  _stowedPalette: null,
  _dragOverItem: null,
  _customizing: false,

  get panelUIContents() {
    return this.document.getElementById("PanelUI-contents");
  },

  enter: function() {
    if (this._customizing || this._transitioning) {
      return;
    }

    
    
    if (this.browser.selectedBrowser.currentURI.spec != kAboutURI) {
      this.window.switchToTabHavingURI(kAboutURI, true);
      return;
    }

    
    
    LightweightThemeManager.temporarilyToggleTheme(false);

    this.dispatchToolboxEvent("beforecustomization");

    let window = this.window;
    let document = this.document;

    CustomizableUI.addListener(this);

    
    
    
    let deck = document.getElementById("tab-view-deck");
    deck.addEventListener("keypress", this, false);
    deck.addEventListener("click", this, false);

    
    
    window.PanelUI.hide();
    window.PanelUI.menuButton.addEventListener("click", this, false);
    window.PanelUI.menuButton.open = true;
    window.PanelUI.beginBatchUpdate();

    
    
    let panelHolder = document.getElementById("customization-panelHolder");
    panelHolder.appendChild(window.PanelUI.mainView);

    this._transitioning = true;

    let customizer = document.getElementById("customization-container");
    customizer.parentNode.selectedPanel = customizer;
    customizer.hidden = false;

    Task.spawn(function() {
      yield this._doTransition(true);

      
      this.dispatchToolboxEvent("customizationstarting");

      
      
      
      
      
      window.PanelUI.ensureRegistered(true);

      this._showPanelCustomizationPlaceholders();

      yield this._wrapToolbarItems();
      yield this.populatePalette();

      window.PanelUI.mainView.addEventListener("contextmenu", this, true);
      this.visiblePalette.addEventListener("dragstart", this, true);
      this.visiblePalette.addEventListener("dragover", this, true);
      this.visiblePalette.addEventListener("dragexit", this, true);
      this.visiblePalette.addEventListener("drop", this, true);
      this.visiblePalette.addEventListener("dragend", this, true);

      
      
      window.PanelUI.menuButton.addEventListener("click", this, false);
      window.PanelUI.menuButton.disabled = true;

      this._updateResetButton();

      let customizableToolbars = document.querySelectorAll("toolbar[customizable=true]:not([autohide=true]):not([collapsed=true])");
      for (let toolbar of customizableToolbars)
        toolbar.setAttribute("customizing", true);

      window.PanelUI.endBatchUpdate();
      this._customizing = true;
      this._transitioning = false;
      this.dispatchToolboxEvent("customizationready");
    }.bind(this));
  },

  exit: function() {
    if (!this._customizing || this._transitioning) {
      return;
    }

    CustomizableUI.removeListener(this);

    let deck = this.document.getElementById("tab-view-deck");
    deck.removeEventListener("keypress", this, false);
    deck.removeEventListener("click", this, false);
    this.window.PanelUI.menuButton.removeEventListener("click", this, false);
    this.window.PanelUI.menuButton.open = false;

    this.window.PanelUI.beginBatchUpdate();

    this._removePanelCustomizationPlaceholders();

    this._transitioning = true;

    let window = this.window;
    let document = this.document;
    let documentElement = document.documentElement;

    Task.spawn(function() {
      yield this._doTransition(false);

      let customizer = document.getElementById("customization-container");
      customizer.hidden = true;
      let browser = document.getElementById("browser");
      browser.parentNode.selectedPanel = browser;

      yield this.depopulatePalette();

      window.PanelUI.mainView.removeEventListener("contextmenu", this, true);
      this.visiblePalette.removeEventListener("dragstart", this, true);
      this.visiblePalette.removeEventListener("dragover", this, true);
      this.visiblePalette.removeEventListener("dragexit", this, true);
      this.visiblePalette.removeEventListener("drop", this, true);
      this.visiblePalette.removeEventListener("dragend", this, true);

      yield this._unwrapToolbarItems();

      if (this._changed) {
        
        
        
        
        this.persistCurrentSets();
      }

      
      this.areas = [];

      
      
      this.dispatchToolboxEvent("customizationending");
      window.PanelUI.setMainView(window.PanelUI.mainView);
      window.PanelUI.menuButton.disabled = false;

      
      
      
      this._customizing = false;

      if (this.browser.selectedBrowser.currentURI.spec == kAboutURI) {
        let custBrowser = this.browser.selectedBrowser;
        if (custBrowser.canGoBack) {
          
          custBrowser.goBack();
        } else {
          
          
          
          if (window.getTopWin(true) == window) {
            let customizationTab = this.browser.selectedTab;
            if (this.browser.browsers.length == 1) {
              window.BrowserOpenTab();
            }
            this.browser.removeTab(customizationTab);
          }
        }
      }

      LightweightThemeManager.temporarilyToggleTheme(true);

      let customizableToolbars = document.querySelectorAll("toolbar[customizable=true]:not([autohide=true])");
      for (let toolbar of customizableToolbars)
        toolbar.removeAttribute("customizing");

      this.window.PanelUI.endBatchUpdate();
      this._changed = false;
      this._transitioning = false;
      this.dispatchToolboxEvent("aftercustomization");
    }.bind(this));
  },

  _doTransition: function(aEntering) {
    let deferred = Promise.defer();

    let deck = this.document.getElementById("tab-view-deck");
    deck.addEventListener("transitionend", function customizeTransitionEnd(aEvent) {
      if (aEvent.originalTarget != deck || aEvent.propertyName != "padding-top") {
        return;
      }
      deck.removeEventListener("transitionend", customizeTransitionEnd);

      if (!aEntering) {
        this.document.documentElement.removeAttribute("customize-exiting");
      }

      deferred.resolve();
    }.bind(this));

    if (aEntering) {
      this.document.documentElement.setAttribute("customizing", true);
    } else {
      this.document.documentElement.setAttribute("customize-exiting", true);
      this.document.documentElement.removeAttribute("customizing");
    }
    return deferred.promise;
  },

  dispatchToolboxEvent: function(aEventType, aDetails={}) {
    let evt = this.document.createEvent("CustomEvent");
    evt.initCustomEvent(aEventType, true, true, {changed: this._changed});
    let result = this.window.gNavToolbox.dispatchEvent(evt);
  },

  addToToolbar: function(aNode) {
    CustomizableUI.addWidgetToArea(aNode.id, CustomizableUI.AREA_NAVBAR);
  },

  removeFromPanel: function(aNode) {
    CustomizableUI.removeWidgetFromArea(aNode.id);
  },

  populatePalette: function() {
    let fragment = this.document.createDocumentFragment();
    let toolboxPalette = this.window.gNavToolbox.palette;

    return Task.spawn(function() {
      let unusedWidgets = CustomizableUI.getUnusedWidgets(toolboxPalette);
      for (let widget of unusedWidgets) {
        let paletteItem = this.makePaletteItem(widget, "palette");
        fragment.appendChild(paletteItem);
      }

      this.visiblePalette.appendChild(fragment);
      this._stowedPalette = this.window.gNavToolbox.palette;
      this.window.gNavToolbox.palette = this.visiblePalette;
    }.bind(this));
  },

  
  
  
  
  makePaletteItem: function(aWidget, aPlace) {
    let widgetNode = aWidget.forWindow(this.window).node;
    let wrapper = this.createWrapper(widgetNode, aPlace);
    wrapper.appendChild(widgetNode);
    return wrapper;
  },

  depopulatePalette: function() {
    return Task.spawn(function() {
      this.visiblePalette.hidden = true;
      let paletteChild = this.visiblePalette.firstChild;
      let nextChild;
      while (paletteChild) {
        nextChild = paletteChild.nextElementSibling;
        let provider = CustomizableUI.getWidget(paletteChild.id).provider;
        if (provider == CustomizableUI.PROVIDER_XUL) {
          let unwrappedPaletteItem =
            yield this.deferredUnwrapToolbarItem(paletteChild);
          this._stowedPalette.appendChild(unwrappedPaletteItem);
        } else if (provider == CustomizableUI.PROVIDER_API) {
          
          
          
          
          
          
        } else if (provider == CustomizableUI.PROVIDER_SPECIAL) {
          this.visiblePalette.removeChild(paletteChild);
        }

        paletteChild = nextChild;
      }
      this.visiblePalette.hidden = false;
      this.window.gNavToolbox.palette = this._stowedPalette;
    }.bind(this));
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

  deferredWrapToolbarItem: function(aNode, aPlace) {
    let deferred = Promise.defer();

    dispatchFunction(function() {
      let wrapper = this.wrapToolbarItem(aNode, aPlace);
      deferred.resolve(wrapper);
    }.bind(this))

    return deferred.promise;
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

  deferredUnwrapToolbarItem: function(aWrapper) {
    let deferred = Promise.defer();
    dispatchFunction(function() {
      deferred.resolve(this.unwrapToolbarItem(aWrapper));
    }.bind(this));
    return deferred.promise;
  },

  unwrapToolbarItem: function(aWrapper) {
    if (aWrapper.nodeName != "toolbarpaletteitem") {
      return aWrapper;
    }
    aWrapper.removeEventListener("mousedown", this);
    aWrapper.removeEventListener("mouseup", this);

    let toolbarItem = aWrapper.firstChild;
    if (!toolbarItem) {
      ERROR("no toolbarItem child for " + aWrapper.tagName + "#" + aWrapper.id);
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
    
    return Task.spawn(function() {
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
            yield this.deferredWrapToolbarItem(child, getPlaceForItem(child));
          }
        }
        this.areas.push(target);
      }
    }.bind(this));
  },

  
  _wrapToolbarItemsSync: function() {
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
    return Task.spawn(function() {
      this._unwrapToolbarItemsSync();
    }.bind(this));
  },

  
  _unwrapToolbarItemsSync: function() {
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
    return Task.spawn(function() {
      this._removePanelCustomizationPlaceholders();
      yield this.depopulatePalette();
      yield this._unwrapToolbarItems();

      CustomizableUI.reset();

      yield this._wrapToolbarItems();
      yield this.populatePalette();

      let document = this.document;
      let toolbars = document.querySelectorAll("toolbar[customizable='true']");
      for (let toolbar of toolbars) {
        let set = toolbar.currentSet;
        toolbar.removeAttribute("currentset");
        LOG("[RESET] Removing currentset of " + toolbar.id);
        
        document.persist(toolbar.id, "currentset");
      }

      this._updateResetButton();
      this._showPanelCustomizationPlaceholders();
    }.bind(this));
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
    this._updateResetButton();
    this.dispatchToolboxEvent("customizationchange");
  },

  _updateResetButton: function() {
    let btn = this.document.getElementById("customization-reset-button");
    btn.disabled = CustomizableUI.inDefaultState;
  },

  handleEvent: function(aEvent) {
    switch(aEvent.type) {
      case "contextmenu":
        aEvent.preventDefault();
        aEvent.stopPropagation();
        break;
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

    let targetArea = this._getCustomizableParent(aEvent.currentTarget);
    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    let {id: draggedItemId} =
      aEvent.dataTransfer.mozGetDataAt(kDragDataTypePrefix + documentId, 0);
    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);
    let originArea = this._getCustomizableParent(draggedWrapper);
    
    if (!targetArea || !originArea) {
      return;
    }
    let targetNode = this._getDragOverNode(aEvent.target, targetArea);
    if (targetNode.tagName == "toolbarpaletteitem") {
      targetNode = targetNode.firstChild;
    }

    this._setDragActive(this._dragOverItem, false);
    this._removePanelCustomizationPlaceholders();

    
    this._unwrapToolbarItemsSync();
    let paletteChild = this.visiblePalette.firstChild;
    let nextChild;
    while (paletteChild) {
      nextChild = paletteChild.nextElementSibling;
      this.unwrapToolbarItem(paletteChild);
      paletteChild = nextChild;
    }

    try {
      this._applyDrop(aEvent, targetArea, originArea, draggedItemId, targetNode);
    } catch (ex) {
      ERROR(ex, ex.stack);
    }

    
    this._wrapToolbarItemsSync();
    
    let paletteChild = this.visiblePalette.firstChild;
    let nextChild;
    while (paletteChild) {
      nextChild = paletteChild.nextElementSibling;
      this.wrapToolbarItem(paletteChild, "palette");
      paletteChild = nextChild;
    }
    this._showPanelCustomizationPlaceholders();
  },

  _applyDrop: function(aEvent, aTargetArea, aOriginArea, aDraggedItemId, aTargetNode) {
    let document = aEvent.target.ownerDocument;
    let draggedItem = document.getElementById(aDraggedItemId);
    draggedItem.hidden = false;
    draggedItem.removeAttribute("mousedown");

    
    
    if (draggedItem == aTargetNode) {
      return;
    }

    
    
    if (aTargetArea.id == kPaletteId) {
      if (aOriginArea.id !== kPaletteId) {
        if (!CustomizableUI.isWidgetRemovable(aDraggedItemId)) {
          return;
        }

        CustomizableUI.removeWidgetFromArea(aDraggedItemId);
      }

      
      if (aTargetNode == this.visiblePalette) {
        this.visiblePalette.appendChild(draggedItem);
      } else {
        this.visiblePalette.insertBefore(draggedItem, aTargetNode);
      }
      return;
    }

    if (!CustomizableUI.canWidgetMoveToArea(aDraggedItemId, aTargetArea.id)) {
      return;
    }

    
    
    if (aTargetNode == aTargetArea.customizationTarget) {
      CustomizableUI.addWidgetToArea(aDraggedItemId, aTargetArea.id);
      return;
    }

    
    
    let placement;
    if (!aTargetNode.classList.contains(kPlaceholderClass)) {
      let targetNodeId = (aTargetNode.nodeName == "toolbarpaletteitem") ?
                            aTargetNode.firstChild && aTargetNode.firstChild.id :
                            aTargetNode.id;
      placement = CustomizableUI.getPlacementOfWidget(targetNodeId);
    }
    if (!placement) {
      LOG("Could not get a position for " + aTargetNode + "#" + aTargetNode.id + "." + aTargetNode.className);
    }
    let position = placement ? placement.position :
                               aTargetArea.childElementCount;


    
    
    
    if (aTargetArea == aOriginArea) {
      CustomizableUI.moveWidgetWithinArea(aDraggedItemId, position);
      return;
    }

    CustomizableUI.addWidgetToArea(aDraggedItemId, aTargetArea.id, position);
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

  _showPanelCustomizationPlaceholders: function() {
    this._removePanelCustomizationPlaceholders();
    let doc = this.document;
    let contents = this.panelUIContents;
    let visibleCombinedButtons = contents.querySelectorAll("toolbarpaletteitem:not([hidden]) > .panel-combined-item");
    let visibleChildren = contents.querySelectorAll("toolbarpaletteitem:not([hidden])");
    
    
    let hangingItems = (visibleChildren.length - visibleCombinedButtons.length) % kColumnsInMenuPanel;
    let newPlaceholders = kColumnsInMenuPanel - hangingItems;
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

function dispatchFunction(aFunc) {
  Services.tm.currentThread.dispatch(aFunc, Ci.nsIThread.DISPATCH_NORMAL);
}
