





const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["ResponsiveUIManager"];

const MIN_WIDTH = 50;
const MIN_HEIGHT = 50;

const MAX_WIDTH = 10000;
const MAX_HEIGHT = 10000;

let ResponsiveUIManager = {
  







  toggle: function(aWindow, aTab) {
    if (aTab.__responsiveUI) {
      aTab.__responsiveUI.close();
    } else {
      aTab.__responsiveUI = new ResponsiveUI(aWindow, aTab);
    }
  },

  







  handleGcliCommand: function(aWindow, aTab, aCommand, aArgs) {
    switch (aCommand) {
      case "resize to":
        if (!aTab.__responsiveUI) {
          aTab.__responsiveUI = new ResponsiveUI(aWindow, aTab);
        }
        aTab.__responsiveUI.setSize(aArgs.width, aArgs.height);
        break;
      case "resize on":
        if (!aTab.__responsiveUI) {
          aTab.__responsiveUI = new ResponsiveUI(aWindow, aTab);
        }
        break;
      case "resize off":
        if (aTab.__responsiveUI) {
          aTab.__responsiveUI.close();
        }
        break;
      case "resize toggle":
          this.toggle(aWindow, aTab);
      default:
    }
  },
}

let presets = [
  
  {key: "320x480", width: 320, height: 480},    
  {key: "360x640", width: 360, height: 640},    

  
  {key: "768x1024", width: 768, height: 1024},   
  {key: "800x1280", width: 800, height: 1280},   

  
  {key: "980x1280", width: 980, height: 1280},

  
  {key: "1280x600", width: 1280, height: 600},
  {key: "1920x900", width: 1920, height: 900},
];

function ResponsiveUI(aWindow, aTab)
{
  this.mainWindow = aWindow;
  this.tab = aTab;
  this.tabContainer = aWindow.gBrowser.tabContainer;
  this.browser = aTab.linkedBrowser;
  this.chromeDoc = aWindow.document;
  this.container = aWindow.gBrowser.getBrowserContainer(this.browser);
  this.stack = this.container.querySelector(".browserStack");

  
  if (Services.prefs.prefHasUserValue("devtools.responsiveUI.presets")) {
    try {
      presets = JSON.parse(Services.prefs.getCharPref("devtools.responsiveUI.presets"));
    } catch(e) {
      
      Cu.reportError("Could not parse pref `devtools.responsiveUI.presets`: " + e);
    }
  }

  if (Array.isArray(presets)) {
    this.presets = [{key: "custom", custom: true}].concat(presets)
  } else {
    Cu.reportError("Presets value (devtools.responsiveUI.presets) is malformated.");
    this.presets = [{key: "custom", custom: true}];
  }

  try {
    let width = Services.prefs.getIntPref("devtools.responsiveUI.customWidth");
    let height = Services.prefs.getIntPref("devtools.responsiveUI.customHeight");
    this.presets[0].width = Math.min(MAX_WIDTH, width);
    this.presets[0].height = Math.min(MAX_HEIGHT, height);

    let key = Services.prefs.getCharPref("devtools.responsiveUI.currentPreset");
    let idx = this.getPresetIdx(key);
    this.currentPreset = (idx == -1 ? 0 : idx);
  } catch(e) {
    
    let bbox = this.stack.getBoundingClientRect();

    this.presets[0].width = bbox.width - 40; 
    this.presets[0].height = bbox.height - 80; 
    this.currentPreset = 0; 
  }

  this.container.setAttribute("responsivemode", "true");
  this.stack.setAttribute("responsivemode", "true");

  
  this.bound_presetSelected = this.presetSelected.bind(this);
  this.bound_rotate = this.rotate.bind(this);
  this.bound_startResizing = this.startResizing.bind(this);
  this.bound_stopResizing = this.stopResizing.bind(this);
  this.bound_onDrag = this.onDrag.bind(this);
  this.bound_onKeypress = this.onKeypress.bind(this);

  
  this.tab.addEventListener("TabClose", this);
  this.tabContainer.addEventListener("TabSelect", this);
  this.mainWindow.document.addEventListener("keypress", this.bound_onKeypress, false);

  this.buildUI();
  this.checkMenus();

  this.inspectorWasOpen = this.mainWindow.InspectorUI.isInspectorOpen;

  try {
    if (Services.prefs.getBoolPref("devtools.responsiveUI.rotate")) {
      this.rotate();
    }
  } catch(e) {}
}

