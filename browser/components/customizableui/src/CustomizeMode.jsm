



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizeMode"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const kPrefCustomizationDebug = "browser.uiCustomization.debug";
const kPrefCustomizationAnimation = "browser.uiCustomization.disableAnimation";
const kPaletteId = "customization-palette";
const kAboutURI = "about:customizing";
const kDragDataTypePrefix = "text/toolbarwrapper-id/";
const kPlaceholderClass = "panel-customization-placeholder";
const kSkipSourceNodePref = "browser.uiCustomization.skipSourceNodeCheck";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/CustomizableUI.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DragPositionManager",
                                  "resource:///modules/DragPositionManager.jsm");

let gModuleName = "[CustomizeMode]";
#include logging.js

let gDisableAnimation = null;

function CustomizeMode(aWindow) {
  if (gDisableAnimation === null) {
    gDisableAnimation = Services.prefs.getPrefType(kPrefCustomizationAnimation) == Ci.nsIPrefBranch.PREF_BOOL &&
                        Services.prefs.getBoolPref(kPrefCustomizationAnimation);
  }
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
  _skipSourceNodeCheck: null,

  get panelUIContents() {
    return this.document.getElementById("PanelUI-contents");
  },

  toggle: function() {
    if (this._transitioning) {
      return;
    }
    if (this._customizing) {
      this.exit();
    } else {
      this.enter();
    }
  },

  enter: function() {
    if (this._customizing || this._transitioning) {
      return;
    }

    
    
    if (this.browser.selectedBrowser.currentURI.spec != kAboutURI) {
      this.window.switchToTabHavingURI(kAboutURI, true);
      return;
    }

    Task.spawn(function() {
      
      if (!this.window.gBrowserInit.delayedStartupFinished) {
        let delayedStartupDeferred = Promise.defer();
        let delayedStartupObserver = function(aSubject) {
          if (aSubject == this.window) {
            Services.obs.removeObserver(delayedStartupObserver, "browser-delayed-startup-finished");
            delayedStartupDeferred.resolve();
          }
        }.bind(this);
        Services.obs.addObserver(delayedStartupObserver, "browser-delayed-startup-finished", false);
        yield delayedStartupDeferred.promise;
      }

      
      
      if (this.document.documentElement._lightweightTheme)
        this.document.documentElement._lightweightTheme.disable();

      this.dispatchToolboxEvent("beforecustomization");
      CustomizableUI.notifyStartCustomizing(this.window);

      let window = this.window;
      let document = this.document;

      
      
      document.addEventListener("keypress", this);

      
      
      window.PanelUI.hide();
      window.PanelUI.menuButton.addEventListener("mousedown", this);
      window.PanelUI.menuButton.open = true;
      window.PanelUI.beginBatchUpdate();

      
      
      let panelHolder = document.getElementById("customization-panelHolder");
      panelHolder.appendChild(window.PanelUI.mainView);

      this._transitioning = true;

      let customizer = document.getElementById("customization-container");
      customizer.parentNode.selectedPanel = customizer;
      customizer.hidden = false;

      yield this._doTransition(true);

      
      this.dispatchToolboxEvent("customizationstarting");

      
      
      
      
      
      yield window.PanelUI.ensureReady(true);

      this._showPanelCustomizationPlaceholders();
      CustomizableUI.addListener(this);

      yield this._wrapToolbarItems();
      yield this.populatePalette();

      window.PanelUI.mainView.addEventListener("contextmenu", this, true);
      this.visiblePalette.addEventListener("dragstart", this, true);
      this.visiblePalette.addEventListener("dragover", this, true);
      this.visiblePalette.addEventListener("dragexit", this, true);
      this.visiblePalette.addEventListener("drop", this, true);
      this.visiblePalette.addEventListener("dragend", this, true);

      window.gNavToolbox.addEventListener("toolbarvisibilitychange", this);

      document.getElementById("PanelUI-help").setAttribute("disabled", true);
      document.getElementById("PanelUI-quit").setAttribute("disabled", true);

      this._updateResetButton();

      this._skipSourceNodeCheck = Services.prefs.getPrefType(kSkipSourceNodePref) == Ci.nsIPrefBranch.PREF_BOOL &&
                                  Services.prefs.getBoolPref(kSkipSourceNodePref);

      let customizableToolbars = document.querySelectorAll("toolbar[customizable=true]:not([autohide=true]):not([collapsed=true])");
      for (let toolbar of customizableToolbars)
        toolbar.setAttribute("customizing", true);

      window.PanelUI.endBatchUpdate();
      this._customizing = true;
      this._transitioning = false;
      this.dispatchToolboxEvent("customizationready");
    }.bind(this)).then(null, ERROR);
  },

  exit: function() {
    if (!this._customizing || this._transitioning) {
      return;
    }

    CustomizableUI.removeListener(this);

    this.document.removeEventListener("keypress", this);
    this.window.PanelUI.menuButton.removeEventListener("mousedown", this);
    this.window.PanelUI.menuButton.open = false;

    this.window.PanelUI.beginBatchUpdate();

    this._removePanelCustomizationPlaceholders();

    this._transitioning = true;

    let window = this.window;
    let document = this.document;
    let documentElement = document.documentElement;

    Task.spawn(function() {
      yield this.depopulatePalette();

      yield this._doTransition(false);

      let customizer = document.getElementById("customization-container");
      customizer.hidden = true;
      let browser = document.getElementById("browser");
      browser.parentNode.selectedPanel = browser;

      window.gNavToolbox.removeEventListener("toolbarvisibilitychange", this);

      DragPositionManager.stop();
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

      
      
      
      document.getElementById("PanelUI-help").removeAttribute("disabled");
      document.getElementById("PanelUI-quit").removeAttribute("disabled");

      
      
      
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

      if (this.document.documentElement._lightweightTheme)
        this.document.documentElement._lightweightTheme.enable();

      let customizableToolbars = document.querySelectorAll("toolbar[customizable=true]:not([autohide=true])");
      for (let toolbar of customizableToolbars)
        toolbar.removeAttribute("customizing");

      this.window.PanelUI.endBatchUpdate();
      this._changed = false;
      this._transitioning = false;
      this.dispatchToolboxEvent("aftercustomization");
      CustomizableUI.notifyEndCustomizing(this.window);
    }.bind(this)).then(null, ERROR);
  },

  
















  _doTransition: function(aEntering) {
    let deferred = Promise.defer();
    let deck = this.document.getElementById("tab-view-deck");

    let customizeTransitionEnd = function(aEvent) {
      if (aEvent.originalTarget != deck || aEvent.propertyName != "padding-bottom") {
        return;
      }
      deck.removeEventListener("transitionend", customizeTransitionEnd);

      if (!aEntering) {
        this.document.documentElement.removeAttribute("customize-exiting");
        this.document.documentElement.removeAttribute("customizing");
      } else {
        this.document.documentElement.setAttribute("customize-entered", true);
        this.document.documentElement.removeAttribute("customize-entering");
      }
      this.dispatchToolboxEvent("customization-transitionend", aEntering);

      deferred.resolve();
    }.bind(this);
    deck.addEventListener("transitionend", customizeTransitionEnd);

    if (gDisableAnimation) {
      deck.setAttribute("fastcustomizeanimation", true);
    }
    if (aEntering) {
      this.document.documentElement.setAttribute("customizing", true);
      this.document.documentElement.setAttribute("customize-entering", true);
    } else {
      this.document.documentElement.setAttribute("customize-exiting", true);
      this.document.documentElement.removeAttribute("customize-entered");
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
    }.bind(this)).then(null, ERROR);
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
    }.bind(this)).then(null, ERROR);
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
      aWrapper.remove();
      return null;
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
    }.bind(this)).then(null, ERROR);
  },

  _wrapItemsInArea: function(target) {
    for (let child of target.children) {
      if (this.isCustomizableItem(child)) {
        this.wrapToolbarItem(child, getPlaceForItem(child));
      }
    }
  },

  _unwrapItemsInArea: function(target) {
    for (let toolbarItem of target.children) {
      if (this.isWrappedToolbarItem(toolbarItem)) {
        this.unwrapToolbarItem(toolbarItem);
      }
    }
  },

  _unwrapToolbarItems: function() {
    return Task.spawn(function() {
      for (let target of this.areas) {
        for (let toolbarItem of target.children) {
          if (this.isWrappedToolbarItem(toolbarItem)) {
            yield this.deferredUnwrapToolbarItem(toolbarItem);
          }
        }
        target.removeEventListener("dragstart", this, true);
        target.removeEventListener("dragover", this, true);
        target.removeEventListener("dragexit", this, true);
        target.removeEventListener("drop", this, true);
        target.removeEventListener("dragend", this, true);
      }
    }.bind(this)).then(null, ERROR);
  },

  persistCurrentSets: function(aSetBeforePersisting)  {
    let document = this.document;
    let toolbars = document.querySelectorAll("toolbar[customizable='true'][currentset]");
    for (let toolbar of toolbars) {
      if (aSetBeforePersisting) {
        let set = toolbar.currentSet;
        toolbar.setAttribute("currentset", set);
      }
      
      document.persist(toolbar.id, "currentset");
    }
  },

  reset: function() {
    this.resetting = true;
    return Task.spawn(function() {
      this._removePanelCustomizationPlaceholders();
      yield this.depopulatePalette();
      yield this._unwrapToolbarItems();

      CustomizableUI.reset();

      yield this._wrapToolbarItems();
      yield this.populatePalette();

      this.persistCurrentSets(true);

      this._updateResetButton();
      this._showPanelCustomizationPlaceholders();
      this.resetting = false;
    }.bind(this)).then(null, ERROR);
  },

  _onToolbarVisibilityChange: function(aEvent) {
    let toolbar = aEvent.target;
    if (aEvent.detail.visible) {
      toolbar.setAttribute("customizing", "true");
    } else {
      toolbar.removeAttribute("customizing");
    }
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

  onWidgetBeforeDOMChange: function(aNodeToChange, aSecondaryNode, aContainer) {
    if (aContainer.ownerDocument.defaultView != this.window || this.resetting) {
      return;
    }
    if (aContainer.id == CustomizableUI.AREA_PANEL) {
      this._removePanelCustomizationPlaceholders();
    }
    
    
    if (aNodeToChange.parentNode) {
      this.unwrapToolbarItem(aNodeToChange.parentNode);
    }
    if (aSecondaryNode) {
      this.unwrapToolbarItem(aSecondaryNode.parentNode);
    }
  },

  onWidgetAfterDOMChange: function(aNodeToChange, aSecondaryNode, aContainer) {
    if (aContainer.ownerDocument.defaultView != this.window || this.resetting) {
      return;
    }
    
    if (aNodeToChange.parentNode) {
      let place = getPlaceForItem(aNodeToChange);
      this.wrapToolbarItem(aNodeToChange, place);
      if (aSecondaryNode) {
        this.wrapToolbarItem(aSecondaryNode, place);
      }
    } else {
      

      
      
      
      
      let widgetId = aNodeToChange.id;
      let widget = CustomizableUI.getWidget(widgetId);
      if (widget.provider == CustomizableUI.PROVIDER_API) {
        let paletteItem = this.makePaletteItem(widget, "palette");
        this.visiblePalette.appendChild(paletteItem);
      }
    }
    if (aContainer.id == CustomizableUI.AREA_PANEL) {
      this._showPanelCustomizationPlaceholders();
    }
  },

  onWidgetDestroyed: function(aWidgetId) {
    let wrapper = this.document.getElementById("wrapper-" + aWidgetId);
    if (wrapper) {
      let wasInPanel = wrapper.parentNode == this.panelUIContents;
      wrapper.remove();
      if (wasInPanel) {
        this._showPanelCustomizationPlaceholders();
      }
    }
  },

  onWidgetAfterCreation: function(aWidgetId, aArea) {
    
    
    if (!aArea) {
      let widgetNode = this.document.getElementById(aWidgetId);
      if (widgetNode) {
        this.wrapToolbarItem(widgetNode, "palette");
      } else {
        let widget = CustomizableUI.getWidget(aWidgetId);
        this.visiblePalette.appendChild(this.makePaletteItem(widget, "palette"));
      }
    }
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
      case "toolbarvisibilitychange":
        this._onToolbarVisibilityChange(aEvent);
        break;
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
        if (aEvent.button == 0 &&
            (aEvent.originalTarget == this.window.PanelUI.menuButton)) {
          this.exit();
          aEvent.preventDefault();
          return;
        }
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
    }
  },

  _onDragStart: function(aEvent) {
    __dumpDragData(aEvent);
    let item = aEvent.target;
    while (item && item.localName != "toolbarpaletteitem") {
      if (item.localName == "toolbar") {
        return;
      }
      item = item.parentNode;
    }

    if (item.classList.contains(kPlaceholderClass)) {
      return;
    }

    let dt = aEvent.dataTransfer;
    let documentId = aEvent.target.ownerDocument.documentElement.id;
    let draggedItem = item.firstChild;

    dt.mozSetDataAt(kDragDataTypePrefix + documentId, draggedItem.id, 0);
    dt.effectAllowed = "move";

    let itemRect = draggedItem.getBoundingClientRect();
    let itemCenter = {x: itemRect.left + itemRect.width / 2,
                      y: itemRect.top + itemRect.height / 2};
    this._dragOffset = {x: aEvent.clientX - itemCenter.x,
                        y: aEvent.clientY - itemCenter.y};

    
    
    this._initializeDragAfterMove = function() {
      
      
      
      if (this._customizing && !this._transitioning) {
        item.hidden = true;
        this._showPanelCustomizationPlaceholders();
        DragPositionManager.start(this.window);
      }
      this._initializeDragAfterMove = null;
      this.window.clearTimeout(this._dragInitializeTimeout);
    }.bind(this);
    this._dragInitializeTimeout = this.window.setTimeout(this._initializeDragAfterMove, 0);
  },

  _onDragOver: function(aEvent) {
    if (this._isUnwantedDragDrop(aEvent)) {
      return;
    }
    if (this._initializeDragAfterMove) {
      this._initializeDragAfterMove();
    }

    __dumpDragData(aEvent);

    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    if (!aEvent.dataTransfer.mozTypesAt(0)) {
      return;
    }

    let draggedItemId =
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

    let targetIsToolbar = CustomizableUI.getAreaType(targetArea.id) == "toolbar";
    let targetNode = this._getDragOverNode(aEvent, targetArea, targetIsToolbar, draggedItemId);

    
    
    let dragOverItem, dragValue;
    if (targetNode == targetArea.customizationTarget) {
      
      
      dragOverItem = targetNode.lastChild || targetNode;
      dragValue = "after";
    } else {
      let targetParent = targetNode.parentNode;
      let position = Array.indexOf(targetParent.children, targetNode);
      if (position == -1) {
        dragOverItem = targetParent.lastChild;
        dragValue = "after";
      } else {
        dragOverItem = targetParent.children[position];
        if (!targetIsToolbar) {
          dragValue = "before";
          dragOverItem = position == -1 ? targetParent.firstChild : targetParent.children[position];
        } else {
          
          let window = dragOverItem.ownerDocument.defaultView;
          let direction = window.getComputedStyle(dragOverItem, null).direction;
          let itemRect = dragOverItem.getBoundingClientRect();
          let dropTargetCenter = itemRect.left + (itemRect.width / 2);
          let existingDir = dragOverItem.getAttribute("dragover");
          if ((existingDir == "before") == (direction == "ltr")) {
            dropTargetCenter += (parseInt(dragOverItem.style.borderLeftWidth) || 0) / 2;
          } else {
            dropTargetCenter -= (parseInt(dragOverItem.style.borderRightWidth) || 0) / 2;
          }
          let before = direction == "ltr" ? aEvent.clientX < dropTargetCenter : aEvent.clientX > dropTargetCenter;
          dragValue = before ? "before" : "after";
        }
      }
    }

    if (this._dragOverItem && dragOverItem != this._dragOverItem) {
      this._cancelDragActive(this._dragOverItem, dragOverItem);
    }

    if (dragOverItem != this._dragOverItem || dragValue != dragOverItem.getAttribute("dragover")) {
      if (dragOverItem != targetArea.customizationTarget) {
        this._setDragActive(dragOverItem, dragValue, draggedItemId, targetIsToolbar);
      }
      this._dragOverItem = dragOverItem;
    }

    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  _onDragDrop: function(aEvent) {
    if (this._isUnwantedDragDrop(aEvent)) {
      return;
    }

    __dumpDragData(aEvent);
    this._initializeDragAfterMove = null;
    this.window.clearTimeout(this._dragInitializeTimeout);

    let targetArea = this._getCustomizableParent(aEvent.currentTarget);
    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    let draggedItemId =
      aEvent.dataTransfer.mozGetDataAt(kDragDataTypePrefix + documentId, 0);
    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);
    let originArea = this._getCustomizableParent(draggedWrapper);
    if (this._dragSizeMap) {
      this._dragSizeMap.clear();
    }
    
    if (!targetArea || !originArea) {
      return;
    }
    let targetNode = this._dragOverItem;
    let dropDir = targetNode.getAttribute("dragover");
    
    if (targetNode != targetArea && dropDir == "after") {
      if (targetNode.nextSibling) {
        targetNode = targetNode.nextSibling;
      } else {
        targetNode = targetArea;
      }
    }
    
    while (targetNode.classList.contains(kPlaceholderClass) && targetNode.nextSibling) {
      targetNode = targetNode.nextSibling;
    }
    if (targetNode.tagName == "toolbarpaletteitem") {
      targetNode = targetNode.firstChild;
    }

    this._cancelDragActive(this._dragOverItem, null, true);
    this._removePanelCustomizationPlaceholders();

    try {
      this._applyDrop(aEvent, targetArea, originArea, draggedItemId, targetNode);
    } catch (ex) {
      ERROR(ex, ex.stack);
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
      draggedItem = draggedItem.parentNode;

      
      if (aTargetNode == this.visiblePalette) {
        this.visiblePalette.appendChild(draggedItem);
      } else {
        
        this.visiblePalette.insertBefore(draggedItem, aTargetNode.parentNode);
      }
      return;
    }

    if (!CustomizableUI.canWidgetMoveToArea(aDraggedItemId, aTargetArea.id)) {
      return;
    }

    
    if (draggedItem.getAttribute("skipintoolbarset") == "true") {
      
      if (aTargetArea != aOriginArea) {
        return;
      }
      let place = draggedItem.parentNode.getAttribute("place");
      this.unwrapToolbarItem(draggedItem.parentNode);
      if (aTargetNode == aTargetArea.customizationTarget) {
        aTargetArea.customizationTarget.appendChild(draggedItem);
      } else {
        this.unwrapToolbarItem(aTargetNode.parentNode);
        aTargetArea.customizationTarget.insertBefore(draggedItem, aTargetNode);
        this.wrapToolbarItem(aTargetNode, place);
      }
      this.wrapToolbarItem(draggedItem, place);
      return;
    }

    
    
    if (aTargetNode == aTargetArea.customizationTarget) {
      CustomizableUI.addWidgetToArea(aDraggedItemId, aTargetArea.id);
      return;
    }

    
    
    let placement;
    let itemForPlacement = aTargetNode;
    
    while (itemForPlacement && itemForPlacement.getAttribute("skipintoolbarset") == "true" &&
           itemForPlacement.parentNode &&
           itemForPlacement.parentNode.nodeName == "toolbarpaletteitem") {
      itemForPlacement = itemForPlacement.parentNode.nextSibling;
      if (itemForPlacement && itemForPlacement.nodeName == "toolbarpaletteitem") {
        itemForPlacement = itemForPlacement.firstChild;
      }
    }
    if (itemForPlacement && !itemForPlacement.classList.contains(kPlaceholderClass)) {
      let targetNodeId = (itemForPlacement.nodeName == "toolbarpaletteitem") ?
                            itemForPlacement.firstChild && itemForPlacement.firstChild.id :
                            itemForPlacement.id;
      placement = CustomizableUI.getPlacementOfWidget(targetNodeId);
    }
    if (!placement) {
      LOG("Could not get a position for " + aTargetNode.nodeName + "#" + aTargetNode.id + "." + aTargetNode.className);
    }
    let position = placement ? placement.position : null;


    
    
    
    if (aTargetArea == aOriginArea) {
      CustomizableUI.moveWidgetWithinArea(aDraggedItemId, position);
    } else {
      CustomizableUI.addWidgetToArea(aDraggedItemId, aTargetArea.id, position);
    }
    
    if (aTargetNode != itemForPlacement) {
      let draggedWrapper = draggedItem.parentNode;
      let container = draggedWrapper.parentNode;
      container.insertBefore(draggedWrapper, aTargetNode.parentNode);
    }
  },

  _onDragExit: function(aEvent) {
    if (this._isUnwantedDragDrop(aEvent)) {
      return;
    }

    __dumpDragData(aEvent);

    
    
    
    
    
    if (this._dragOverItem && aEvent.target == aEvent.currentTarget) {
      this._cancelDragActive(this._dragOverItem);
      this._dragOverItem = null;
    }
  },

  _onDragEnd: function(aEvent) {
    if (this._isUnwantedDragDrop(aEvent)) {
      return;
    }
    this._initializeDragAfterMove = null;
    this.window.clearTimeout(this._dragInitializeTimeout);

    __dumpDragData(aEvent);
    let document = aEvent.target.ownerDocument;
    document.documentElement.removeAttribute("customizing-movingItem");

    let documentId = document.documentElement.id;
    if (!aEvent.dataTransfer.mozTypesAt(0)) {
      return;
    }

    let draggedItemId =
      aEvent.dataTransfer.mozGetDataAt(kDragDataTypePrefix + documentId, 0);

    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);
    draggedWrapper.hidden = false;
    draggedWrapper.removeAttribute("mousedown");
    this._showPanelCustomizationPlaceholders();
  },

  _isUnwantedDragDrop: function(aEvent) {
    
    
    
    
    
    if (this._skipSourceNodeCheck) {
      return false;
    }

    

    let mozSourceNode = aEvent.dataTransfer.mozSourceNode;
    
    
    return !mozSourceNode ||
           mozSourceNode.ownerDocument.defaultView != this.window;
  },

  _setDragActive: function(aItem, aValue, aDraggedItemId, aInToolbar) {
    if (!aItem) {
      return;
    }

    if (aItem.hasAttribute("dragover") != aValue) {
      aItem.setAttribute("dragover", aValue);

      let window = aItem.ownerDocument.defaultView;
      let draggedItem = window.document.getElementById(aDraggedItemId);
      if (!aInToolbar) {
        this._setPanelDragActive(aItem, draggedItem, aValue);
      } else {
        
        let width = this._getDragItemSize(aItem, draggedItem).width;
        let direction = window.getComputedStyle(aItem).direction;
        let prop, otherProp;
        
        if ((aValue == "before") == (direction == "ltr")) {
          prop = "borderLeftWidth";
          otherProp = "border-right-width";
        } else {
          
          prop = "borderRightWidth";
          otherProp = "border-left-width";
        }
        aItem.style[prop] = width + 'px';
        aItem.style.removeProperty(otherProp);
      }
    }
  },
  _cancelDragActive: function(aItem, aNextItem, aNoTransition) {
    let currentArea = this._getCustomizableParent(aItem);
    if (!currentArea) {
      return;
    }
    let isToolbar = CustomizableUI.getAreaType(currentArea.id) == "toolbar";
    if (isToolbar) {
      aItem.removeAttribute("dragover");
      
      
      aItem.style.removeProperty("border-left-width");
      aItem.style.removeProperty("border-right-width");
    } else  {
      if (aNextItem) {
        let nextArea = this._getCustomizableParent(aNextItem);
        if (nextArea == currentArea) {
          
          return;
        }
      }
      
      let positionManager = DragPositionManager.getManagerForArea(currentArea);
      positionManager.clearPlaceholders(currentArea, aNoTransition);
    }
  },

  _setPanelDragActive: function(aDragOverNode, aDraggedItem, aValue) {
    let targetArea = this._getCustomizableParent(aDragOverNode);
    let positionManager = DragPositionManager.getManagerForArea(targetArea);
    let draggedSize = this._getDragItemSize(aDragOverNode, aDraggedItem);
    let isWide = aDraggedItem.classList.contains(CustomizableUI.WIDE_PANEL_CLASS);
    positionManager.insertPlaceholder(targetArea, aDragOverNode, isWide, draggedSize);
  },

  _getDragItemSize: function(aDragOverNode, aDraggedItem) {
    
    if (!this._dragSizeMap)
      this._dragSizeMap = new WeakMap();
    if (!this._dragSizeMap.has(aDraggedItem))
      this._dragSizeMap.set(aDraggedItem, new WeakMap());
    let itemMap = this._dragSizeMap.get(aDraggedItem);
    let targetArea = this._getCustomizableParent(aDragOverNode);
    let currentArea = this._getCustomizableParent(aDraggedItem);
    
    let size = itemMap.get(targetArea);
    if (size)
      return size;

    
    let currentParent = aDraggedItem.parentNode;
    let currentSibling = aDraggedItem.nextSibling;
    const kAreaType = "cui-areatype";
    let areaType, currentType;

    if (targetArea != currentArea) {
      
      aDragOverNode.parentNode.insertBefore(aDraggedItem, aDragOverNode);
      
      areaType = CustomizableUI.getAreaType(targetArea.id);
      currentType = aDraggedItem.hasAttribute(kAreaType) &&
                    aDraggedItem.getAttribute(kAreaType);
      if (areaType)
        aDraggedItem.setAttribute(kAreaType, areaType);
      this.wrapToolbarItem(aDraggedItem, areaType || "palette");
      CustomizableUI.onWidgetDrag(aDraggedItem.id, targetArea.id);
    } else {
      aDraggedItem.parentNode.hidden = false;
    }

    
    let rect = aDraggedItem.parentNode.getBoundingClientRect();
    size = {width: rect.width, height: rect.height};
    
    itemMap.set(targetArea, size);

    if (targetArea != currentArea) {
      this.unwrapToolbarItem(aDraggedItem.parentNode);
      
      if (currentSibling)
        currentParent.insertBefore(aDraggedItem, currentSibling);
      else
        currentParent.appendChild(aDraggedItem);
      
      if (areaType) {
        if (currentType === false)
          aDraggedItem.removeAttribute(kAreaType);
        else
          aDraggedItem.setAttribute(kAreaType, currentType);
      }
      CustomizableUI.onWidgetDrag(aDraggedItem.id);
    } else {
      aDraggedItem.parentNode.hidden = true;
    }
    return size;
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

  _getDragOverNode: function(aEvent, aAreaElement, aInToolbar, aDraggedItemId) {
    let expectedParent = aAreaElement.customizationTarget || aAreaElement;
    
    if (!aEvent.clientX  && !aEvent.clientY) {
      return aEvent.target;
    }
    
    
    let dragX = aEvent.clientX - this._dragOffset.x;
    let dragY = aEvent.clientY - this._dragOffset.y;

    
    let bounds = expectedParent.getBoundingClientRect();
    dragX = Math.min(bounds.right, Math.max(dragX, bounds.left));
    dragY = Math.min(bounds.bottom, Math.max(dragY, bounds.top));

    let targetNode;
    if (aInToolbar) {
      targetNode = aAreaElement.ownerDocument.elementFromPoint(dragX, dragY);
      while (targetNode && targetNode.parentNode != expectedParent) {
        targetNode = targetNode.parentNode;
      }
    } else {
      let positionManager = DragPositionManager.getManagerForArea(aAreaElement);
      
      targetNode = positionManager.find(aAreaElement, dragX, dragY, aDraggedItemId);
    }
    return targetNode || aEvent.target;
  },

  _onMouseDown: function(aEvent) {
    LOG("_onMouseDown");
    let doc = aEvent.target.ownerDocument;
    doc.documentElement.setAttribute("customizing-movingItem", true);
    let item = this._getWrapper(aEvent.target);
    if (item && !item.classList.contains(kPlaceholderClass)) {
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
    let doc = this.document;
    let contents = this.panelUIContents;
    let narrowItemsAfterWideItem = 0;
    let node = contents.lastChild;
    while (node && !node.classList.contains(CustomizableUI.WIDE_PANEL_CLASS) &&
           (!node.firstChild || !node.firstChild.classList.contains(CustomizableUI.WIDE_PANEL_CLASS))) {
      if (!node.hidden && !node.classList.contains(kPlaceholderClass)) {
        narrowItemsAfterWideItem++;
      }
      node = node.previousSibling;
    }

    let orphanedItems = narrowItemsAfterWideItem % CustomizableUI.PANEL_COLUMN_COUNT;
    let placeholders = CustomizableUI.PANEL_COLUMN_COUNT - orphanedItems;

    let currentPlaceholderCount = contents.querySelectorAll("." + kPlaceholderClass).length;
    if (placeholders > currentPlaceholderCount) {
      while (placeholders-- > currentPlaceholderCount) {
        let placeholder = doc.createElement("toolbarpaletteitem");
        placeholder.classList.add(kPlaceholderClass);
        
        
        let placeholderChild = doc.createElement("toolbarbutton");
        placeholderChild.classList.add(kPlaceholderClass + "-child");
        placeholder.appendChild(placeholderChild);
        contents.appendChild(placeholder);
      }
    } else if (placeholders < currentPlaceholderCount) {
      while (placeholders++ < currentPlaceholderCount) {
        contents.querySelectorAll("." + kPlaceholderClass)[0].remove();
      }
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
  if (!gDebug) {
    return;
  }
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
