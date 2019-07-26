



"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AutocompletePopup",
                                  "resource:///modules/devtools/AutocompletePopup.jsm");
this.EXPORTED_SYMBOLS = ["SelectorSearch"];


const MAX_SUGGESTIONS = 15;














this.SelectorSearch = function(aContentDocument, aInputNode, aCallback) {
  this.doc = aContentDocument;
  this.callback = aCallback;
  this.searchBox = aInputNode;
  this.panelDoc = this.searchBox.ownerDocument;

  
  this._lastSearched = null;
  this._lastValidSearch = "";
  this._lastToLastValidSearch = null;
  this._searchResults = null;
  this._searchSuggestions = {};
  this._searchIndex = 0;

  
  this._showPopup = this._showPopup.bind(this);
  this._onHTMLSearch = this._onHTMLSearch.bind(this);
  this._onSearchKeypress = this._onSearchKeypress.bind(this);
  this._onListBoxKeypress = this._onListBoxKeypress.bind(this);

  
  let options = {
    panelId: "inspector-searchbox-panel",
    listBoxId: "searchbox-panel-listbox",
    fixedWidth: true,
    autoSelect: true,
    position: "before_start",
    direction: "ltr",
    onClick: this._onListBoxKeypress,
    onKeypress: this._onListBoxKeypress,
  };
  this.searchPopup = new AutocompletePopup(this.panelDoc, options);

  
  this.searchBox.addEventListener("command", this._onHTMLSearch, true);
  this.searchBox.addEventListener("keypress", this._onSearchKeypress, true);
}

