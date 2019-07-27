



"use strict";

const {Cu, Cc, Ci} = require("chrome");
const Services = require("Services");
const promise = require("promise");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizeMode", "resource:///modules/CustomizeMode.jsm");
const kDeveditionChangedNotification = "devedition-theme-state-changed";
const DEVEDITION_THEME_PREF = "browser.devedition.theme.enabled";

exports.OptionsPanel = OptionsPanel;

XPCOMUtils.defineLazyGetter(this, "l10n", function() {
  let bundle = Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");
  let l10n = function(aName, ...aArgs) {
    try {
      if (aArgs.length == 0) {
        return bundle.GetStringFromName(aName);
      } else {
        return bundle.formatStringFromName(aName, aArgs, aArgs.length);
      }
    } catch (ex) {
      Services.console.logStringMessage("Error reading '" + aName + "'");
    }
  };
  return l10n;
});

function GetPref(name) {
  let type = Services.prefs.getPrefType(name);
  switch (type) {
    case Services.prefs.PREF_STRING:
      return Services.prefs.getCharPref(name);
    case Services.prefs.PREF_INT:
      return Services.prefs.getIntPref(name);
    case Services.prefs.PREF_BOOL:
      return Services.prefs.getBoolPref(name);
    default:
      throw new Error("Unknown type");
  }
}

function SetPref(name, value) {
  let type = Services.prefs.getPrefType(name);
  switch (type) {
    case Services.prefs.PREF_STRING:
      return Services.prefs.setCharPref(name, value);
    case Services.prefs.PREF_INT:
      return Services.prefs.setIntPref(name, value);
    case Services.prefs.PREF_BOOL:
      return Services.prefs.setBoolPref(name, value);
    default:
      throw new Error("Unknown type");
  }
}

function InfallibleGetBoolPref(key) {
  try {
    return Services.prefs.getBoolPref(key);
  } catch (ex) {
    return true;
  }
}





function OptionsPanel(iframeWindow, toolbox) {
  this.panelDoc = iframeWindow.document;
  this.panelWin = iframeWindow;

  this.toolbox = toolbox;
  this.isReady = false;

  this._prefChanged = this._prefChanged.bind(this);
  this._themeRegistered = this._themeRegistered.bind(this);
  this._themeUnregistered = this._themeUnregistered.bind(this);
  this._disableJSClicked = this._disableJSClicked.bind(this);

  this.disableJSNode = this.panelDoc.getElementById("devtools-disable-javascript");

  this._addListeners();

  Services.obs.addObserver(this, kDeveditionChangedNotification, false);
  const EventEmitter = require("devtools/toolkit/event-emitter");
  EventEmitter.decorate(this);
}

