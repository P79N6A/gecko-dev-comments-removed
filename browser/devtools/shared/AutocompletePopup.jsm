




const Cu = Components.utils;


const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["AutocompletePopup"];




















this.AutocompletePopup =
function AutocompletePopup(aDocument,
                           aOptions = {fixedWidth: false,
                                       autoSelect: false,
                                       position: "after_start",
                                       panelId: "devtools_autoCompletePopup"})
{
  this._document = aDocument;

  this.fixedWidth = aOptions.fixedWidth;
  this.autoSelect = aOptions.autoSelect;
  this.position = aOptions.position;
  this.direction = aOptions.direction;

  this.onSelect = aOptions.onSelect;
  this.onClick = aOptions.onClick;
  this.onKeypress = aOptions.onKeypress;

  let id = aOptions.panelId;
  
  this._panel = this._document.getElementById(id);
  if (!this._panel) {
    this._panel = this._document.createElementNS(XUL_NS, "panel");
    this._panel.setAttribute("id", id);
    this._panel.setAttribute("class", "devtools-autocomplete-popup");

    this._panel.setAttribute("noautofocus", "true");
    this._panel.setAttribute("level", "top");
    if (!aOptions.onKeypress) {
      this._panel.setAttribute("ignorekeys", "true");
    }

    let mainPopupSet = this._document.getElementById("mainPopupSet");
    if (mainPopupSet) {
      mainPopupSet.appendChild(this._panel);
    }
    else {
      this._document.documentElement.appendChild(this._panel);
    }
    this._list = null;
  }
  else {
    this._list = this._panel.firstChild;
  }

  if (!this._list) {
    this._list = this._document.createElementNS(XUL_NS, "richlistbox");
    this._panel.appendChild(this._list);

    
    this._panel.openPopup(null, this.popup, 0, 0);
    this._panel.hidePopup();
  }

  this._list.flex = 1;
  this._list.setAttribute("seltype", "single");

  if (aOptions.listBoxId) {
    this._list.setAttribute("id", aOptions.listBoxId);
  }
  this._list.setAttribute("class", "devtools-autocomplete-listbox");


  if (this.onSelect) {
    this._list.addEventListener("select", this.onSelect, false);
  }

  if (this.onClick) {
    this._list.addEventListener("click", this.onClick, false);
  }

  if (this.onKeypress) {
    this._list.addEventListener("keypress", this.onKeypress, false);
  }
}

AutocompletePopup.prototype = {
  _document: null,
  _panel: null,
  _list: null,

  
  onSelect: null,
  onClick: null,
  onKeypress: null,

  





  openPopup: function AP_openPopup(aAnchor)
  {
    this._panel.openPopup(aAnchor, this.position, 0, 0);

    if (this.autoSelect) {
      this.selectFirstItem();
    }
    if (!this.fixedWidth) {
      this._updateSize();
    }
  },

  


  hidePopup: function AP_hidePopup()
  {
    this._panel.hidePopup();
  },

  


  get isOpen() {
    return this._panel.state == "open";
  },

  





  destroy: function AP_destroy()
  {
    if (this.isOpen) {
      this.hidePopup();
    }
    this.clearItems();

    if (this.onSelect) {
      this._list.removeEventListener("select", this.onSelect, false);
    }

    if (this.onClick) {
      this._list.removeEventListener("click", this.onClick, false);
    }

    if (this.onKeypress) {
      this._list.removeEventListener("keypress", this.onKeypress, false);
    }

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
      if (!this.fixedWidth) {
        this._updateSize();
      }
    }
  },

  




  selectFirstItem: function AP_selectFirstItem()
  {
    if (this.position.contains("before")) {
      this.selectedIndex = this.itemCount - 1;
    }
    else {
      this.selectedIndex = 0;
    }
  },

  




  _updateSize: function AP__updateSize()
  {
    
    
    this._document.defaultView.setTimeout(function() {
      if (!this._panel) {
        return;
      }
      this._list.width = this._panel.clientWidth +
                         this._scrollbarWidth;
      
      
      this._list.height = this._panel.clientHeight;
      
      this._list.top = 0;
      
      
      this._list.ensureIndexIsVisible(this._list.selectedIndex);
    }.bind(this), 5);
  },

  


  clearItems: function AP_clearItems()
  {
    
    this.selectedIndex = -1;

    while (this._list.hasChildNodes()) {
      this._list.removeChild(this._list.firstChild);
    }

    if (!this.fixedWidth) {
      
      
      this._list.width = "";
      this._list.height = "";
      this._panel.width = "";
      this._panel.height = "";
      this._panel.top = "";
      this._panel.left = "";
    }
  },

  




  get selectedIndex() {
    return this._list.selectedIndex;
  },

  





  set selectedIndex(aIndex) {
    this._list.selectedIndex = aIndex;
    if (this.isOpen) {
      this._list.ensureIndexIsVisible(this._list.selectedIndex);
    }
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
  },

  















  appendItem: function AP_appendItem(aItem)
  {
    let listItem = this._document.createElementNS(XUL_NS, "richlistitem");
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

  





  selectNextItem: function AP_selectNextItem()
  {
    if (this.selectedIndex < (this.itemCount - 1)) {
      this.selectedIndex++;
    }
    else {
      this.selectedIndex = -1;
    }

    return this.selectedItem;
  },

  





  selectPreviousItem: function AP_selectPreviousItem()
  {
    if (this.selectedIndex > -1) {
      this.selectedIndex--;
    }
    else {
      this.selectedIndex = this.itemCount - 1;
    }

    return this.selectedItem;
  },

  


  focus: function AP_focus()
  {
    this._list.focus();
  },

  




  get _scrollbarWidth()
  {
    if (this.__scrollbarWidth) {
      return this.__scrollbarWidth;
    }

    let hbox = this._document.createElementNS(XUL_NS, "hbox");
    hbox.setAttribute("style", "height: 0%; overflow: hidden");

    let scrollbar = this._document.createElementNS(XUL_NS, "scrollbar");
    scrollbar.setAttribute("orient", "vertical");
    hbox.appendChild(scrollbar);

    this._document.documentElement.appendChild(hbox);
    this.__scrollbarWidth = scrollbar.clientWidth;
    this._document.documentElement.removeChild(hbox);

    return this.__scrollbarWidth;
  },
};

