



"use strict";

const {Cu, Cc, Ci} = require("chrome");

let Promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

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




function OptionsPanel(iframeWindow, toolbox) {
  this.panelDoc = iframeWindow.document;
  this.panelWin = iframeWindow;
  this.toolbox = toolbox;
  this.isReady = false;

  
  this.panelWin.restart = this.restart;

  EventEmitter.decorate(this);
};

OptionsPanel.prototype = {

  get target() {
    return this.toolbox.target;
  },

  open: function() {
    let deferred = Promise.defer();

    this.setupToolsList();
    this.populatePreferences();
    this.prepareRestartPreferences();

    this._disableJSClicked = this._disableJSClicked.bind(this);

    let disableJSNode = this.panelDoc.getElementById("devtools-disable-javascript");
    disableJSNode.addEventListener("click", this._disableJSClicked, false);

    this.isReady = true;
    this.emit("ready");
    deferred.resolve(this);
    return deferred.promise;
  },

  setupToolsList: function() {
    let defaultToolsBox = this.panelDoc.getElementById("default-tools-box");
    let additionalToolsBox = this.panelDoc.getElementById("additional-tools-box");
    let toolsNotSupportedLabel = this.panelDoc.getElementById("tools-not-supported-label");
    let atleastOneToolNotSupported = false;

    defaultToolsBox.textContent = "";
    additionalToolsBox.textContent = "";

    let pref = function(key) {
      try {
        return Services.prefs.getBoolPref(key);
      }
      catch (ex) {
        return true;
      }
    };

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
      }
      checkbox.setAttribute("checked", pref(tool.visibilityswitch));
      checkbox.addEventListener("command", onCheckboxClick.bind(checkbox, tool.id));
      return checkbox;
    };

    
    for (let tool of gDevTools.getDefaultTools()) {
      if (tool.id == "options") {
        continue;
      }
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

  populatePreferences: function() {
    let prefCheckboxes = this.panelDoc.querySelectorAll("checkbox[data-pref]");
    for (let checkbox of prefCheckboxes) {
      checkbox.checked = Services.prefs.getBoolPref(checkbox.getAttribute("data-pref"));
      checkbox.addEventListener("command", function() {
        let data = {
          pref: this.getAttribute("data-pref"),
          newValue: this.checked
        };
        data.oldValue = Services.prefs.getBoolPref(data.pref);
        Services.prefs.setBoolPref(data.pref, data.newValue);
        gDevTools.emit("pref-changed", data);
      }.bind(checkbox));
    }
    let prefRadiogroups = this.panelDoc.querySelectorAll("radiogroup[data-pref]");
    for (let radiogroup of prefRadiogroups) {
      let selectedValue = Services.prefs.getCharPref(radiogroup.getAttribute("data-pref"));
      for (let radio of radiogroup.childNodes) {
        radiogroup.selectedIndex = -1;
        if (radio.getAttribute("value") == selectedValue) {
          radiogroup.selectedItem = radio;
          break;
        }
      }
      radiogroup.addEventListener("select", function() {
        let data = {
          pref: this.getAttribute("data-pref"),
          newValue: this.selectedItem.getAttribute("value")
        };
        data.oldValue = Services.prefs.getCharPref(data.pref);
        Services.prefs.setCharPref(data.pref, data.newValue);
        gDevTools.emit("pref-changed", data);
      }.bind(radiogroup));
    }
  },

  




  prepareRestartPreferences: function() {
    let checkboxes = this.panelDoc.querySelectorAll(".hidden-labels-box > checkbox");
    for (let checkbox of checkboxes) {
      checkbox.addEventListener("command", function(target) {
        target.parentNode.classList.toggle("visible");
      }.bind(null, checkbox));
    }
  },

  restart: function() {
    let canceled = Cc["@mozilla.org/supports-PRBool;1"]
                     .createInstance(Ci.nsISupportsPRBool);
    Services.obs.notifyObservers(canceled, "quit-application-requested", "restart");
    if (canceled.data) {
      return;
    }

    
    Cc['@mozilla.org/toolkit/app-startup;1']
      .getService(Ci.nsIAppStartup)
      .quit(Ci.nsIAppStartup.eAttemptQuit | Ci.nsIAppStartup.eRestart);
  },

  









  _disableJSClicked: function(event) {
    let checked = event.target.checked;
    let linkedBrowser = this.toolbox._host.hostTab.linkedBrowser;
    let win = linkedBrowser.contentWindow;
    let docShell = linkedBrowser.docShell;

    if (typeof this.toolbox._origAllowJavascript == "undefined") {
      this.toolbox._origAllowJavascript = docShell.allowJavascript;
    }

    docShell.allowJavascript = !checked;
    win.location.reload();
  },

  destroy: function OP_destroy() {
    let disableJSNode = this.panelDoc.getElementById("devtools-disable-javascript");
    disableJSNode.removeEventListener("click", this._disableJSClicked, false);

    this.panelWin = this.panelDoc = this.toolbox = this._disableJSClicked = null;
  }
};
