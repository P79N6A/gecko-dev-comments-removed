




"use strict";

var SanitizeUI = {
  _sanitizer: null,

  _privDataElement: null,
  get _privData() {
    if (this._privDataElement === null) {
      this._privDataElement = document.getElementById("prefs-privdata");
    }
    return this._privDataElement;
  },

  init: function () {
    this._sanitizer = new Sanitizer();
    this._privData.addEventListener("CheckboxStateChange", this, true);
  },

  _clearNotificationTimeout: null,
  onSanitize: function onSanitize() {
    let button = document.getElementById("prefs-clear-data");
    let clearNotificationDeck = document.getElementById("clear-notification");
    let clearNotificationEmpty = document.getElementById("clear-notification-empty");
    let clearNotificationClearing = document.getElementById("clear-notification-clearing");
    let clearNotificationDone = document.getElementById("clear-notification-done");
    let allCheckboxes = SanitizeUI._privData.querySelectorAll("checkbox");
    let allSelected = SanitizeUI._privData.querySelectorAll(
      "#prefs-privdata-history[checked], " +
      "#prefs-privdata-other[checked] + #prefs-privdata-subitems .privdata-subitem-item[checked]");

    
    button.disabled = true;
    for (let checkbox of allCheckboxes) {
      checkbox.disabled = true;
    }
    clearNotificationDeck.selectedPanel = clearNotificationClearing;

    
    setTimeout(function() {
      for (let item of allSelected) {
        let itemName = item.getAttribute("itemName");

        try {
          SanitizeUI._sanitizer.clearItem(itemName);
        } catch(e) {
          Components.utils.reportError("Error sanitizing " + itemName + ": " + e);
        }
      }

      button.disabled = false;
      for (let checkbox of allCheckboxes) {
        checkbox.disabled = false;
      }
      clearNotificationDeck.selectedPanel = clearNotificationDone;

      
      clearTimeout(SanitizeUI._clearNotificationTimeout);
      SanitizeUI._clearNotificationTimeout = setTimeout(function() {
        clearNotificationDeck.selectedPanel = clearNotificationEmpty;
      }, 4000);
    }, 0);
  },

  
  _onCheckboxChange: function _onCheckboxChange() {
    let anySelected = SanitizeUI._privData.querySelector(
      "#prefs-privdata-history[checked], " +
      "#prefs-privdata-other[checked] + #prefs-privdata-subitems .privdata-subitem-item[checked]");

    let clearButton = document.getElementById("prefs-clear-data");
    clearButton.disabled = !anySelected;
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "CheckboxStateChange":
        this._onCheckboxChange();
        break;
    }
  },
};