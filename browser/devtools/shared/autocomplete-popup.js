




"use strict";

const {Cc, Ci, Cu} = require("chrome");
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

loader.lazyImporter(this, "Services", "resource://gre/modules/Services.jsm");
loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");
const events  = require("devtools/toolkit/event-emitter");




















function AutocompletePopup(aDocument, aOptions = {})
{
  this._document = aDocument;

  this.autoSelect = aOptions.autoSelect || false;
  this.position = aOptions.position || "after_start";
  this.direction = aOptions.direction || "ltr";

  this.onSelect = aOptions.onSelect;
  this.onClick = aOptions.onClick;
  this.onKeypress = aOptions.onKeypress;

  let id = aOptions.panelId || "devtools_autoCompletePopup";
  let theme = aOptions.theme || "dark";
  
  if (theme == "auto") {
    theme = Services.prefs.getCharPref("devtools.theme");
    this.autoThemeEnabled = true;
    
    this._handleThemeChange = this._handleThemeChange.bind(this);
    gDevTools.on("pref-changed", this._handleThemeChange);
  }
  
  this._panel = this._document.getElementById(id);
  if (!this._panel) {
    this._panel = this._document.createElementNS(XUL_NS, "panel");
    this._panel.setAttribute("id", id);
    this._panel.className = "devtools-autocomplete-popup devtools-monospace "
                            + theme + "-theme";

    this._panel.setAttribute("noautofocus", "true");
    this._panel.setAttribute("level", "top");
    if (!aOptions.onKeypress) {
      this._panel.setAttribute("ignorekeys", "true");
    }
    
    this._panel.setAttribute("role", "presentation");

    let mainPopupSet = this._document.getElementById("mainPopupSet");
    if (mainPopupSet) {
      mainPopupSet.appendChild(this._panel);
    }
    else {
      this._document.documentElement.appendChild(this._panel);
    }
  }
  else {
    this._list = this._panel.firstChild;
  }

  if (!this._list) {
    this._list = this._document.createElementNS(XUL_NS, "richlistbox");
    this._panel.appendChild(this._list);

    
    this._panel.openPopup(null, this.position, 0, 0);
    this._panel.hidePopup();
  }

  this._list.setAttribute("flex", "1");
  this._list.setAttribute("seltype", "single");

  if (aOptions.listBoxId) {
    this._list.setAttribute("id", aOptions.listBoxId);
  }
  this._list.className = "devtools-autocomplete-listbox " + theme + "-theme";

  if (this.onSelect) {
    this._list.addEventListener("select", this.onSelect, false);
  }

  if (this.onClick) {
    this._list.addEventListener("click", this.onClick, false);
  }

  if (this.onKeypress) {
    this._list.addEventListener("keypress", this.onKeypress, false);
  }
  this._itemIdCounter = 0;

  events.decorate(this);
}
exports.AutocompletePopup = AutocompletePopup;