ResponsiveUI.prototype = {
  _transitionsEnabled: true,
  get transitionsEnabled() this._transitionsEnabled,
  set transitionsEnabled(aValue) {
    this._transitionsEnabled = aValue;
    if (aValue && !this._resizing && this.stack.hasAttribute("responsivemode")) {
      this.stack.removeAttribute("notransition");
    } else if (!aValue) {
      this.stack.setAttribute("notransition", "true");
    }
  },

  


  close: function RUI_unload() {
    if (this.closing)
      return;
    this.closing = true;

    this.unCheckMenus();
    
    let style = "max-width: none;" +
                "min-width: 0;" +
                "max-height: none;" +
                "min-height: 0;";
    this.stack.setAttribute("style", style);

    if (this.isResizing)
      this.stopResizing();

    this.saveCurrentPreset();

    
    this.mainWindow.document.removeEventListener("keypress", this.bound_onKeypress, false);
    this.menulist.removeEventListener("select", this.bound_presetSelected, true);
    this.tab.removeEventListener("TabClose", this);
    this.tabContainer.removeEventListener("TabSelect", this);
    this.rotatebutton.removeEventListener("command", this.bound_rotate, true);

    
    this.container.removeChild(this.toolbar);
    this.stack.removeChild(this.resizer);
    this.stack.removeChild(this.resizeBar);

    
    this.container.removeAttribute("responsivemode");
    this.stack.removeAttribute("responsivemode");

    delete this.tab.__responsiveUI;
  },

  





   getPresetIdx: function RUI_getPresetIdx(aKey) {
     for (let i = 0; i < this.presets.length; i++) {
       if (this.presets[i].key == aKey) {
         return i;
       }
     }
     return -1;
   },

  




  onKeypress: function RUI_onKeypress(aEvent) {
    if (aEvent.keyCode == this.mainWindow.KeyEvent.DOM_VK_ESCAPE &&
        this.mainWindow.gBrowser.selectedBrowser == this.browser) {

      
      
      

      let isInspectorOpen = this.mainWindow.InspectorUI.isInspectorOpen;
      if (this.inspectorWasOpen || !isInspectorOpen) {
        aEvent.preventDefault();
        aEvent.stopPropagation();
        this.close();
      }
    }
  },

  


  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "TabClose":
        this.close();
        break;
      case "TabSelect":
        if (this.tab.selected) {
          this.checkMenus();
        } else if (!this.mainWindow.gBrowser.selectedTab.responsiveUI) {
          this.unCheckMenus();
        }
        break;
    }
  },

  


   checkMenus: function RUI_checkMenus() {
     this.chromeDoc.getElementById("Tools:ResponsiveUI").setAttribute("checked", "true");
   },

  


   unCheckMenus: function RUI_unCheckMenus() {
     this.chromeDoc.getElementById("Tools:ResponsiveUI").setAttribute("checked", "false");
   },

  














  buildUI: function RUI_buildUI() {
    
    this.toolbar = this.chromeDoc.createElement("toolbar");
    this.toolbar.className = "devtools-toolbar devtools-responsiveui-toolbar";

    this.menulist = this.chromeDoc.createElement("menulist");
    this.menulist.className = "devtools-menulist";

    this.menulist.addEventListener("select", this.bound_presetSelected, true);

    let menupopup = this.chromeDoc.createElement("menupopup");
    this.registerPresets(menupopup);
    this.menulist.appendChild(menupopup);

    this.rotatebutton = this.chromeDoc.createElement("toolbarbutton");
    this.rotatebutton.setAttribute("tabindex", "0");
    this.rotatebutton.setAttribute("label", this.strings.GetStringFromName("responsiveUI.rotate"));
    this.rotatebutton.className = "devtools-toolbarbutton";
    this.rotatebutton.addEventListener("command", this.bound_rotate, true);

    this.toolbar.appendChild(this.menulist);
    this.toolbar.appendChild(this.rotatebutton);

    
    this.resizer = this.chromeDoc.createElement("box");
    this.resizer.className = "devtools-responsiveui-resizehandle";
    this.resizer.setAttribute("right", "0");
    this.resizer.setAttribute("bottom", "0");
    this.resizer.onmousedown = this.bound_startResizing;

    this.resizeBar =  this.chromeDoc.createElement("box");
    this.resizeBar.className = "devtools-responsiveui-resizebar";
    this.resizeBar.setAttribute("top", "0");
    this.resizeBar.setAttribute("right", "0");
    this.resizeBar.onmousedown = this.bound_startResizing;

    this.container.insertBefore(this.toolbar, this.stack);
    this.stack.appendChild(this.resizer);
    this.stack.appendChild(this.resizeBar);
  },

  




  registerPresets: function RUI_registerPresets(aParent) {
    let fragment = this.chromeDoc.createDocumentFragment();
    let doc = this.chromeDoc;

    for (let i = 0; i < this.presets.length; i++) {
      let menuitem = doc.createElement("menuitem");
      if (i == this.currentPreset)
        menuitem.setAttribute("selected", "true");
      this.setMenuLabel(menuitem, this.presets[i]);
      fragment.appendChild(menuitem);
    }
    aParent.appendChild(fragment);
  },

  





  setMenuLabel: function RUI_setMenuLabel(aMenuitem, aPreset) {
    let size = Math.round(aPreset.width) + "x" + Math.round(aPreset.height);
    if (aPreset.custom) {
      let str = this.strings.formatStringFromName("responsiveUI.customResolution", [size], 1);
      aMenuitem.setAttribute("label", str);
    } else {
      aMenuitem.setAttribute("label", size);
    }
  },

  


  presetSelected: function RUI_presetSelected() {
    this.rotateValue = false;
    this.currentPreset = this.menulist.selectedIndex;
    let preset = this.presets[this.currentPreset];
    this.loadPreset(preset);
  },

  




  loadPreset: function RUI_loadPreset(aPreset) {
    this.setSize(aPreset.width, aPreset.height);
  },

  


  rotate: function RUI_rotate() {
    this.setSize(this.currentHeight, this.currentWidth);
    if (this.currentPreset == 0) {
      this.saveCustomSize();
    } else {
      this.rotateValue = !this.rotateValue;
    }
  },

  





  setSize: function RUI_setSize(aWidth, aHeight) {
    this.currentWidth = Math.min(Math.max(aWidth, MIN_WIDTH), MAX_WIDTH);
    this.currentHeight = Math.min(Math.max(aHeight, MIN_HEIGHT), MAX_WIDTH);

    
    let style = "max-width: %width;" +
                "min-width: %width;" +
                "max-height: %height;" +
                "min-height: %height;";

    style = style.replace(/%width/g, this.currentWidth + "px");
    style = style.replace(/%height/g, this.currentHeight + "px");

    this.stack.setAttribute("style", style);

    if (!this.ignoreY)
      this.resizeBar.setAttribute("top", Math.round(this.currentHeight / 2));

    
    if (this.presets[this.currentPreset].custom) {
      let preset = this.presets[this.currentPreset];
      preset.width = this.currentWidth;
      preset.height = this.currentHeight;

      let menuitem = this.menulist.firstChild.childNodes[this.currentPreset];
      this.setMenuLabel(menuitem, preset);
    }
  },

  




  startResizing: function RUI_startResizing(aEvent) {
    let preset = this.presets[this.currentPreset];
    if (!preset.custom) {
      this.currentPreset = 0;
      preset = this.presets[0];
      preset.width = this.currentWidth;
      preset.height = this.currentHeight;
      let menuitem = this.menulist.firstChild.childNodes[0];
      this.setMenuLabel(menuitem, preset);
      this.menulist.selectedIndex = 0;
    }
    this.mainWindow.addEventListener("mouseup", this.bound_stopResizing, true);
    this.mainWindow.addEventListener("mousemove", this.bound_onDrag, true);
    this.container.style.pointerEvents = "none";

    this._resizing = true;
    this.stack.setAttribute("notransition", "true");

    this.lastClientX = aEvent.clientX;
    this.lastClientY = aEvent.clientY;

    this.ignoreY = (aEvent.target === this.resizeBar);

    this.isResizing = true;
  },

  




  onDrag: function RUI_onDrag(aEvent) {
    let deltaX = aEvent.clientX - this.lastClientX;
    let deltaY = aEvent.clientY - this.lastClientY;

    if (this.ignoreY)
      deltaY = 0;

    let width = this.currentWidth + deltaX;
    let height = this.currentHeight + deltaY;

    if (width < MIN_WIDTH) {
        width = MIN_WIDTH;
    } else {
        this.lastClientX = aEvent.clientX;
    }

    if (height < MIN_HEIGHT) {
        height = MIN_HEIGHT;
    } else {
        this.lastClientY = aEvent.clientY;
    }

    this.setSize(width, height);
  },

  


  stopResizing: function RUI_stopResizing() {
    this.container.style.pointerEvents = "auto";

    this.mainWindow.removeEventListener("mouseup", this.bound_stopResizing, true);
    this.mainWindow.removeEventListener("mousemove", this.bound_onDrag, true);

    this.saveCustomSize();

    delete this._resizing;
    if (this.transitionsEnabled) {
      this.stack.removeAttribute("notransition");
    }
    this.ignoreY = false;
    this.isResizing = false;
  },

  


   saveCustomSize: function RUI_saveCustomSize() {
     Services.prefs.setIntPref("devtools.responsiveUI.customWidth", this.currentWidth);
     Services.prefs.setIntPref("devtools.responsiveUI.customHeight", this.currentHeight);
   },

  


   saveCurrentPreset: function RUI_saveCurrentPreset() {
     let key = this.presets[this.currentPreset].key;
     Services.prefs.setCharPref("devtools.responsiveUI.currentPreset", key);
     Services.prefs.setBoolPref("devtools.responsiveUI.rotate", this.rotateValue);
   },
}

XPCOMUtils.defineLazyGetter(ResponsiveUI.prototype, "strings", function () {
  return Services.strings.createBundle("chrome://browser/locale/devtools/responsiveUI.properties");
});
