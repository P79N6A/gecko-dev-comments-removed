






































const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["ResponsiveUIManager"];

const MIN_WIDTH = 50;
const MIN_HEIGHT = 50;

let ResponsiveUIManager = {
  







  toggle: function(aWindow, aTab) {
    if (aTab.responsiveUI) {
      aTab.responsiveUI.close();
    } else {
      aTab.responsiveUI = new ResponsiveUI(aWindow, aTab);
    }
  },
}

let presets =  [
  
  {width: 320, height: 480},    
  {width: 360, height: 640},    

  
  {width: 768, height: 1024},   
  {width: 800, height: 1280},   

  
  {width: 980, height: 1280},

  
  {width: 1280, height: 600},
  {width: 1920, height: 900},
];

function ResponsiveUI(aWindow, aTab)
{
  this.mainWindow = aWindow;
  this.tab = aTab;
  this.browser = aTab.linkedBrowser;
  this.chromeDoc = aWindow.document;
  this.container = aWindow.gBrowser.getBrowserContainer(this.browser);
  this.stack = this.container.querySelector("[anonid=browserStack]");

  
  if (Services.prefs.prefHasUserValue("devtools.responsiveUI.presets")) {
    try {
      presets = JSON.parse(Services.prefs.getCharPref("devtools.responsiveUI.presets"));
    } catch(e) {
      
      Cu.reportError("Could not parse pref `devtools.responsiveUI.presets`: " + e);
    }
  }

  if (Array.isArray(presets)) {
    this.presets = [{custom: true}].concat(presets)
  } else {
    Cu.reportError("Presets value (devtools.responsiveUI.presets) is malformated.");
    this.presets = [{custom: true}];
  }

  
  let bbox = this.stack.getBoundingClientRect();
  this.presets[0].width = bbox.width - 40; 
  this.presets[0].height = bbox.height - 80; 
  this.currentPreset = 0; 

  this.container.setAttribute("responsivemode", "true");
  this.stack.setAttribute("responsivemode", "true");

  
  this.bound_presetSelected = this.presetSelected.bind(this);
  this.bound_rotate = this.rotate.bind(this);
  this.bound_startResizing = this.startResizing.bind(this);
  this.bound_stopResizing = this.stopResizing.bind(this);
  this.bound_onDrag = this.onDrag.bind(this);
  this.bound_onKeypress = this.onKeypress.bind(this);

  
  this.tab.addEventListener("TabClose", this);
  this.tab.addEventListener("TabAttrModified", this);
  this.mainWindow.addEventListener("keypress", this.bound_onKeypress, true);

  this.buildUI();
  this.checkMenus();
}

ResponsiveUI.prototype = {
  


  close: function RUI_unload() {
    this.unCheckMenus();
    
    let style = "max-width: none;" +
                "min-width: 0;" +
                "max-height: none;" +
                "min-height: 0;";
    this.stack.setAttribute("style", style);

    this.stopResizing();

    
    this.mainWindow.removeEventListener("keypress", this.bound_onKeypress, true);
    this.menulist.removeEventListener("select", this.bound_presetSelected, true);
    this.tab.removeEventListener("TabClose", this);
    this.tab.removeEventListener("TabAttrModified", this);
    this.rotatebutton.removeEventListener("command", this.bound_rotate, true);

    
    this.container.removeChild(this.toolbar);
    this.stack.removeChild(this.resizer);
    this.stack.removeChild(this.resizeBar);

    
    this.container.removeAttribute("responsivemode");
    this.stack.removeAttribute("responsivemode");

    delete this.tab.responsiveUI;
  },

  




  onKeypress: function RUI_onKeypress(aEvent) {
    if (aEvent.keyCode == this.mainWindow.KeyEvent.DOM_VK_ESCAPE &&
        this.mainWindow.gBrowser.selectedBrowser == this.browser) {
      aEvent.preventDefault();
      aEvent.stopPropagation();
      this.close();
    }
  },

  


  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "TabClose":
        this.close();
        break;
      case "TabAttrModified":
        if (this.mainWindow.gBrowser.selectedBrowser == this.browser) {
          this.checkMenus();
        } else {
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
    let self = this;
    this.presets.forEach(function(preset) {
        let menuitem = doc.createElement("menuitem");
        self.setMenuLabel(menuitem, preset);
        fragment.appendChild(menuitem);
    });
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
    this.currentPreset = this.menulist.selectedIndex;
    let preset = this.presets[this.currentPreset];
    this.loadPreset(preset);
  },

  




  loadPreset: function RUI_loadPreset(aPreset) {
    this.setSize(aPreset.width, aPreset.height);
  },

  


  rotate: function RUI_rotate() {
    this.setSize(this.currentHeight, this.currentWidth);
  },

  





  setSize: function RUI_setSize(aWidth, aHeight) {
    this.currentWidth = aWidth;
    this.currentHeight = aHeight;

    
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

    this.stack.setAttribute("notransition", "true");

    this.lastClientX = aEvent.clientX;
    this.lastClientY = aEvent.clientY;

    this.ignoreY = (aEvent.target === this.resizeBar);
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

    this.stack.removeAttribute("notransition");
    this.ignoreY = false;
  },
}

XPCOMUtils.defineLazyGetter(ResponsiveUI.prototype, "strings", function () {
  return Services.strings.createBundle("chrome://browser/locale/devtools/responsiveUI.properties");
});
