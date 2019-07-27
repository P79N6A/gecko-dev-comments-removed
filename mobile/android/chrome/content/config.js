


"use strict";

const {classes: Cc, interfaces: Ci, manager: Cm, utils: Cu} = Components;
Cu.import("resource://gre/modules/Services.jsm");

const VKB_ENTER_KEY = 13;   
const INITIAL_PAGE_DELAY = 500;   
const PREFS_BUFFER_MAX = 30;   
const PAGE_SCROLL_TRIGGER = 200;     
const FILTER_CHANGE_TRIGGER = 200;     
const INNERHTML_VALUE_DELAY = 100;    

let gStringBundle = Services.strings.createBundle("chrome://browser/locale/config.properties");
let gClipboardHelper = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);









var NewPrefDialog = {

  _prefsShield: null,

  _newPrefsDialog: null,
  _newPrefItem: null,
  _prefNameInputElt: null,
  _prefTypeSelectElt: null,

  _booleanValue: null,
  _booleanToggle: null,
  _stringValue: null,
  _intValue: null,

  _positiveButton: null,

  get type() {
    return this._prefTypeSelectElt.value;
  },

  set type(aType) {
    this._prefTypeSelectElt.value = aType;
    switch(this._prefTypeSelectElt.value) {
      case "boolean":
        this._prefTypeSelectElt.selectedIndex = 0;
        break;
      case "string":
        this._prefTypeSelectElt.selectedIndex = 1;
        break;
      case "int":
        this._prefTypeSelectElt.selectedIndex = 2;
        break;
    }

    this._newPrefItem.setAttribute("typestyle", aType);
  },

  
  init: function AC_init() {
    this._prefsShield = document.getElementById("prefs-shield");

    this._newPrefsDialog = document.getElementById("new-pref-container");
    this._newPrefItem = document.getElementById("new-pref-item");
    this._prefNameInputElt = document.getElementById("new-pref-name");
    this._prefTypeSelectElt = document.getElementById("new-pref-type");

    this._booleanValue = document.getElementById("new-pref-value-boolean");
    this._stringValue = document.getElementById("new-pref-value-string");
    this._intValue = document.getElementById("new-pref-value-int");

    this._positiveButton = document.getElementById("positive-button");
  },

  
  
  _updatePositiveButton: function AC_updatePositiveButton(aPrefName) {
    this._positiveButton.textContent = gStringBundle.GetStringFromName("newPref.createButton");
    this._positiveButton.setAttribute("disabled", true);
    if (aPrefName == "") {
      return;
    }

    
    let item = AboutConfig._list.filter(i => { return i.name == aPrefName });
    if (item.length) {
      this._positiveButton.textContent = gStringBundle.GetStringFromName("newPref.changeButton");
    } else {
      this._positiveButton.removeAttribute("disabled");
    }
  },

  
  toggleShowHide: function AC_toggleShowHide() {
    if (this._newPrefsDialog.classList.contains("show")) {
      this.hide();
    } else {
      this._show();
    }
  },

  
  _show: function AC_show() {
    this._newPrefsDialog.classList.add("show");
    this._prefsShield.setAttribute("shown", true);

    
    this._prefNameInputElt.value = "";
    this._updatePositiveButton(this._prefNameInputElt.value);

    this.type = "boolean";
    this._booleanValue.value = "false";
    this._stringValue.value = "";
    this._intValue.value = "";

    this._prefNameInputElt.focus();

    window.addEventListener("keypress", this.handleKeypress, false);
  },

  
  hide: function AC_hide() {
    this._newPrefsDialog.classList.remove("show");
    this._prefsShield.removeAttribute("shown");

    window.removeEventListener("keypress", this.handleKeypress, false);
  },

  
  handleKeypress: function AC_handleKeypress(aEvent) {
    
    if (aEvent.keyCode == VKB_ENTER_KEY)
      aEvent.target.blur();
  },

  
  
  create: function AC_create(aEvent) {
    if (this._positiveButton.getAttribute("disabled") == "true") {
      return;
    }

    switch(this.type) {
      case "boolean":
        Services.prefs.setBoolPref(this._prefNameInputElt.value, (this._booleanValue.value == "true") ? true : false);
        break;
      case "string":
        Services.prefs.setCharPref(this._prefNameInputElt.value, this._stringValue.value);
        break;
      case "int":
        Services.prefs.setIntPref(this._prefNameInputElt.value, this._intValue.value);
        break;
    }

    
    Services.prefs.savePrefFile(null);

    this.hide();
  },

  
  focusName: function AC_focusName(aEvent) {
    this._updatePositiveButton(aEvent.target.value);
  },

  
  updateName: function AC_updateName(aEvent) {
    this._updatePositiveButton(aEvent.target.value);
  },

  
  
  toggleBoolValue: function AC_toggleBoolValue() {
    this._booleanValue.value = (this._booleanValue.value == "true" ? "false" : "true");
  }
}









