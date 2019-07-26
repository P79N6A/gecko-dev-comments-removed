



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizeMode"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const kPrefCustomizationDebug = "browser.uiCustomization.debug";
const kPaletteId = "customization-palette";
const kAboutURI = "about:customizing";

let gDebug = false;
try {
  gDebug = Services.prefs.getBoolPref(kPrefCustomizationDebug);
} catch (e) {}

function LOG(str) {
  if (gDebug) {
    Services.console.logStringMessage(str);
  }
}

function ERROR(aMsg) Cu.reportError("[CustomizeMode] " + aMsg);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/CustomizableUI.jsm");

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
    let window = this.window;
    let document = this.document;

    
    
    if (this.browser.selectedBrowser.currentURI.spec != kAboutURI) {
      this.window.switchToTabHavingURI(kAboutURI, true);
    }

    CustomizableUI.addListener(this);

    
    
    
    window.PanelUI.ensureRegistered();

    
    
    
    let deck = document.getElementById("tab-view-deck");
    deck.addEventListener("keypress", this, false);
    deck.addEventListener("click", this, false);

    
    
    window.PanelUI.menuButton.addEventListener("click", this, false);
    window.PanelUI.menuButton.disabled = true;

    
    let evt = document.createEvent("CustomEvent");
    evt.initCustomEvent("CustomizationStart", true, true, window);
    window.dispatchEvent(evt);

    let customizer = document.getElementById("customization-container");
    customizer.parentNode.selectedPanel = customizer;

    window.PanelUI.hide();
    
    
    let panelHolder = document.getElementById("customization-panelHolder");
    panelHolder.appendChild(window.PanelUI.mainView);


    let self = this;
    deck.addEventListener("transitionend", function customizeTransitionEnd() {
      deck.removeEventListener("transitionend", customizeTransitionEnd);

      
      self.areas = [];
      for (let area of CustomizableUI.areas) {
        let target = CustomizableUI.getCustomizeTargetForArea(area, window);
        target.addEventListener("dragstart", self);
        target.addEventListener("dragover", self);
        target.addEventListener("dragexit", self);
        target.addEventListener("drop", self);
        for (let child of target.children) {
          self.wrapToolbarItem(child, getPlaceForItem(child));
        }
        self.areas.push(target);
      }

      self.populatePalette();
    });

    this.visiblePalette.addEventListener("dragstart", this);
    this.visiblePalette.addEventListener("dragover", this);
    this.visiblePalette.addEventListener("dragexit", this);
    this.visiblePalette.addEventListener("drop", this);

    document.documentElement.setAttribute("customizing", true);
    this._customizing = true;
  },

  exit: function() {
    CustomizableUI.removeListener(this);

    let deck = this.document.getElementById("tab-view-deck");
    deck.removeEventListener("keypress", this, false);
    deck.removeEventListener("click", this, false);
    this.window.PanelUI.menuButton.removeEventListener("click", this, false);
    this.window.PanelUI.menuButton.disabled = false;

    this.depopulatePalette();

    this.visiblePalette.removeEventListener("dragstart", this);
    this.visiblePalette.removeEventListener("dragover", this);
    this.visiblePalette.removeEventListener("dragexit", this);
    this.visiblePalette.removeEventListener("drop", this);

    let window = this.window;
    let document = this.document;

    if (this._changed) {
      
      
      
      
      this.persistCurrentSets();
    }

    document.documentElement.removeAttribute("customizing");

    for (let target of this.areas) {
      for (let toolbarItem of target.children) {
        this.unwrapToolbarItem(toolbarItem);
      }
      target.removeEventListener("dragstart", this);
      target.removeEventListener("dragover", this);
      target.removeEventListener("dragexit", this);
      target.removeEventListener("drop", this);
    }

    
    this.areas = [];

    
    let evt = document.createEvent("CustomEvent");
    evt.initCustomEvent("CustomizationEnd", true, true, {changed: this._changed});
    window.dispatchEvent(evt);

    window.PanelUI.replaceMainView(window.PanelUI.mainView);

    let browser = document.getElementById("browser");
    browser.parentNode.selectedPanel = browser;

    if (this.browser.selectedBrowser.currentURI.spec == kAboutURI) {
      let custBrowser = this.browser.selectedBrowser;
      if (custBrowser.canGoBack) {
        
        custBrowser.goBack();
      } else {
        let customizationTab = this.browser.selectedTab;
        if (this.browser.browsers.length == 1) {
          this.window.BrowserOpenTab();
        }
        this.browser.removeTab(customizationTab);
      }
    }

    this._changed = false;
    this._customizing = false;
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
    let wrapper = this.visiblePalette.firstChild;

    while (wrapper) {
      let widgetNode = wrapper.firstChild;

      let provider = CustomizableUI.getWidget(widgetNode.id);
      
      if (provider = CustomizableUI.PROVIDER_XUL) {
        if (wrapper.hasAttribute("itemdisabled")) {
          widgetNode.disabled = true;
        }

        if (wrapper.hasAttribute("itemchecked")) {
          widgetNode.checked = true;
        }

        if (wrapper.hasAttribute("itemcommand")) {
          let commandID = wrapper.getAttribute("itemcommand");
          widgetNode.setAttribute("command", commandID);

          
          let command = this.document.getElementById(commandID);
          if (command && command.hasAttribute("disabled")) {
            widgetNode.setAttribute("disabled",
                                    command.getAttribute("disabled"));
          }
        }

        this._stowedPalette.appendChild(widgetNode);
      } else if (provider = CustomizableUI.PROVIDER_API) {
        
        
        
        
        
        
      }

      this.visiblePalette.removeChild(wrapper);
      wrapper = this.visiblePalette.firstChild;
    }
    this.window.gNavToolbox.palette = this._stowedPalette;
  },

  wrapToolbarItem: function(aNode, aPlace) {
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

  
  
  
  
  persistCurrentSets: function()  {
    let document = this.document;
    let toolbars = document.querySelectorAll("toolbar");

    for (let toolbar of toolbars) {
      
      toolbar.setAttribute("currentset", toolbar.currentSet);
      
      document.persist(toolbar.id, "currentset");
    }
  },

  reset: function() {
    CustomizableUI.reset();
  },

  onWidgetMoved: function(aWidgetId, aArea, aOldPosition, aNewPosition) {
    this._changed = true;
  },

  onWidgetAdded: function(aWidgetId, aArea, aPosition) {
    this._changed = true;
  },

  onWidgetRemoved: function(aWidgetId, aArea) {
    this._changed = true;
  },

  onWidgetCreated: function(aWidgetId) {
  },

  onWidgetDestroyed: function(aWidgetId) {
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
      if (item.localName == "toolbar") {
        return;
      }
      item = item.parentNode;
    }

    let dt = aEvent.dataTransfer;
    let documentId = aEvent.target.ownerDocument.documentElement.id;
    dt.setData("text/toolbarwrapper-id/" + documentId, item.firstChild.id);
    dt.effectAllowed = "move";
  },

  _onDragOver: function(aEvent) {
    __dumpDragData(aEvent);

    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    if (!aEvent.dataTransfer.types.contains("text/toolbarwrapper-id/"
                                            + documentId.toLowerCase())) {
      return;
    }

    let draggedItemId = aEvent.dataTransfer.getData("text/toolbarwrapper-id/" + documentId);
    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);
    let targetNode = aEvent.target;
    let targetParent = targetNode.parentNode;
    let targetArea = this._getCustomizableParent(targetNode);
    let originArea = this._getCustomizableParent(draggedWrapper);

    
    if (!targetArea || !originArea) {
      gCurrentDragOverItem = null;
      return;
    }

    
    
    let position = Array.indexOf(targetParent.children, targetNode);
    let dragOverItem = position == -1 ? targetParent.lastChild : targetParent.children[position];

    if (this._dragOverItem && dragOverItem != this._dragOverItem) {
      this._setDragActive(this._dragOverItem, false);
    }

    
    if (targetArea.localName == "toolbar") {
      this._setDragActive(dragOverItem, true);
    }
    this._dragOverItem = dragOverItem;

    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  _onDragDrop: function(aEvent) {
    __dumpDragData(aEvent);

    this._setDragActive(this._dragOverItem, false);

    let document = aEvent.target.ownerDocument;
    let documentId = document.documentElement.id;
    let draggedItemId = aEvent.dataTransfer.getData("text/toolbarwrapper-id/" + documentId);
    let draggedWrapper = document.getElementById("wrapper-" + draggedItemId);

    draggedWrapper.removeAttribute("mousedown");

    let targetNode = aEvent.target;
    let targetParent = targetNode.parentNode;
    let targetArea = this._getCustomizableParent(targetNode);
    let originArea = this._getCustomizableParent(draggedWrapper);

    
    if (!targetArea || !originArea) {
      return;
    }

    
    
    if (draggedWrapper == targetNode) {
      return;
    }

    
    
    if (targetArea.id == kPaletteId) {
      if (originArea.id !== kPaletteId) {
        let widget = this.unwrapToolbarItem(draggedWrapper);
        CustomizableUI.removeWidgetFromArea(draggedItemId);
        draggedWrapper = this.wrapToolbarItem(widget, "palette");
      }

      
      if (targetNode == this.visiblePalette) {
        this.visiblePalette.appendChild(draggedWrapper);
      } else {
        this.visiblePalette.insertBefore(draggedWrapper, targetNode);
      }
      return;
    }

    
    
    if (targetNode == targetArea.customizationTarget) {
      let widget = this.unwrapToolbarItem(draggedWrapper);
      CustomizableUI.addWidgetToArea(draggedItemId, targetArea.id);
      this.wrapToolbarItem(widget, getPlaceForItem(targetNode));
      return;
    }

    
    
    let placement = CustomizableUI.getPlacementOfWidget(targetNode.firstChild.id);
    if (!placement) {
      ERROR("Could not get a position for " + targetNode.firstChild.id);
      return;
    }

    let position = placement.position;

    
    
    
    if (targetArea == originArea) {
      let properPlace = getPlaceForItem(targetNode);
      
      
      
      let widget = this.unwrapToolbarItem(draggedWrapper);
      let targetWidget = this.unwrapToolbarItem(targetNode);
      CustomizableUI.moveWidgetWithinArea(draggedItemId, position);
      this.wrapToolbarItem(targetWidget, properPlace);
      this.wrapToolbarItem(widget, properPlace);
      return;
    }

    
    
    
    
    let properPlace = getPlaceForItem(targetNode);
    let widget = this.unwrapToolbarItem(draggedWrapper);
    let targetWidget = this.unwrapToolbarItem(targetNode);
    CustomizableUI.addWidgetToArea(draggedItemId, targetArea.id, position);
    this.wrapToolbarItem(targetWidget, properPlace);
    draggedWrapper = this.wrapToolbarItem(widget, properPlace);
  },

  _onDragExit: function(aEvent) {
    if (this._dragOverItem) {
      this._setDragActive(this._dragOverItem, false);
    }
  },

  
  _setDragActive: function(aItem, aValue) {
    let node = aItem;
    let window = aItem.ownerDocument.defaultView;
    let direction = window.getComputedStyle(aItem, null).direction;
    let value = direction == "ltr"? "left" : "right";
    if (aItem.localName == "toolbar") {
      node = aItem.lastChild;
      value = direction == "ltr"? "right" : "left";
    }

    if (!node) {
      return;
    }

    if (aValue) {
      if (!node.hasAttribute("dragover")) {
        node.setAttribute("dragover", value);
      }
    } else {
      node.removeAttribute("dragover");
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

  _onMouseDown: function(aEvent) {
    let item = this._getWrapper(aEvent.target);
    if (item) {
      item.setAttribute("mousedown", "true");
    }
  },

  _onMouseUp: function(aEvent) {
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