this.SelectorSearch.prototype = {

  
  States: {
    CLASS: "class",
    ID: "id",
    TAG: "tag",
  },

  
  _state: null,

  
  _lastStateCheckAt: null,

  









  get state() {
    if (!this.searchBox || !this.searchBox.value) {
      return null;
    }

    let query = this.searchBox.value;
    if (this._lastStateCheckAt == query) {
      
      return this._state;
    }
    this._lastStateCheckAt = query;

    this._state = null;
    let subQuery = "";
    
    
    
    
    
    
    
    for (let i = 1; i <= query.length; i++) {
      
      subQuery = query.slice(0, i);
      let [secondLastChar, lastChar] = subQuery.slice(-2);
      switch (this._state) {
        case null:
          
          lastChar = secondLastChar;
        case this.States.TAG:
          this._state = lastChar == "."
            ? this.States.CLASS
            : lastChar == "#"
              ? this.States.ID
              : this.States.TAG;
          break;

        case this.States.CLASS:
          if (subQuery.match(/[\.]+[^\.]*$/)[0].length > 2) {
            
            this._state = (lastChar == " " || lastChar == ">")
            ? this.States.TAG
            : lastChar == "#"
              ? this.States.ID
              : this.States.CLASS;
          }
          break;

        case this.States.ID:
          if (subQuery.match(/[#]+[^#]*$/)[0].length > 2) {
            
            this._state = (lastChar == " " || lastChar == ">")
            ? this.States.TAG
            : lastChar == "."
              ? this.States.CLASS
              : this.States.ID;
          }
          break;
      }
    }
    return this._state;
  },

  


  destroy: function SelectorSearch_destroy() {
    
    this.searchBox.removeEventListener("command", this._onHTMLSearch, true);
    this.searchBox.removeEventListener("keypress", this._onSearchKeypress, true);
    this.searchPopup.destroy();
    this.searchPopup = null;
    this.searchBox = null;
    this.doc = null;
    this.panelDoc = null;
    this._searchResults = null;
    this._searchSuggestions = null;
    this.callback = null;
  },

  



  _onHTMLSearch: function SelectorSearch__onHTMLSearch() {
    let query = this.searchBox.value;
    if (query == this._lastSearched) {
      return;
    }
    this._lastSearched = query;
    this._searchIndex = 0;

    if (query.length == 0) {
      this._lastValidSearch = "";
      this.searchBox.removeAttribute("filled");
      this.searchBox.classList.remove("devtools-no-search-result");
      if (this.searchPopup.isOpen) {
        this.searchPopup.hidePopup();
      }
      return;
    }

    this.searchBox.setAttribute("filled", true);
    try {
      this._searchResults = this.doc.querySelectorAll(query);
    }
    catch (ex) {
      this._searchResults = [];
    }
    if (this._searchResults.length > 0) {
      this._lastValidSearch = query;
      
      
      if (query.match(/[\s>+]$/)) {
        
        
        this._lastValidSearch += "*";
      }
      else if (query.match(/[\s>+][\.#a-zA-Z][\.#>\s+]*$/)) {
        
        
        
        let lastPart = query.match(/[\s>+][\.#a-zA-Z][^>\s+]*$/)[0];
        this._lastValidSearch = query.slice(0, -1 * lastPart.length + 1) + "*";
      }

      if (!query.slice(-1).match(/[\.#\s>+]/)) {
        
        
        
        if (this.searchPopup.isOpen) {
          this.searchPopup.hidePopup();
        }
      }
      else {
        this.showSuggestions();
      }
      this.searchBox.classList.remove("devtools-no-search-result");
      this.callback(this._searchResults[0]);
    }
    else {
      if (query.match(/[\s>+]$/)) {
        this._lastValidSearch = query + "*";
      }
      else if (query.match(/[\s>+][\.#a-zA-Z][\.#>\s+]*$/)) {
        let lastPart = query.match(/[\s+>][\.#a-zA-Z][^>\s+]*$/)[0];
        this._lastValidSearch = query.slice(0, -1 * lastPart.length + 1) + "*";
      }
      this.searchBox.classList.add("devtools-no-search-result");
      this.showSuggestions();
    }
  },

  


  _onSearchKeypress: function SelectorSearch__onSearchKeypress(aEvent) {
    let query = this.searchBox.value;
    switch(aEvent.keyCode) {
      case aEvent.DOM_VK_ENTER:
      case aEvent.DOM_VK_RETURN:
        if (query == this._lastSearched) {
          this._searchIndex = (this._searchIndex + 1) % this._searchResults.length;
        }
        else {
          this._onHTMLSearch();
          return;
        }
        break;

      case aEvent.DOM_VK_UP:
        if (this.searchPopup.isOpen && this.searchPopup.itemCount > 0) {
          this.searchPopup.focus();
          if (this.searchPopup.selectedIndex == this.searchPopup.itemCount - 1) {
            this.searchPopup.selectedIndex =
              Math.max(0, this.searchPopup.itemCount - 2);
          }
          else {
            this.searchPopup.selectedIndex = this.searchPopup.itemCount - 1;
          }
          this.searchBox.value = this.searchPopup.selectedItem.label;
        }
        else if (--this._searchIndex < 0) {
          this._searchIndex = this._searchResults.length - 1;
        }
        break;

      case aEvent.DOM_VK_DOWN:
        if (this.searchPopup.isOpen && this.searchPopup.itemCount > 0) {
          this.searchPopup.focus();
          this.searchPopup.selectedIndex = 0;
          this.searchBox.value = this.searchPopup.selectedItem.label;
        }
        this._searchIndex = (this._searchIndex + 1) % this._searchResults.length;
        break;

      case aEvent.DOM_VK_TAB:
        if (this.searchPopup.isOpen &&
            this.searchPopup.getItemAtIndex(this.searchPopup.itemCount - 1)
                .preLabel == query) {
          this.searchPopup.selectedIndex = this.searchPopup.itemCount - 1;
          this.searchBox.value = this.searchPopup.selectedItem.label;
          this._onHTMLSearch();
        }
        break;

      case aEvent.DOM_VK_BACK_SPACE:
      case aEvent.DOM_VK_DELETE:
        
        this._lastToLastValidSearch = null;
        
        
        
        this._lastValidSearch = (query.match(/(.*)[\.#][^\.# ]{0,}$/) ||
                                 query.match(/(.*[\s>+])[a-zA-Z][^\.# ]{0,}$/) ||
                                 ["",""])[1];
        return;

      default:
        return;
    }

    aEvent.preventDefault();
    aEvent.stopPropagation();
    if (this._searchResults.length > 0) {
      this.callback(this._searchResults[this._searchIndex]);
    }
  },

  


  _onListBoxKeypress: function SelectorSearch__onListBoxKeypress(aEvent) {
    switch(aEvent.keyCode || aEvent.button) {
      case aEvent.DOM_VK_ENTER:
      case aEvent.DOM_VK_RETURN:
      case aEvent.DOM_VK_TAB:
      case 0: 
        aEvent.stopPropagation();
        aEvent.preventDefault();
        this.searchBox.value = this.searchPopup.selectedItem.label;
        this.searchBox.focus();
        this._onHTMLSearch();
        break;

      case aEvent.DOM_VK_UP:
        if (this.searchPopup.selectedIndex == 0) {
          this.searchPopup.selectedIndex = -1;
          aEvent.stopPropagation();
          aEvent.preventDefault();
          this.searchBox.focus();
        }
        else {
          let index = this.searchPopup.selectedIndex;
          this.searchBox.value = this.searchPopup.getItemAtIndex(index - 1).label;
        }
        break;

      case aEvent.DOM_VK_DOWN:
        if (this.searchPopup.selectedIndex == this.searchPopup.itemCount - 1) {
          this.searchPopup.selectedIndex = -1;
          aEvent.stopPropagation();
          aEvent.preventDefault();
          this.searchBox.focus();
        }
        else {
          let index = this.searchPopup.selectedIndex;
          this.searchBox.value = this.searchPopup.getItemAtIndex(index + 1).label;
        }
        break;

      case aEvent.DOM_VK_BACK_SPACE:
        aEvent.stopPropagation();
        aEvent.preventDefault();
        this.searchBox.focus();
        if (this.searchBox.selectionStart > 0) {
          this.searchBox.value =
            this.searchBox.value.substring(0, this.searchBox.selectionStart - 1);
        }
        this._lastToLastValidSearch = null;
        let query = this.searchBox.value;
        this._lastValidSearch = (query.match(/(.*)[\.#][^\.# ]{0,}$/) ||
                                 query.match(/(.*[\s>+])[a-zA-Z][^\.# ]{0,}$/) ||
                                 ["",""])[1];
        this._onHTMLSearch();
        break;
    }
  },

  
  


  _showPopup: function SelectorSearch__showPopup(aList, aFirstPart) {
    
    aList = aList.sort();
    
    aList = aList.sort(function([a1,a2], [b1,b2]) {
      return a2 < b2;
    });

    let total = 0;
    let query = this.searchBox.value;
    let toLowerCase = false;
    let items = [];
    
    if (query.match(/.*[\.#][^\.#]{0,}$/) == null) {
      toLowerCase = true;
    }
    for (let [value, count] of aList) {
      
      if (query.match(/[\s>+]$/)) {
        value = query + value;
      }
      
      else if (query.match(/[\s>+][\.#a-zA-Z][^\s>+\.#]*$/)) {
        let lastPart = query.match(/[\s>+][\.#a-zA-Z][^>\s+\.#]*$/)[0];
        value = query.slice(0, -1 * lastPart.length + 1) + value;
      }
      
      else if (query.match(/[a-zA-Z][#\.][^#\.\s+>]*$/)) {
        let lastPart = query.match(/[a-zA-Z][#\.][^#\.\s>+]*$/)[0];
        value = query.slice(0, -1 * lastPart.length + 1) + value;
      }
      let item = {
        preLabel: query,
        label: value,
        count: count
      };
      if (toLowerCase) {
        item.label = value.toLowerCase();
      }
      items.unshift(item);
      if (++total > MAX_SUGGESTIONS - 1) {
        break;
      }
    }
    if (total > 0) {
      this.searchPopup.setItems(items);
      this.searchPopup.openPopup(this.searchBox);
    }
    else {
      this.searchPopup.hidePopup();
    }
  },

  



  showSuggestions: function SelectorSearch_showSuggestions() {
    let query = this.searchBox.value;
    if (this._lastValidSearch != "" &&
        this._lastToLastValidSearch != this._lastValidSearch) {
      this._searchSuggestions = {
        ids: new Map(),
        classes: new Map(),
        tags: new Map(),
      };

      let nodes = [];
      try {
        nodes = this.doc.querySelectorAll(this._lastValidSearch);
      } catch (ex) {}
      for (let node of nodes) {
        this._searchSuggestions.ids.set(node.id, 1);
        this._searchSuggestions.tags
            .set(node.tagName,
                 (this._searchSuggestions.tags.get(node.tagName) || 0) + 1);
        for (let className of node.classList) {
          this._searchSuggestions.classes
            .set(className,
                 (this._searchSuggestions.classes.get(className) || 0) + 1);
        }
      }
      this._lastToLastValidSearch = this._lastValidSearch;
    }
    else if (this._lastToLastValidSearch != this._lastValidSearch) {
      this._searchSuggestions = {
        ids: new Map(),
        classes: new Map(),
        tags: new Map(),
      };

      if (query.length == 0) {
        return;
      }

      let nodes = null;
      if (this.state == this.States.CLASS) {
        nodes = this.doc.querySelectorAll("[class]");
        for (let node of nodes) {
          for (let className of node.classList) {
            this._searchSuggestions.classes
              .set(className,
                   (this._searchSuggestions.classes.get(className) || 0) + 1);
          }
        }
      }
      else if (this.state == this.States.ID) {
        nodes = this.doc.querySelectorAll("[id]");
        for (let node of nodes) {
          this._searchSuggestions.ids.set(node.id, 1);
        }
      }
      else if (this.state == this.States.TAG) {
        nodes = this.doc.getElementsByTagName("*");
        for (let node of nodes) {
          this._searchSuggestions.tags
              .set(node.tagName,
                   (this._searchSuggestions.tags.get(node.tagName) || 0) + 1);
        }
      }
      else {
        return;
      }
      this._lastToLastValidSearch = this._lastValidSearch;
    }

    
    let result = [];
    let firstPart = "";
    if (this.state == this.States.TAG) {
      
      
      firstPart = (query.match(/[\s>+]?([a-zA-Z]*)$/) || ["",query])[1];
      for (let [tag, count] of this._searchSuggestions.tags) {
        if (tag.toLowerCase().startsWith(firstPart.toLowerCase())) {
          result.push([tag, count]);
        }
      }
    }
    else if (this.state == this.States.CLASS) {
      
      firstPart = query.match(/\.([^\.]*)$/)[1];
      for (let [className, count] of this._searchSuggestions.classes) {
        if (className.startsWith(firstPart)) {
          result.push(["." + className, count]);
        }
      }
      firstPart = "." + firstPart;
    }
    else if (this.state == this.States.ID) {
      
      firstPart = query.match(/#([^#]*)$/)[1];
      for (let [id, count] of this._searchSuggestions.ids) {
        if (id.startsWith(firstPart)) {
          result.push(["#" + id, 1]);
        }
      }
      firstPart = "#" + firstPart;
    }

    this._showPopup(result, firstPart);
  },
};