var AboutConfig = {

  contextMenuLINode: null,
  filterInput: null,
  _filterPrevInput: null,
  _filterChangeTimer: null,
  _prefsContainer: null,
  _loadingContainer: null,
  _list: null,

  
  init: function AC_init() {
    this.filterInput = document.getElementById("filter-input");
    this._prefsContainer = document.getElementById("prefs-container");
    this._loadingContainer = document.getElementById("loading-container");

    let list = Services.prefs.getChildList("");
    this._list = list.sort().map( function AC_getMapPref(aPref) {
      return new Pref(aPref);
    }, this);

    
    this.bufferFilterInput();

    
    Services.prefs.addObserver("", this, false);
  },

  
  uninit: function AC_uninit() {
    
    Services.prefs.removeObserver("", this);
  },

  
  clearFilterInput: function AC_clearFilterInput() {
    this.filterInput.value = "";
    this.bufferFilterInput();
  },

  
  bufferFilterInput: function AC_bufferFilterInput() {
    if (this._filterChangeTimer) {
      clearTimeout(this._filterChangeTimer);
    }

    this._filterChangeTimer = setTimeout((function() {
      this._filterChangeTimer = null;
      
      this._displayNewList();
    }).bind(this), FILTER_CHANGE_TRIGGER);
  },

  
  _displayNewList: function AC_displayNewList() {
    
    this.filterInput.setAttribute("value", this.filterInput.value);

    
    if (this.filterInput.value == this._filterPrevInput) {
      return;
    }
    this._filterPrevInput = this.filterInput.value;

    
    this.selected = "";
    this._clearPrefsContainer();
    this._addMorePrefsToContainer();
    window.onscroll = this.onScroll.bind(this);

    
    setTimeout((function() {
      window.scrollTo(0, 0);
    }).bind(this), INITIAL_PAGE_DELAY);
  },

  
  _clearPrefsContainer: function AC_clearPrefsContainer() {
    
    let empty = this._prefsContainer.cloneNode(false);
    this._prefsContainer.parentNode.replaceChild(empty, this._prefsContainer); 
    this._prefsContainer = empty;

    
    this._list.forEach(function(item) {
      delete item.li;
    });
  },

  
  _addMorePrefsToContainer: function AC_addMorePrefsToContainer() {
    
    let filterExp = this.filterInput.value ?
      new RegExp(this.filterInput.value, "i") : null;

    
    let prefsBuffer = [];
    for (let i = 0; i < this._list.length && prefsBuffer.length < PREFS_BUFFER_MAX; i++) {
      if (!this._list[i].li && this._list[i].test(filterExp)) {
        prefsBuffer.push(this._list[i]);
      }
    }

    
    for (let i = 0; i < prefsBuffer.length; i++) {
      this._prefsContainer.appendChild(prefsBuffer[i].getOrCreateNewLINode());
    }

    
    let anotherPrefsBufferRemains = false;
    for (let i = 0; i < this._list.length; i++) {
      if (!this._list[i].li && this._list[i].test(filterExp)) {
        anotherPrefsBufferRemains = true;
        break;
      }
    }

    if (anotherPrefsBufferRemains) {
      
      this._loadingContainer.style.display = "block";
    } else {
      
      this._loadingContainer.style.display = "none";
      window.onscroll = null;
    }
  },

  
  onScroll: function AC_onScroll(aEvent) {
    if (this._prefsContainer.scrollHeight - (window.pageYOffset + window.innerHeight) < PAGE_SCROLL_TRIGGER) {
      if (!this._filterChangeTimer) {
        this._addMorePrefsToContainer();
      }
    }
  },


  
  get selected() {
    return document.querySelector(".pref-item.selected");
  },

  
  set selected(aSelection) {
    let currentSelection = this.selected;
    if (aSelection == currentSelection) {
      return;
    }

    
    if (currentSelection) {
      currentSelection.classList.remove("selected");
      currentSelection.removeEventListener("keypress", this.handleKeypress, false);
    }

    
    if (aSelection) {
      aSelection.classList.add("selected");
      aSelection.addEventListener("keypress", this.handleKeypress, false);
    }
  },

  
  handleKeypress: function AC_handleKeypress(aEvent) {
    if (aEvent.keyCode == VKB_ENTER_KEY)
      aEvent.target.blur();
  },

  
  getLINodeForEvent: function AC_getLINodeForEvent(aEvent) {
    let node = aEvent.target;
    while (node && node.nodeName != "li") {
      node = node.parentNode;
    }

    return node;
  },

  
  _getPrefForNode: function AC_getPrefForNode(aNode) {
    let pref = aNode.getAttribute("name");

    return new Pref(pref);
  },

  
  selectOrToggleBoolPref: function AC_selectOrToggleBoolPref(aEvent) {
    let node = this.getLINodeForEvent(aEvent);

    
    if (this.selected != node) {
      this.selected = node;
      return;
    }

    
    let pref = this._getPrefForNode(node);
    if (pref.type != Services.prefs.PREF_BOOL) {
      return;
    }

    this.toggleBoolPref(aEvent);
  },

  
  setIntOrStringPref: function AC_setIntOrStringPref(aEvent) {
    let node = this.getLINodeForEvent(aEvent);

    
    let pref = this._getPrefForNode(node);
    if (pref.locked) {
      return;
    }

    
    if (pref.type == Services.prefs.PREF_BOOL) {
      return;
    }

    
    pref.value = aEvent.target.value;
  },

  
  resetDefaultPref: function AC_resetDefaultPref(aEvent) {
    let node = this.getLINodeForEvent(aEvent);

    
    if (this.selected != node) {
      this.selected = node;
    }

    
    let pref = this._getPrefForNode(node);
    pref.reset();

    
    Services.prefs.savePrefFile(null);
  },

  
  toggleBoolPref: function AC_toggleBoolPref(aEvent) {
    let node = this.getLINodeForEvent(aEvent);

    
    let pref = this._getPrefForNode(node);
    if (pref.locked) {
      return;
    }

    
    pref.value = !pref.value;
    aEvent.target.blur();
  },

  
  incrOrDecrIntPref: function AC_incrOrDecrIntPref(aEvent, aInt) {
    let node = this.getLINodeForEvent(aEvent);

    
    let pref = this._getPrefForNode(node);
    if (pref.locked) {
      return;
    }

    pref.value += aInt;
  },

  
  observe: function AC_observe(aSubject, aTopic, aPrefName) {
    let pref = new Pref(aPrefName);

    
    if (aTopic != "nsPref:changed") {
      return;
    }

    
    if (pref.type == Services.prefs.PREF_INVALID) {
      document.location.reload();
      return;
    }

    
    let item = document.querySelector(".pref-item[name=\"" + CSS.escape(pref.name) + "\"]");
    if (item) {
      item.setAttribute("value", pref.value);
      let input = item.querySelector("input");
      input.setAttribute("value", pref.value);
      input.value = pref.value;

      pref.default ?
        item.querySelector(".reset").setAttribute("disabled", "true") :
        item.querySelector(".reset").removeAttribute("disabled");
      return;
    }

    
    let anyWhere = this._list.filter(i => { return i.name == pref.name });
    if (!anyWhere.length) {
      document.location.reload();
    }
  },

  
  clipboardCopy: function AC_clipboardCopy(aField) {
    let pref = this._getPrefForNode(this.contextMenuLINode);
    if (aField == 'name') {
      gClipboardHelper.copyString(pref.name);
    } else {
      gClipboardHelper.copyString(pref.value);
    }
  }
}