OptionsPanel.prototype = {

  get target() {
    return this.toolbox.target;
  },

  open: function() {
    let targetPromise;

    
    if (!this.target.isRemote) {
      targetPromise = this.target.makeRemote();
    } else {
      targetPromise = promise.resolve(this.target);
    }

    return targetPromise.then(() => {
      this.setupToolsList();
      this.setupToolbarButtonsList();
      this.setupThemeList();
      this.setupBrowserThemeButton();
      this.populatePreferences();
      this.updateDefaultTheme();
    }).then(() => {
      this.isReady = true;
      this.emit("ready");
      return this;
    }).then(null, function onError(aReason) {
      Cu.reportError("OptionsPanel open failed. " +
                     aReason.error + ": " + aReason.message);
    });
  },

  _addListeners: function() {
    gDevTools.on("pref-changed", this._prefChanged);
    gDevTools.on("theme-registered", this._themeRegistered);
    gDevTools.on("theme-unregistered", this._themeUnregistered);
  },

  _removeListeners: function() {
    gDevTools.off("pref-changed", this._prefChanged);
    gDevTools.off("theme-registered", this._themeRegistered);
    gDevTools.off("theme-unregistered", this._themeUnregistered);
  },

  _prefChanged: function(event, data) {
    if (data.pref === "devtools.cache.disabled") {
      let cacheDisabled = data.newValue;
      let cbx = this.panelDoc.getElementById("devtools-disable-cache");

      cbx.checked = cacheDisabled;
    }
    else if (data.pref === "devtools.theme") {
      this.updateCurrentTheme();
    } else if (data.pref === "browser.devedition.theme.enabled") {
      this.updateBrowserTheme();
    }
  },

  _themeRegistered: function(event, themeId) {
    this.setupThemeList();
  },

  _themeUnregistered: function(event, theme) {
    let themeBox = this.panelDoc.getElementById("devtools-theme-box");
    let themeOption = themeBox.querySelector("[value=" + theme.id + "]");

    if (themeOption) {
      themeBox.removeChild(themeOption);
    }
  },

  setupToolbarButtonsList: function() {
    let enabledToolbarButtonsBox = this.panelDoc.getElementById("enabled-toolbox-buttons-box");
    enabledToolbarButtonsBox.textContent = "";

    let toggleableButtons = this.toolbox.toolboxButtons;
    let setToolboxButtonsVisibility =
      this.toolbox.setToolboxButtonsVisibility.bind(this.toolbox);

    let onCheckboxClick = (checkbox) => {
      let toolDefinition = toggleableButtons.filter(tool => tool.id === checkbox.id)[0];
      Services.prefs.setBoolPref(toolDefinition.visibilityswitch, checkbox.checked);
      setToolboxButtonsVisibility();
    };

    let createCommandCheckbox = tool => {
      let checkbox = this.panelDoc.createElement("checkbox");
      checkbox.setAttribute("id", tool.id);
      checkbox.setAttribute("label", tool.label);
      checkbox.setAttribute("checked", InfallibleGetBoolPref(tool.visibilityswitch));
      checkbox.addEventListener("command", onCheckboxClick.bind(this, checkbox));
      return checkbox;
    };

    for (let tool of toggleableButtons) {
      if (this.toolbox.target.isMultiProcess && tool.id === "command-button-tilt") {
        continue;
      }

      enabledToolbarButtonsBox.appendChild(createCommandCheckbox(tool));
    }
  },

  setupToolsList: function() {
    let defaultToolsBox = this.panelDoc.getElementById("default-tools-box");
    let additionalToolsBox = this.panelDoc.getElementById("additional-tools-box");
    let toolsNotSupportedLabel = this.panelDoc.getElementById("tools-not-supported-label");
    let atleastOneToolNotSupported = false;

    defaultToolsBox.textContent = "";
    additionalToolsBox.textContent = "";

    let onCheckboxClick = function(id) {
      let toolDefinition = gDevTools._tools.get(id);
      
      Services.prefs.setBoolPref(toolDefinition.visibilityswitch, this.checked);
      if (this.checked) {
        gDevTools.emit("tool-registered", id);
      }
      else {
        gDevTools.emit("tool-unregistered", toolDefinition);
      }
    };

    let createToolCheckbox = tool => {
      let checkbox = this.panelDoc.createElement("checkbox");
      checkbox.setAttribute("id", tool.id);
      checkbox.setAttribute("tooltiptext", tool.tooltip || "");
      if (tool.isTargetSupported(this.target)) {
        checkbox.setAttribute("label", tool.label);
      }
      else {
        atleastOneToolNotSupported = true;
        checkbox.setAttribute("label",
                              l10n("options.toolNotSupportedMarker", tool.label));
        checkbox.setAttribute("unsupported", "");
      }
      checkbox.setAttribute("checked", InfallibleGetBoolPref(tool.visibilityswitch));
      checkbox.addEventListener("command", onCheckboxClick.bind(checkbox, tool.id));
      return checkbox;
    };

    
    let toggleableTools = gDevTools.getDefaultTools().filter(tool => {
      return tool.visibilityswitch;
    });

    for (let tool of toggleableTools) {
      defaultToolsBox.appendChild(createToolCheckbox(tool));
    }

    
    let atleastOneAddon = false;
    for (let tool of gDevTools.getAdditionalTools()) {
      atleastOneAddon = true;
      additionalToolsBox.appendChild(createToolCheckbox(tool));
    }

    if (!atleastOneAddon) {
      additionalToolsBox.style.display = "none";
      additionalToolsBox.previousSibling.style.display = "none";
    }

    if (!atleastOneToolNotSupported) {
      toolsNotSupportedLabel.style.display = "none";
    }

    this.panelWin.focus();
  },

  setupThemeList: function() {
    let themeBox = this.panelDoc.getElementById("devtools-theme-box");
    themeBox.textContent = "";

    let createThemeOption = theme => {
      let radio = this.panelDoc.createElement("radio");
      radio.setAttribute("value", theme.id);
      radio.setAttribute("label", theme.label);
      return radio;
    };

    
    let themes = gDevTools.getThemeDefinitionArray();
    for (let theme of themes) {
      themeBox.appendChild(createThemeOption(theme));
    }

    this.updateCurrentTheme();
  },

  



  setupBrowserThemeButton: function() {
    let checkbox = this.panelDoc.getElementById("devtools-browser-theme");

    checkbox.addEventListener("command", function() {
      setPrefAndEmit(DEVEDITION_THEME_PREF, this.checked);
    }.bind(checkbox));

    this.updateBrowserThemeButton();
  },

  


  updateBrowserThemeButton: function() {
    let checkbox = this.panelDoc.getElementById("devtools-browser-theme");

    
    
    
    if (this._isDevEditionThemeOn()) {
      checkbox.setAttribute("checked", "true");
    } else {
      checkbox.removeAttribute("checked");
    }

    
    if (GetPref("browser.devedition.theme.showCustomizeButton")) {
      checkbox.removeAttribute("hidden");
    } else {
      checkbox.setAttribute("hidden", "true");
    }
  },

  



  updateBrowserTheme: function() {
    let enabled = GetPref("browser.devedition.theme.enabled");
    CustomizeMode.prototype.toggleDevEditionTheme.call(this, enabled);
  },

  populatePreferences: function() {
    let prefCheckboxes = this.panelDoc.querySelectorAll("checkbox[data-pref]");
    for (let checkbox of prefCheckboxes) {
      checkbox.checked = GetPref(checkbox.getAttribute("data-pref"));
      checkbox.addEventListener("command", function() {
        setPrefAndEmit(this.getAttribute("data-pref"), this.checked);
      }.bind(checkbox));
    }
    let prefRadiogroups = this.panelDoc.querySelectorAll("radiogroup[data-pref]");
    for (let radiogroup of prefRadiogroups) {
      let selectedValue = GetPref(radiogroup.getAttribute("data-pref"));
      for (let radio of radiogroup.childNodes) {
        radiogroup.selectedIndex = -1;
        if (radio.getAttribute("value") == selectedValue) {
          radiogroup.selectedItem = radio;
          break;
        }
      }
      radiogroup.addEventListener("select", function() {
        setPrefAndEmit(this.getAttribute("data-pref"), this.selectedItem.getAttribute("value"));
      }.bind(radiogroup));
    }
    let prefMenulists = this.panelDoc.querySelectorAll("menulist[data-pref]");
    for (let menulist of prefMenulists) {
      let pref = GetPref(menulist.getAttribute("data-pref"));
      let menuitems = menulist.querySelectorAll("menuitem");
      for (let menuitem of menuitems) {
        let value = menuitem.value;
        if (value == pref) { 
          menulist.selectedItem = menuitem;
          break;
        }
      }
      menulist.addEventListener("command", function() {
        setPrefAndEmit(this.getAttribute("data-pref"), this.value);
      }.bind(menulist));
    }

    if (this.target.activeTab) {
      this.target.client.attachTab(this.target.activeTab._actor, (response) => {
        this._origJavascriptEnabled = response.javascriptEnabled;
        this.disableJSNode.checked = !this._origJavascriptEnabled;
        this.disableJSNode.addEventListener("click", this._disableJSClicked, false);
      });
    } else {
      this.disableJSNode.hidden = true;
    }
  },

  updateDefaultTheme: function() {
    
    
    let themeBox = this.panelDoc.getElementById("devtools-theme-box");
    if (themeBox.selectedIndex == -1) {
      themeBox.selectedItem = themeBox.querySelector("[value=light]");
    }
  },

  updateCurrentTheme: function() {
    let currentTheme = GetPref("devtools.theme");
    let themeBox = this.panelDoc.getElementById("devtools-theme-box");
    let themeOption = themeBox.querySelector("[value=" + currentTheme + "]");

    if (themeOption) {
      themeBox.selectedItem = themeOption;
    }
  },

  









  _disableJSClicked: function(event) {
    let checked = event.target.checked;

    let options = {
      "javascriptEnabled": !checked
    };

    this.target.activeTab.reconfigure(options);
  },

  



  _isDevEditionThemeOn: function() {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    return !!(win && win.DevEdition.styleSheet);
  },

  



  observe: function(aSubject, aTopic, aData) {
    if (aTopic === kDeveditionChangedNotification) {
      this.updateBrowserThemeButton();
    }
  },

  destroy: function() {
    if (this.destroyPromise) {
      return this.destroyPromise;
    }

    let deferred = promise.defer();

    this.destroyPromise = deferred.promise;
    this._removeListeners();

    if (this.target.activeTab) {
      this.disableJSNode.removeEventListener("click", this._disableJSClicked, false);
      
      let options = {
        "javascriptEnabled": this._origJavascriptEnabled
      };
      this.target.activeTab.reconfigure(options, () => {
        this.toolbox = null;
        deferred.resolve();
      }, true);
    }

    this.panelWin = this.panelDoc = this.disableJSNode = null;

    Services.obs.removeObserver(this, kDeveditionChangedNotification);

    return deferred.promise;
  }
};


function setPrefAndEmit(prefName, newValue) {
  let data = {
    pref: prefName,
    newValue: newValue
  };
  data.oldValue = GetPref(data.pref);
  SetPref(data.pref, data.newValue);

  if (data.newValue != data.oldValue) {
    gDevTools.emit("pref-changed", data);
  }
}
