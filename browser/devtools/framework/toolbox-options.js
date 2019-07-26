



"use strict";

const {Cu} = require("chrome");

let Promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

exports.OptionsPanel = OptionsPanel;




function OptionsPanel(iframeWindow, toolbox) {
  this.panelDoc = iframeWindow.document;
  this.panelWin = iframeWindow;

  EventEmitter.decorate(this);
};

OptionsPanel.prototype = {

  open: function OP_open() {
    let deferred = Promise.defer();

    this.setupToolsList();
    this.populatePreferences();

    this.emit("ready");
    deferred.resolve(this);
    return deferred.promise;
  },

  setupToolsList: function OP_setupToolsList() {
    let defaultToolsBox = this.panelDoc.getElementById("default-tools-box");
    let additionalToolsBox = this.panelDoc.getElementById("additional-tools-box");

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

    
    for (let tool of gDevTools.getDefaultTools()) {
      if (tool.id == "options") {
        continue;
      }
      let checkbox = this.panelDoc.createElement("checkbox");
      checkbox.setAttribute("id", tool.id);
      checkbox.setAttribute("label", tool.label);
      checkbox.setAttribute("tooltiptext", tool.tooltip || "");
      checkbox.setAttribute("checked", pref(tool.visibilityswitch));
      checkbox.addEventListener("command", onCheckboxClick.bind(checkbox, tool.id));
      defaultToolsBox.appendChild(checkbox);
    }

    
    let atleastOneAddon = false;
    for (let tool of gDevTools.getAdditionalTools()) {
      atleastOneAddon = true;
      let checkbox = this.panelDoc.createElement("checkbox");
      checkbox.setAttribute("id", tool.id);
      checkbox.setAttribute("label", tool.label);
      checkbox.setAttribute("tooltiptext", tool.tooltip || "");
      checkbox.setAttribute("checked", pref(tool.visibilityswitch));
      checkbox.addEventListener("command", onCheckboxClick.bind(checkbox, tool.id));
      additionalToolsBox.appendChild(checkbox);
    }

    if (!atleastOneAddon) {
      additionalToolsBox.style.display = "none";
      additionalToolsBox.previousSibling.style.display = "none";
    }

    this.panelWin.focus();
  },

  populatePreferences: function OP_populatePreferences() {
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

  destroy: function OP_destroy() {
    this.panelWin = this.panelDoc = null;
  }
};