function Pref(aName) {
  this.name = aName;
}

Pref.prototype = {
  get type() {
    return Services.prefs.getPrefType(this.name);
  },

  get value() {
    switch (this.type) {
      case Services.prefs.PREF_BOOL:
        return Services.prefs.getBoolPref(this.name);
      case Services.prefs.PREF_INT:
        return Services.prefs.getIntPref(this.name);
      case Services.prefs.PREF_STRING:
      default:
        return Services.prefs.getCharPref(this.name);
    }

  },
  set value(aPrefValue) {
    switch (this.type) {
      case Services.prefs.PREF_BOOL:
        Services.prefs.setBoolPref(this.name, aPrefValue);
        break;
      case Services.prefs.PREF_INT:
        Services.prefs.setIntPref(this.name, aPrefValue);
        break;
      case Services.prefs.PREF_STRING:
      default:
        Services.prefs.setCharPref(this.name, aPrefValue);
    }

    
    Services.prefs.savePrefFile(null);
  },

  get default() {
    return !Services.prefs.prefHasUserValue(this.name);
  },

  get locked() {
    return Services.prefs.prefIsLocked(this.name);
  },

  reset: function AC_reset() {
    Services.prefs.clearUserPref(this.name);
  },

  test: function AC_test(aValue) {
    return aValue ? aValue.test(this.name) : true;
  },

  
  getOrCreateNewLINode: function AC_getOrCreateNewLINode() {
    if (!this.li) {
      this.li = document.createElement("li");

      this.li.className = "pref-item";
      this.li.setAttribute("name", this.name);

      
      this.li.addEventListener("click",
        function(aEvent) {
          AboutConfig.selected = AboutConfig.getLINodeForEvent(aEvent);
        },
        false
      );

      
      this.li.addEventListener("contextmenu",
        function(aEvent) {
          AboutConfig.contextMenuLINode = AboutConfig.getLINodeForEvent(aEvent);
        },
        false
      );

      this.li.setAttribute("contextmenu", "prefs-context-menu");

      
      this.li.innerHTML =
        "<div class='pref-name' " +
            "onclick='AboutConfig.selectOrToggleBoolPref(event);'>" +
            this.name +
        "</div>" +
        "<div class='pref-item-line'>" +
          "<input class='pref-value' value='' " +
            "onblur='AboutConfig.setIntOrStringPref(event);' " +
            "onclick='AboutConfig.selectOrToggleBoolPref(event);'>" +
          "</input>" +
          "<div class='pref-button reset' " +
            "onclick='AboutConfig.resetDefaultPref(event);'>" +
            gStringBundle.GetStringFromName("pref.resetButton") +
          "</div>" +
          "<div class='pref-button toggle' " +
            "onclick='AboutConfig.toggleBoolPref(event);'>" +
            gStringBundle.GetStringFromName("pref.toggleButton") +
          "</div>" +
          "<div class='pref-button up' " +
            "onclick='AboutConfig.incrOrDecrIntPref(event, 1);'>" +
          "</div>" +
          "<div class='pref-button down' " +
            "onclick='AboutConfig.incrOrDecrIntPref(event, -1);'>" +
          "</div>" +
        "</div>";

      
      setTimeout(this._valueSetup.bind(this), INNERHTML_VALUE_DELAY);
    }

    return this.li;
  },

  
  _valueSetup: function AC_valueSetup() {

    this.li.setAttribute("type", this.type);
    this.li.setAttribute("value", this.value);

    let valDiv = this.li.querySelector(".pref-value");
    valDiv.value = this.value;

    switch(this.type) {
      case Services.prefs.PREF_BOOL:
        valDiv.setAttribute("type", "button");
        this.li.querySelector(".up").setAttribute("disabled", true);
        this.li.querySelector(".down").setAttribute("disabled", true);
        break;
      case Services.prefs.PREF_STRING:
        valDiv.setAttribute("type", "text");
        this.li.querySelector(".up").setAttribute("disabled", true);
        this.li.querySelector(".down").setAttribute("disabled", true);
        this.li.querySelector(".toggle").setAttribute("disabled", true);
        break;
      case Services.prefs.PREF_INT:
        valDiv.setAttribute("type", "number");
        this.li.querySelector(".toggle").setAttribute("disabled", true);
        break;
    }

    this.li.setAttribute("default", this.default);
    if (this.default) {
      this.li.querySelector(".reset").setAttribute("disabled", true);
    }

    if (this.locked) {
      valDiv.setAttribute("disabled", this.locked);
      this.li.querySelector(".pref-name").setAttribute("locked", true);
    }
  }
}

