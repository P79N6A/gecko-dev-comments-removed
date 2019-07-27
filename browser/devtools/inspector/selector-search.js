



"use strict";

const { Cu } = require("chrome");

const promise = require("resource://gre/modules/Promise.jsm").Promise;
loader.lazyGetter(this, "EventEmitter", () => require("devtools/toolkit/event-emitter"));
loader.lazyGetter(this, "AutocompletePopup", () => require("devtools/shared/autocomplete-popup").AutocompletePopup);


const MAX_SUGGESTIONS = 15;
















function SelectorSearch(aInspector, aInputNode) {
  this.inspector = aInspector;
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
    autoSelect: true,
    position: "before_start",
    direction: "ltr",
    theme: "auto",
    onClick: this._onListBoxKeypress,
    onKeypress: this._onListBoxKeypress
  };
  this.searchPopup = new AutocompletePopup(this.panelDoc, options);

  
  this.searchBox.addEventListener("command", this._onHTMLSearch, true);
  this.searchBox.addEventListener("keypress", this._onSearchKeypress, true);

  
  
  this._lastQuery = promise.resolve(null);
  EventEmitter.decorate(this);
}

exports.SelectorSearch = SelectorSearch;

SelectorSearch.prototype = {

  get walker() this.inspector.walker,

  
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

  


  destroy: function() {
    
    this.searchBox.removeEventListener("command", this._onHTMLSearch, true);
    this.searchBox.removeEventListener("keypress", this._onSearchKeypress, true);
    this.searchPopup.destroy();
    this.searchPopup = null;
    this.searchBox = null;
    this.panelDoc = null;
    this._searchResults = null;
    this._searchSuggestions = null;
  },

  _selectResult: function(index) {
    return this._searchResults.item(index).then(node => {
      this.inspector.selection.setNodeFront(node, "selectorsearch");
    });
  },

  _queryNodes: Task.async(function*(query) {
    if (typeof this.hasMultiFrameSearch === "undefined") {
      let target = this.inspector.toolbox.target;
      this.hasMultiFrameSearch = yield target.actorHasMethod("domwalker",
        "multiFrameQuerySelectorAll");
    }

    if (this.hasMultiFrameSearch) {
      return yield this.walker.multiFrameQuerySelectorAll(query);
    } else {
      return yield this.walker.querySelectorAll(this.walker.rootNode, query);
    }
  }),

  



  _onHTMLSearch: function() {
    let query = this.searchBox.value;
    if (query == this._lastSearched) {
      this.emit("processing-done");
      return;
    }
    this._lastSearched = query;
    this._searchResults = [];
    this._searchIndex = 0;

    if (query.length == 0) {
      this._lastValidSearch = "";
      this.searchBox.removeAttribute("filled");
      this.searchBox.classList.remove("devtools-no-search-result");
      if (this.searchPopup.isOpen) {
        this.searchPopup.hidePopup();
      }
      this.emit("processing-done");
      return;
    }

    this.searchBox.setAttribute("filled", true);
    let queryList = null;

    this._lastQuery = this._queryNodes(query).then(list => {
      return list;
    }, (err) => {
      
      return null;
    }).then(queryList => {
      
      if (query != this.searchBox.value) {
        if (queryList) {
          queryList.release();
        }
        return promise.reject(null);
      }

      this._searchResults = queryList || [];
      if (this._searchResults && this._searchResults.length > 0) {
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
          this.searchBox.classList.remove("devtools-no-search-result");

          return this._selectResult(0);
        }
        return this._selectResult(0).then(() => {
          this.searchBox.classList.remove("devtools-no-search-result");
        }).then(() => this.showSuggestions());
      }
      if (query.match(/[\s>+]$/)) {
        this._lastValidSearch = query + "*";
      }
      else if (query.match(/[\s>+][\.#a-zA-Z][\.#>\s+]*$/)) {
        let lastPart = query.match(/[\s+>][\.#a-zA-Z][^>\s+]*$/)[0];
        this._lastValidSearch = query.slice(0, -1 * lastPart.length + 1) + "*";
      }
      this.searchBox.classList.add("devtools-no-search-result");
      return this.showSuggestions();
    }).then(() => this.emit("processing-done"), Cu.reportError);
  },

  


  _onSearchKeypress: function(aEvent) {
    let query = this.searchBox.value;
    switch(aEvent.keyCode) {
      case aEvent.DOM_VK_RETURN:
        if (query == this._lastSearched && this._searchResults) {
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
    if (this._searchResults && this._searchResults.length > 0) {
      this._lastQuery = this._selectResult(this._searchIndex).then(() => this.emit("processing-done"));
    }
    else {
      this.emit("processing-done");
    }
  },

  


  _onListBoxKeypress: function(aEvent) {
    switch(aEvent.keyCode || aEvent.button) {
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
    this.emit("processing-done");
  },

  


  _showPopup: function(aList, aFirstPart) {
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

  



  showSuggestions: function() {
    let query = this.searchBox.value;
    let firstPart = "";
    if (this.state == this.States.TAG) {
      
      
      firstPart = (query.match(/[\s>+]?([a-zA-Z]*)$/) || ["", query])[1];
      query = query.slice(0, query.length - firstPart.length);
    }
    else if (this.state == this.States.CLASS) {
      
      firstPart = query.match(/\.([^\.]*)$/)[1];
      query = query.slice(0, query.length - firstPart.length - 1);
    }
    else if (this.state == this.States.ID) {
      
      firstPart = query.match(/#([^#]*)$/)[1];
      query = query.slice(0, query.length - firstPart.length - 1);
    }
    
    
    if (/[\s+>~]$/.test(query)) {
      query += "*";
    }
    this._currentSuggesting = query;
    return this.walker.getSuggestionsForQuery(query, firstPart, this.state).then(result => {
      if (this._currentSuggesting != result.query) {
        
        
        return;
      }
      this._lastToLastValidSearch = this._lastValidSearch;
      if (this.state == this.States.CLASS) {
        firstPart = "." + firstPart;
      }
      else if (this.state == this.States.ID) {
        firstPart = "#" + firstPart;
      }
      this._showPopup(result.suggestions, firstPart);
    });
  }
};