AutocompletePopup.prototype = {
  _document: null,
  _panel: null,
  _list: null,
  __scrollbarWidth: null,

  
  onSelect: null,
  onClick: null,
  onKeypress: null,

  











  openPopup: function AP_openPopup(aAnchor, aXOffset = 0, aYOffset = 0)
  {
    this.__maxLabelLength = -1;
    this._updateSize();
    this._panel.openPopup(aAnchor, this.position, aXOffset, aYOffset);

    if (this.autoSelect) {
      this.selectFirstItem();
    }

    this.emit("popup-opened");
  },

  


  hidePopup: function AP_hidePopup()
  {
    
    this._document.activeElement.removeAttribute("aria-activedescendant");
    this._panel.hidePopup();
  },

  


  get isOpen() {
    return this._panel &&
           (this._panel.state == "open" || this._panel.state == "showing");
  },

  





  destroy: function AP_destroy()
  {
    if (this.isOpen) {
      this.hidePopup();
    }

    if (this.onSelect) {
      this._list.removeEventListener("select", this.onSelect, false);
    }

    if (this.onClick) {
      this._list.removeEventListener("click", this.onClick, false);
    }

    if (this.onKeypress) {
      this._list.removeEventListener("keypress", this.onKeypress, false);
    }

    if (this.autoThemeEnabled) {
      gDevTools.off("pref-changed", this._handleThemeChange);
    }

    this._list.remove();
    this._panel.remove();
    this._document = null;
    this._list = null;
    this._panel = null;
  },

  






  getItemAtIndex: function AP_getItemAtIndex(aIndex)
  {
    return this._list.getItemAtIndex(aIndex)._autocompleteItem;
  },

  





  getItems: function AP_getItems()
  {
    let items = [];

    Array.forEach(this._list.childNodes, function(aItem) {
      items.push(aItem._autocompleteItem);
    });

    return items;
  },

  





  setItems: function AP_setItems(aItems)
  {
    this.clearItems();
    aItems.forEach(this.appendItem, this);

    
    if (this.isOpen) {
      if (this.autoSelect) {
        this.selectFirstItem();
      }
      this._updateSize();
    }
  },

  




  selectFirstItem: function AP_selectFirstItem()
  {
    if (this.position.includes("before")) {
      this.selectedIndex = this.itemCount - 1;
    }
    else {
      this.selectedIndex = 0;
    }
    this._list.ensureIndexIsVisible(this._list.selectedIndex);
  },

  __maxLabelLength: -1,

  get _maxLabelLength() {
    if (this.__maxLabelLength != -1) {
      return this.__maxLabelLength;
    }

    let max = 0;
    for (let i = 0; i < this._list.childNodes.length; i++) {
      let item = this._list.childNodes[i]._autocompleteItem;
      let str = item.label;
      if (item.count) {
        str += (item.count + "");
      }
      max = Math.max(str.length, max);
    }

    this.__maxLabelLength = max;
    return this.__maxLabelLength;
  },

  




  _updateSize: function AP__updateSize()
  {
    if (!this._panel) {
      return;
    }

    this._list.style.width = (this._maxLabelLength + 3) +"ch";
    this._list.ensureIndexIsVisible(this._list.selectedIndex);
  },

  




  _updateAriaActiveDescendant: function AP__updateAriaActiveDescendant()
  {
    if (!this._list.selectedItem) {
      
      this._document.activeElement.removeAttribute("aria-activedescendant");
      return;
    }
    
    this._document.activeElement.setAttribute("aria-activedescendant",
                                              this._list.selectedItem.id);
  },

  


  clearItems: function AP_clearItems()
  {
    
    this.selectedIndex = -1;

    while (this._list.hasChildNodes()) {
      this._list.removeChild(this._list.firstChild);
    }

    this.__maxLabelLength = -1;

    
    
    this._list.width = "";
    this._list.style.width = "";
    this._list.height = "";
    this._panel.width = "";
    this._panel.height = "";
    this._panel.top = "";
    this._panel.left = "";
  },

  




  get selectedIndex() {
    return this._list.selectedIndex;
  },

  





  set selectedIndex(aIndex) {
    this._list.selectedIndex = aIndex;
    if (this.isOpen && this._list.ensureIndexIsVisible) {
      this._list.ensureIndexIsVisible(this._list.selectedIndex);
    }
    this._updateAriaActiveDescendant();
  },

  



  get selectedItem() {
    return this._list.selectedItem ?
           this._list.selectedItem._autocompleteItem : null;
  },

  





  set selectedItem(aItem) {
    this._list.selectedItem = this._findListItem(aItem);
    if (this.isOpen) {
      this._list.ensureIndexIsVisible(this._list.selectedIndex);
    }
    this._updateAriaActiveDescendant();
  },

  















  appendItem: function AP_appendItem(aItem)
  {
    let listItem = this._document.createElementNS(XUL_NS, "richlistitem");
    
    listItem.id = this._panel.id + "_item_" + this._itemIdCounter++;
    if (this.direction) {
      listItem.setAttribute("dir", this.direction);
    }
    let label = this._document.createElementNS(XUL_NS, "label");
    label.setAttribute("value", aItem.label);
    label.setAttribute("class", "autocomplete-value");
    if (aItem.preLabel) {
      let preDesc = this._document.createElementNS(XUL_NS, "label");
      preDesc.setAttribute("value", aItem.preLabel);
      preDesc.setAttribute("class", "initial-value");
      listItem.appendChild(preDesc);
      label.setAttribute("value", aItem.label.slice(aItem.preLabel.length));
    }
    listItem.appendChild(label);
    if (aItem.count && aItem.count > 1) {
      let countDesc = this._document.createElementNS(XUL_NS, "label");
      countDesc.setAttribute("value", aItem.count);
      countDesc.setAttribute("flex", "1");
      countDesc.setAttribute("class", "autocomplete-count");
      listItem.appendChild(countDesc);
    }
    listItem._autocompleteItem = aItem;

    this._list.appendChild(listItem);
  },

  











  _findListItem: function AP__findListItem(aItem)
  {
    for (let i = 0; i < this._list.childNodes.length; i++) {
      let child = this._list.childNodes[i];
      if (child._autocompleteItem == aItem) {
        return child;
      }
    }
    return null;
  },

  





  removeItem: function AP_removeItem(aItem)
  {
    let item = this._findListItem(aItem);
    if (!item) {
      throw new Error("Item not found!");
    }
    this._list.removeChild(item);
  },

  



  get itemCount() {
    return this._list.childNodes.length;
  },

  






  get _itemHeight() {
    return this._list.selectedItem.clientHeight;
  },

  





  selectNextItem: function AP_selectNextItem()
  {
    if (this.selectedIndex < (this.itemCount - 1)) {
      this.selectedIndex++;
    }
    else {
      this.selectedIndex = 0;
    }

    return this.selectedItem;
  },

  





  selectPreviousItem: function AP_selectPreviousItem()
  {
    if (this.selectedIndex > 0) {
      this.selectedIndex--;
    }
    else {
      this.selectedIndex = this.itemCount - 1;
    }

    return this.selectedItem;
  },

  






  selectNextPageItem: function AP_selectNextPageItem()
  {
    let itemsPerPane = Math.floor(this._list.scrollHeight / this._itemHeight);
    let nextPageIndex = this.selectedIndex + itemsPerPane + 1;
    this.selectedIndex = nextPageIndex > this.itemCount - 1 ?
      this.itemCount - 1 : nextPageIndex;

    return this.selectedItem;
  },

  






  selectPreviousPageItem: function AP_selectPreviousPageItem()
  {
    let itemsPerPane = Math.floor(this._list.scrollHeight / this._itemHeight);
    let prevPageIndex = this.selectedIndex - itemsPerPane - 1;
    this.selectedIndex = prevPageIndex < 0 ? 0 : prevPageIndex;

    return this.selectedItem;
  },

  


  focus: function AP_focus()
  {
    this._list.focus();
  },

  













  _handleThemeChange: function AP__handleThemeChange(aEvent, aData)
  {
    if (aData.pref == "devtools.theme") {
      this._panel.classList.toggle(aData.oldValue + "-theme", false);
      this._panel.classList.toggle(aData.newValue + "-theme", true);
      this._list.classList.toggle(aData.oldValue + "-theme", false);
      this._list.classList.toggle(aData.newValue + "-theme", true);
    }
  },
};
