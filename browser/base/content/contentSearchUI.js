



"use strict";

this.ContentSearchUIController = (function () {

const MAX_DISPLAYED_SUGGESTIONS = 6;
const SUGGESTION_ID_PREFIX = "searchSuggestion";
const ONE_OFF_ID_PREFIX = "oneOff";
const CSS_URI = "chrome://browser/content/contentSearchUI.css";

const HTML_NS = "http://www.w3.org/1999/xhtml";


























function ContentSearchUIController(inputElement, tableParent, healthReportKey,
                                   searchPurpose, idPrefix="") {
  this.input = inputElement;
  this._idPrefix = idPrefix;
  this._healthReportKey = healthReportKey;
  this._searchPurpose = searchPurpose;

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

  this._getSearchEngines();
  this._getStrings();
}

ContentSearchUIController.prototype = {

  
  
  
  remoteTimeout: undefined,
  _oneOffButtons: [],

  get defaultEngine() {
    return this._defaultEngine;
  },

  set defaultEngine(val) {
    this._defaultEngine = val;
    this._updateDefaultEngineHeader();

    if (val && document.activeElement == this.input) {
      this._speculativeConnect();
    }
  },

  get engines() {
    return this._engines;
  },

  set engines(val) {
    this._engines = val;
    this._setUpOneOffButtons();
  },

  
  
  
  get selectedIndex() {
    let allElts = [...this._suggestionsList.children,
                   ...this._oneOffButtons,
                   document.getElementById("contentSearchSettingsButton")];
    for (let i = 0; i < allElts.length; ++i) {
      let elt = allElts[i];
      if (elt.classList.contains("selected")) {
        return i;
      }
    }
    return -1;
  },

  set selectedIndex(idx) {
    
    this._table.removeAttribute("aria-activedescendant");
    this.input.removeAttribute("aria-activedescendant");

    let allElts = [...this._suggestionsList.children,
                   ...this._oneOffButtons,
                   document.getElementById("contentSearchSettingsButton")];
    for (let i = 0; i < allElts.length; ++i) {
      let elt = allElts[i];
      let ariaSelectedElt = i < this.numSuggestions ? elt.firstChild : elt;
      if (i == idx) {
        elt.classList.add("selected");
        ariaSelectedElt.setAttribute("aria-selected", "true");
        this.input.setAttribute("aria-activedescendant", ariaSelectedElt.id);
      }
      else {
        elt.classList.remove("selected");
        ariaSelectedElt.setAttribute("aria-selected", "false");
      }
    }
  },

  get selectedEngineName() {
    let selectedElt = this._table.querySelector(".selected");
    if (selectedElt && selectedElt.engineName) {
      return selectedElt.engineName;
    }
    return this.defaultEngine.name;
  },

  get numSuggestions() {
    return this._suggestionsList.children.length;
  },

  selectAndUpdateInput: function (idx) {
    this.selectedIndex = idx;
    let newValue = this.suggestionAtIndex(idx) || this._stickyInputValue;
    
    
    if (this.input.value != newValue) {
      this.input.value = newValue;
    }
    this._updateSearchWithHeader();
  },

  suggestionAtIndex: function (idx) {
    let row = this._suggestionsList.children[idx];
    return row ? row.textContent : null;
  },

  deleteSuggestionAtIndex: function (idx) {
    
    if (this.isFormHistorySuggestionAtIndex(idx)) {
      let suggestionStr = this.suggestionAtIndex(idx);
      this._sendMsg("RemoveFormHistoryEntry", suggestionStr);
      this._suggestionsList.children[idx].remove();
      this.selectAndUpdateInput(-1);
    }
  },

  isFormHistorySuggestionAtIndex: function (idx) {
    let row = this._suggestionsList.children[idx];
    return row && row.classList.contains("formHistory");
  },

  addInputValueToFormHistory: function () {
    this._sendMsg("AddFormHistoryEntry", this.input.value);
  },

  handleEvent: function (event) {
    this["_on" + event.type[0].toUpperCase() + event.type.substr(1)](event);
  },

  _onCommand: function(aEvent) {
    if (this.selectedIndex == this.numSuggestions + this._oneOffButtons.length) {
      
      this._sendMsg("ManageEngines");
      return;
    }

    this.search(aEvent);

    if (aEvent) {
      aEvent.preventDefault();
    }
  },

  search: function (aEvent) {
    if (!this.defaultEngine) {
      return; 
    }

    let searchText = this.input;
    let searchTerms;
    if (this._table.hidden ||
        aEvent.originalTarget.id == "contentSearchDefaultEngineHeader") {
      searchTerms = searchText.value;
    }
    else {
      searchTerms = this.suggestionAtIndex(this.selectedIndex) || searchText.value;
    }
    
    
    let eventData = {
      engineName: this.selectedEngineName,
      searchString: searchTerms,
      healthReportKey: this._healthReportKey,
      searchPurpose: this._searchPurpose,
      originalEvent: {
        shiftKey: aEvent.shiftKey,
        ctrlKey: aEvent.ctrlKey,
        metaKey: aEvent.metaKey,
        altKey: aEvent.altKey,
        button: aEvent.button,
      },
    };

    if (this.suggestionAtIndex(this.selectedIndex)) {
      eventData.selection = {
        index: this.selectedIndex,
        kind: aEvent instanceof MouseEvent ? "mouse" :
              aEvent instanceof KeyboardEvent ? "key" : undefined,
      };
    }

    this._sendMsg("Search", eventData);
    this.addInputValueToFormHistory();
  },

  _onInput: function () {
    if (!this.input.value) {
      this._stickyInputValue = "";
      this._hideSuggestions();
    }
    else if (this.input.value != this._stickyInputValue) {
      
      this._getSuggestions();
      this.selectAndUpdateInput(-1);
    }
    this._updateSearchWithHeader();
  },

  _onKeypress: function (event) {
    let selectedIndexDelta = 0;
    switch (event.keyCode) {
    case event.DOM_VK_UP:
      if (!this._table.hidden) {
        selectedIndexDelta = -1;
      }
      break;
    case event.DOM_VK_DOWN:
      if (this._table.hidden) {
        this._getSuggestions();
      }
      else {
        selectedIndexDelta = 1;
      }
      break;
    case event.DOM_VK_RIGHT:
      
      if (this.input.selectionStart != this.input.selectionEnd ||
          this.input.selectionEnd != this.input.value.length) {
        return;
      }
      if (this.numSuggestions && this.selectedIndex >= 0 &&
          this.selectedIndex < this.numSuggestions) {
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
    case event.DOM_VK_RETURN:
      this._onCommand(event);
      break;
    case event.DOM_VK_DELETE:
      if (this.selectedIndex >= 0) {
        this.deleteSuggestionAtIndex(this.selectedIndex);
      }
      break;
    case event.DOM_VK_ESCAPE:
      if (!this._table.hidden) {
        this._hideSuggestions();
      }
    default:
      return;
    }

    if (selectedIndexDelta) {
      
      let newSelectedIndex = this.selectedIndex + selectedIndexDelta;
      if (newSelectedIndex < -1) {
        newSelectedIndex = this.numSuggestions + this._oneOffButtons.length;
      }
      else if (this.numSuggestions + this._oneOffButtons.length < newSelectedIndex) {
        newSelectedIndex = -1;
      }
      this.selectAndUpdateInput(newSelectedIndex);

      
      event.preventDefault();
    }
  },

  _onFocus: function () {
    if (this._mousedown) {
      return;
    }
    
    
    
    
    
    this.input.setAttribute("keepfocus", "true");
    this._speculativeConnect();
  },

  _onBlur: function () {
    if (this._mousedown) {
      
      
      
      setTimeout(() => this.input.focus(), 0);
      return;
    }
    this.input.removeAttribute("keepfocus");
    this._hideSuggestions();
  },

  _onMousemove: function (event) {
    this.selectedIndex = this._indexOfTableItem(event.target);
  },

  _onMouseup: function (event) {
    if (event.button == 2) {
      return;
    }
    this._onCommand(event);
  },

  _onClick: function (event) {
    this._onMouseup(event);
  },

  _onContentSearchService: function (event) {
    let methodName = "_onMsg" + event.detail.type;
    if (methodName in this) {
      this[methodName](event.detail.data);
    }
  },

  _onMsgFocusInput: function (event) {
    this.input.focus();
  },

  _onMsgSuggestions: function (suggestions) {
    
    
    
    if (this._stickyInputValue != suggestions.searchString ||
        this.defaultEngine.name != suggestions.engineName) {
      return;
    }

    this._clearSuggestionRows();

    
    let { left } = this.input.getBoundingClientRect();
    this._table.style.top = this.input.offsetHeight + "px";
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
      this._suggestionsList.appendChild(
        this._makeTableRow(type, suggestions[type][idx], i, searchWords));
    }

    if (this._table.hidden) {
      this.selectedIndex = -1;
      this._table.hidden = false;
      this.input.setAttribute("aria-expanded", "true");
    }
  },

  _onMsgState: function (state) {
    this.defaultEngine = {
      name: state.currentEngine.name,
      icon: this._getFaviconURIFromBuffer(state.currentEngine.iconBuffer),
    };
    this.engines = state.engines;
  },

  _onMsgCurrentState: function (state) {
    this._onMsgState(state);
  },

  _onMsgCurrentEngine: function (engine) {
    this.defaultEngine = {
      name: engine.name,
      icon: this._getFaviconURIFromBuffer(engine.iconBuffer),
    };
    this._setUpOneOffButtons();
  },

  _onMsgStrings: function (strings) {
    this._strings = strings;
    this._updateDefaultEngineHeader();
    this._updateSearchWithHeader();
  },

  _updateDefaultEngineHeader: function () {
    let header = document.getElementById("contentSearchDefaultEngineHeader");
    if (this.defaultEngine.icon) {
      header.firstChild.setAttribute("src", this.defaultEngine.icon);
    }
    if (!this._strings) {
      return;
    }
    while (header.firstChild.nextSibling) {
      header.firstChild.nextSibling.remove();
    }
    header.appendChild(document.createTextNode(
      this._strings.searchHeader.replace("%S", this.defaultEngine.name)));
  },

  _updateSearchWithHeader: function () {
    if (!this._strings) {
      return;
    }
    let searchWithHeader = document.getElementById("contentSearchSearchWithHeader");
    while (searchWithHeader.firstChild) {
      searchWithHeader.firstChild.remove();
    }
    if (this.input.value) {
      searchWithHeader.appendChild(document.createTextNode(this._strings.searchFor));
      let span = document.createElementNS(HTML_NS, "span");
      span.setAttribute("class", "contentSearchSearchWithHeaderSearchText");
      span.appendChild(document.createTextNode(" " + this.input.value + " "));
      searchWithHeader.appendChild(span);
      searchWithHeader.appendChild(document.createTextNode(this._strings.searchWith));
      return;
    }
    searchWithHeader.appendChild(document.createTextNode(this._strings.searchWithHeader));
  },

  _speculativeConnect: function () {
    if (this.defaultEngine) {
      this._sendMsg("SpeculativeConnect", this.defaultEngine.name);
    }
  },

  _makeTableRow: function (type, suggestionStr, currentRow, searchWords) {
    let row = document.createElementNS(HTML_NS, "tr");
    row.dir = "auto";
    row.classList.add("contentSearchSuggestionRow");
    row.classList.add(type);
    row.setAttribute("role", "presentation");
    row.addEventListener("mousemove", this);
    row.addEventListener("mouseup", this);

    let entry = document.createElementNS(HTML_NS, "td");
    let img = document.createElementNS(HTML_NS, "div");
    img.setAttribute("class", "historyIcon");
    entry.appendChild(img);
    entry.classList.add("contentSearchSuggestionEntry");
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

  
  _getFaviconURIFromBuffer: function (buffer) {
    let blob = new Blob([buffer]);
    let dpiSize = Math.round(16 * window.devicePixelRatio);
    let sizeStr = dpiSize + "," + dpiSize;
    return URL.createObjectURL(blob) + "#-moz-resolution=" + sizeStr;
  },

  _getSearchEngines: function () {
    this._sendMsg("GetState");
  },

  _getStrings: function () {
    this._sendMsg("GetStrings");
  },

  _getSuggestions: function () {
    this._stickyInputValue = this.input.value;
    if (this.defaultEngine) {
      this._sendMsg("GetSuggestions", {
        engineName: this.defaultEngine.name,
        searchString: this.input.value,
        remoteTimeout: this.remoteTimeout,
      });
    }
  },

  _clearSuggestionRows: function() {
    while (this._suggestionsList.firstElementChild) {
      this._suggestionsList.firstElementChild.remove();
    }
  },

  _hideSuggestions: function () {
    this.input.setAttribute("aria-expanded", "false");
    this._table.hidden = true;
  },

  _indexOfTableItem: function (elt) {
    if (elt.classList.contains("contentSearchOneOffItem")) {
      return this.numSuggestions + this._oneOffButtons.indexOf(elt);
    }
    if (elt.classList.contains("contentSearchSettingsButton")) {
      return this.numSuggestions + this._oneOffButtons.length;
    }
    while (elt && elt.localName != "tr") {
      elt = elt.parentNode;
    }
    if (!elt) {
      throw new Error("Element is not a row");
    }
    return elt.rowIndex;
  },

  _makeTable: function (id) {
    this._table = document.createElementNS(HTML_NS, "table");
    this._table.id = id;
    this._table.hidden = true;
    this._table.classList.add("contentSearchSuggestionTable");
    this._table.setAttribute("role", "presentation");

    
    
    
    this._table.addEventListener("mousedown", () => { this._mousedown = true; });
    document.addEventListener("mouseup", () => { delete this._mousedown; });

    
    this._table.addEventListener("mouseout", () => {
      if (this.selectedIndex >= this.numSuggestions) {
        this.selectAndUpdateInput(-1);
      }
    });

    
    
    
    window.addEventListener("beforeunload", () => { this._hideSuggestions(); });

    let headerRow = document.createElementNS(HTML_NS, "tr");
    let header = document.createElementNS(HTML_NS, "td");
    headerRow.setAttribute("class", "contentSearchHeaderRow");
    header.setAttribute("class", "contentSearchHeader");
    let img = document.createElementNS(HTML_NS, "img");
    img.setAttribute("src", "chrome://browser/skin/search-engine-placeholder.png");
    header.appendChild(img);
    header.id = "contentSearchDefaultEngineHeader";
    headerRow.appendChild(header);
    headerRow.addEventListener("click", this);
    this._table.appendChild(headerRow);

    let row = document.createElementNS(HTML_NS, "tr");
    row.setAttribute("class", "contentSearchSuggestionsContainer");
    let cell = document.createElementNS(HTML_NS, "td");
    cell.setAttribute("class", "contentSearchSuggestionsContainer");
    this._suggestionsList = document.createElementNS(HTML_NS, "table");
    this._suggestionsList.setAttribute("class", "contentSearchSuggestionsList");
    cell.appendChild(this._suggestionsList);
    row.appendChild(cell);
    this._table.appendChild(row);
    this._suggestionsList.setAttribute("role", "listbox");

    this._oneOffsTable = document.createElementNS(HTML_NS, "table");
    this._oneOffsTable.setAttribute("class", "contentSearchOneOffsTable");
    this._oneOffsTable.classList.add("contentSearchSuggestionsContainer");
    this._oneOffsTable.setAttribute("role", "group");
    this._table.appendChild(this._oneOffsTable);

    headerRow = document.createElementNS(HTML_NS, "tr");
    header = document.createElementNS(HTML_NS, "td");
    headerRow.setAttribute("class", "contentSearchHeaderRow");
    header.setAttribute("class", "contentSearchHeader");
    headerRow.appendChild(header);
    header.id = "contentSearchSearchWithHeader";
    this._oneOffsTable.appendChild(headerRow);

    let button = document.createElementNS(HTML_NS, "button");
    button.appendChild(document.createTextNode("Change Search Settings"));
    button.setAttribute("class", "contentSearchSettingsButton");
    button.classList.add("contentSearchHeaderRow");
    button.classList.add("contentSearchHeader");
    button.id = "contentSearchSettingsButton";
    button.addEventListener("click", this);
    button.addEventListener("mousemove", this);
    this._table.appendChild(button);

    return this._table;
  },

  _setUpOneOffButtons: function () {
    
    
    if (!this._engines) {
      return;
    }

    while (this._oneOffsTable.firstChild.nextSibling) {
      this._oneOffsTable.firstChild.nextSibling.remove();
    }

    this._oneOffButtons = [];

    let engines = this._engines.filter(aEngine => aEngine.name != this.defaultEngine.name);
    if (!engines.length) {
      this._oneOffsTable.hidden = true;
      return;
    }

    const kDefaultButtonWidth = 49; 
    let rowWidth = this.input.offsetWidth - 2; 
    let enginesPerRow = Math.floor(rowWidth / kDefaultButtonWidth);
    let buttonWidth = Math.floor(rowWidth / enginesPerRow);

    let row = document.createElementNS(HTML_NS, "tr");
    let cell = document.createElementNS(HTML_NS, "td");
    row.setAttribute("class", "contentSearchSuggestionsContainer");
    cell.setAttribute("class", "contentSearchSuggestionsContainer");

    for (let i = 0; i < engines.length; ++i) {
      let engine = engines[i];
      if (i > 0 && i % enginesPerRow == 0) {
        row.appendChild(cell);
        this._oneOffsTable.appendChild(row);
        row = document.createElementNS(HTML_NS, "tr");
        cell = document.createElementNS(HTML_NS, "td");
        row.setAttribute("class", "contentSearchSuggestionsContainer");
        cell.setAttribute("class", "contentSearchSuggestionsContainer");
      }
      let button = document.createElementNS(HTML_NS, "button");
      button.setAttribute("class", "contentSearchOneOffItem");
      let img = document.createElementNS(HTML_NS, "img");
      let uri = "chrome://browser/skin/search-engine-placeholder.png";
      if (engine.iconBuffer) {
        uri = this._getFaviconURIFromBuffer(engine.iconBuffer);
      }
      img.setAttribute("src", uri);
      button.appendChild(img);
      button.style.width = buttonWidth + "px";
      button.setAttribute("title", engine.name);

      button.engineName = engine.name;
      button.addEventListener("click", this);
      button.addEventListener("mousemove", this);

      if (engines.length - i <= enginesPerRow - (i % enginesPerRow)) {
        button.classList.add("last-row");
      }

      if ((i + 1) % enginesPerRow == 0) {
        button.classList.add("end-of-row");
      }

      button.id = ONE_OFF_ID_PREFIX + i;
      cell.appendChild(button);
      this._oneOffButtons.push(button);
    }
    row.appendChild(cell);
    this._oneOffsTable.appendChild(row);
    this._oneOffsTable.hidden = false;
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

return ContentSearchUIController;
})();
