





































const Cu = Components.utils;


const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const XHTML_NS = "http://www.w3.org/1999/xhtml";

const HUD_STRINGS_URI = "chrome://global/locale/headsUpDisplay.properties";


Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "stringBundle", function () {
  return Services.strings.createBundle(HUD_STRINGS_URI);
});


var EXPORTED_SYMBOLS = ["AutocompletePopup"];








function AutocompletePopup(aDocument)
{
  this._document = aDocument;

  
  this._panel = this._document.getElementById("webConsole_autocompletePopup");
  if (!this._panel) {
    this._panel = this._document.createElementNS(XUL_NS, "panel");
    this._panel.setAttribute("id", "webConsole_autocompletePopup");
    this._panel.setAttribute("label",
      stringBundle.GetStringFromName("Autocomplete.label"));
    this._panel.setAttribute("noautofocus", "true");
    this._panel.setAttribute("ignorekeys", "true");
    this._panel.setAttribute("level", "top");

    let mainPopupSet = this._document.getElementById("mainPopupSet");
    if (mainPopupSet) {
      mainPopupSet.appendChild(this._panel);
    }
    else {
      this._document.documentElement.appendChild(this._panel);
    }

    this._list = this._document.createElementNS(XUL_NS, "richlistbox");
    this._list.flex = 1;
    this._panel.appendChild(this._list);

    
    this._panel.width = 1;
    this._panel.height = 1;
    this._panel.openPopup(null, "overlap", 0, 0, false, false);
    this._panel.hidePopup();
    this._panel.width = "";
    this._panel.height = "";
  }
  else {
    this._list = this._panel.firstChild;
  }
}

AutocompletePopup.prototype = {
  _document: null,
  _panel: null,
  _list: null,

  





  openPopup: function AP_openPopup(aAnchor)
  {
    this._panel.openPopup(aAnchor, "after_start", 0, 0, false, false);

    if (this.onSelect) {
      this._list.addEventListener("select", this.onSelect, false);
    }

    if (this.onClick) {
      this._list.addEventListener("click", this.onClick, false);
    }

    this._updateSize();
  },

  


  hidePopup: function AP_hidePopup()
  {
    this._panel.hidePopup();

    if (this.onSelect) {
      this._list.removeEventListener("select", this.onSelect, false);
    }

    if (this.onClick) {
      this._list.removeEventListener("click", this.onClick, false);
    }
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

    this._document = null;
    this._list = null;
    this._panel = null;
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
      
      
      this._document.defaultView.setTimeout(this._updateSize.bind(this), 1);
    }
  },

  




  _updateSize: function AP__updateSize()
  {
    this._list.width = this._panel.clientWidth +
                       this._scrollbarWidth;
  },

  


  clearItems: function AP_clearItems()
  {
    while (this._list.hasChildNodes()) {
      this._list.removeChild(this._list.firstChild);
    }

    
    
    this._list.width = "";
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
    this._list.ensureIndexIsVisible(this._list.selectedIndex);
  },

  



  get selectedItem() {
    return this._list.selectedItem ?
           this._list.selectedItem._autocompleteItem : null;
  },

  





  set selectedItem(aItem) {
    this._list.selectedItem = this._findListItem(aItem);
    this._list.ensureIndexIsVisible(this._list.selectedIndex);
  },

  






  appendItem: function AP_appendItem(aItem)
  {
    let description = this._document.createElementNS(XUL_NS, "description");
    description.textContent = aItem.label;

    let listItem = this._document.createElementNS(XUL_NS, "richlistitem");
    listItem.appendChild(description);
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

