



"use strict";

this.SearchSuggestionUIController = (function () {

const MAX_DISPLAYED_SUGGESTIONS = 6;
const SUGGESTION_ID_PREFIX = "searchSuggestion";
const CSS_URI = "chrome://browser/content/searchSuggestionUI.css";

const HTML_NS = "http://www.w3.org/1999/xhtml";


























function SearchSuggestionUIController(inputElement, tableParent, onClick=null,
                                      idPrefix="") {
  this.input = inputElement;
  this.onClick = onClick;
  this._idPrefix = idPrefix;

  let tableID = idPrefix + "searchSuggestionTable";
  this.input.autocomplete = "off";
  this.input.setAttribute("aria-autocomplete", "true");
  this.input.setAttribute("aria-controls", tableID);
  tableParent.appendChild(this._makeTable(tableID));

  this.input.addEventListener("keypress", this);
  this.input.addEventListener("input", this);
  this.input.addEventListener("focus", this);
  this.input.addEventListener("blur", this);
  window.addEventListener("ContentSearchService", this);

  this._stickyInputValue = "";
  this._hideSuggestions();
}

SearchSuggestionUIController.prototype = {

  
  
  
  remoteTimeout: undefined,

  get engineName() {
    return this._engineName;
  },

  set engineName(val) {
    this._engineName = val;
    if (val && document.activeElement == this.input) {
      this._speculativeConnect();
    }
  },

  get selectedIndex() {
    for (let i = 0; i < this._table.children.length; i++) {
      let row = this._table.children[i];
      if (row.classList.contains("selected")) {
        return i;
      }
    }
    return -1;
  },

  set selectedIndex(idx) {
    
    this._table.removeAttribute("aria-activedescendant");
    for (let i = 0; i < this._table.children.length; i++) {
      let row = this._table.children[i];
      if (i == idx) {
        row.classList.add("selected");
        row.firstChild.setAttribute("aria-selected", "true");
        this._table.setAttribute("aria-activedescendant", row.firstChild.id);
        this.input.value = this.suggestionAtIndex(i);
      }
      else {
        row.classList.remove("selected");
        row.firstChild.setAttribute("aria-selected", "false");
      }
    }

    
    if (idx < 0) {
      this.input.value = this._stickyInputValue;
    }
  },

  get numSuggestions() {
    return this._table.children.length;
  },

  suggestionAtIndex: function (idx) {
    let row = this._table.children[idx];
    return row ? row.textContent : null;
  },

  deleteSuggestionAtIndex: function (idx) {
    
    if (this.isFormHistorySuggestionAtIndex(idx)) {
      let suggestionStr = this.suggestionAtIndex(idx);
      this._sendMsg("RemoveFormHistoryEntry", suggestionStr);
      this._table.children[idx].remove();
      this.selectedIndex = -1;
    }
  },

  isFormHistorySuggestionAtIndex: function (idx) {
    let row = this._table.children[idx];
    return row && row.classList.contains("formHistory");
  },

  addInputValueToFormHistory: function () {
    this._sendMsg("AddFormHistoryEntry", this.input.value);
  },

  handleEvent: function (event) {
    this["_on" + event.type[0].toUpperCase() + event.type.substr(1)](event);
  },

  _onInput: function () {
    if (this.input.value) {
      this._getSuggestions();
    }
    else {
      this._stickyInputValue = "";
      this._hideSuggestions();
    }
    this.selectedIndex = -1;
  },

  _onKeypress: function (event) {
    let selectedIndexDelta = 0;
    switch (event.keyCode) {
    case event.DOM_VK_UP:
      if (this.numSuggestions) {
        selectedIndexDelta = -1;
      }
      break;
    case event.DOM_VK_DOWN:
      if (this.numSuggestions) {
        selectedIndexDelta = 1;
      }
      else {
        this._getSuggestions();
      }
      break;
    case event.DOM_VK_RIGHT:
      
      if (this.input.selectionStart != this.input.selectionEnd ||
          this.input.selectionEnd != this.input.value.length) {
        return;
      }
      
    case event.DOM_VK_RETURN:
      if (this.selectedIndex >= 0) {
        this.input.value = this.suggestionAtIndex(this.selectedIndex);
        this.input.setAttribute("selection-index", this.selectedIndex);
        this.input.setAttribute("selection-kind", "key");
      } else {
        
        
        this.input.removeAttribute("selection-index");
        this.input.removeAttribute("selection-kind");
      }
      this._stickyInputValue = this.input.value;
      this._hideSuggestions();
      break;
    case event.DOM_VK_DELETE:
      if (this.selectedIndex >= 0) {
        this.deleteSuggestionAtIndex(this.selectedIndex);
      }
      break;
    default:
      return;
    }

    if (selectedIndexDelta) {
      
      let newSelectedIndex = this.selectedIndex + selectedIndexDelta;
      if (newSelectedIndex < -1) {
        newSelectedIndex = this.numSuggestions - 1;
      }
      else if (this.numSuggestions <= newSelectedIndex) {
        newSelectedIndex = -1;
      }
      this.selectedIndex = newSelectedIndex;

      
      event.preventDefault();
    }
  },

  _onFocus: function () {
    this._speculativeConnect();
  },

  _onBlur: function () {
    this._hideSuggestions();
  },

  _onMousedown: function (event) {
    let idx = this._indexOfTableRowOrDescendent(event.target);
    let suggestion = this.suggestionAtIndex(idx);
    this._stickyInputValue = suggestion;
    this.input.value = suggestion;
    this.input.setAttribute("selection-index", idx);
    this.input.setAttribute("selection-kind", "mouse");
    this._hideSuggestions();
    if (this.onClick) {
      this.onClick.call(null);
    }
  },

  _onContentSearchService: function (event) {
    let methodName = "_onMsg" + event.detail.type;
    if (methodName in this) {
      this[methodName](event.detail.data);
    }
  },

  _onMsgSuggestions: function (suggestions) {
    
    
    
    if (this._stickyInputValue != suggestions.searchString ||
        this.engineName != suggestions.engineName) {
      return;
    }

    
    while (this._table.firstElementChild) {
      this._table.firstElementChild.remove();
    }

    
    let { left, bottom } = this.input.getBoundingClientRect();
    this._table.style.left = (left + window.scrollX) + "px";
    this._table.style.top = (bottom + window.scrollY) + "px";
    this._table.style.minWidth = this.input.offsetWidth + "px";
    this._table.style.maxWidth = (window.innerWidth - left - 40) + "px";

    
    let searchWords =
      new Set(suggestions.searchString.trim().toLowerCase().split(/\s+/));
    for (let i = 0; i < MAX_DISPLAYED_SUGGESTIONS; i++) {
      let type, idx;
      if (i < suggestions.formHistory.length) {
        [type, idx] = ["formHistory", i];
      }
      else {
        let j = i - suggestions.formHistory.length;
        if (j < suggestions.remote.length) {
          [type, idx] = ["remote", j];
        }
        else {
          break;
        }
      }
      this._table.appendChild(this._makeTableRow(type, suggestions[type][idx],
                                                 i, searchWords));
    }

    this._table.hidden = false;
    this.input.setAttribute("aria-expanded", "true");
  },

  _speculativeConnect: function () {
    if (this.engineName) {
      this._sendMsg("SpeculativeConnect", this.engineName);
    }
  },

  _makeTableRow: function (type, suggestionStr, currentRow, searchWords) {
    let row = document.createElementNS(HTML_NS, "tr");
    row.classList.add("searchSuggestionRow");
    row.classList.add(type);
    row.setAttribute("role", "presentation");
    row.addEventListener("mousedown", this);

    let entry = document.createElementNS(HTML_NS, "td");
    entry.classList.add("searchSuggestionEntry");
    entry.setAttribute("role", "option");
    entry.id = this._idPrefix + SUGGESTION_ID_PREFIX + currentRow;
    entry.setAttribute("aria-selected", "false");

    let suggestionWords = suggestionStr.trim().toLowerCase().split(/\s+/);
    for (let i = 0; i < suggestionWords.length; i++) {
      let word = suggestionWords[i];
      let wordSpan = document.createElementNS(HTML_NS, "span");
      if (searchWords.has(word)) {
        wordSpan.classList.add("typed");
      }
      wordSpan.textContent = word;
      entry.appendChild(wordSpan);
      if (i < suggestionWords.length - 1) {
        entry.appendChild(document.createTextNode(" "));
      }
    }

    row.appendChild(entry);
    return row;
  },

  _getSuggestions: function () {
    this._stickyInputValue = this.input.value;
    if (this.engineName) {
      this._sendMsg("GetSuggestions", {
        engineName: this.engineName,
        searchString: this.input.value,
        remoteTimeout: this.remoteTimeout,
      });
    }
  },

  _hideSuggestions: function () {
    this.input.setAttribute("aria-expanded", "false");
    this._table.hidden = true;
    while (this._table.firstElementChild) {
      this._table.firstElementChild.remove();
    }
    this.selectedIndex = -1;
  },

  _indexOfTableRowOrDescendent: function (row) {
    while (row && row.localName != "tr") {
      row = row.parentNode;
    }
    if (!row) {
      throw new Error("Element is not a row");
    }
    return row.rowIndex;
  },

  _makeTable: function (id) {
    this._table = document.createElementNS(HTML_NS, "table");
    this._table.id = id;
    this._table.hidden = true;
    this._table.dir = "auto";
    this._table.classList.add("searchSuggestionTable");
    this._table.setAttribute("role", "listbox");
    return this._table;
  },

  _sendMsg: function (type, data=null) {
    dispatchEvent(new CustomEvent("ContentSearchClient", {
      detail: {
        type: type,
        data: data,
      },
    }));
  },
};

return SearchSuggestionUIController;
})();
