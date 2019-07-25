




"use strict";

const BREAKPOINT_LINE_TOOLTIP_MAX_SIZE = 1000; 
const PROPERTY_VIEW_FLASH_DURATION = 400; 
const GLOBAL_SEARCH_MATCH_FLASH_DURATION = 100; 
const GLOBAL_SEARCH_URL_MAX_SIZE = 100; 
const GLOBAL_SEARCH_LINE_MAX_SIZE = 300; 
const GLOBAL_SEARCH_ACTION_DELAY = 150; 

const SEARCH_GLOBAL_FLAG = "!";
const SEARCH_LINE_FLAG = ":";
const SEARCH_TOKEN_FLAG = "#";





let DebuggerView = {

  


  editor: null,

  


  cacheView: function DV_cacheView() {
    this._onTogglePanesButtonPressed = this._onTogglePanesButtonPressed.bind(this);

    this._togglePanesButton = document.getElementById("toggle-panes");
    this._stackframesAndBreakpoints = document.getElementById("stackframes+breakpoints");
    this._stackframes = document.getElementById("stackframes");
    this._breakpoints = document.getElementById("breakpoints");
    this._variables = document.getElementById("variables");
    this._globalSearch = document.getElementById("globalsearch");
  },

  


  initializePanes: function DV_initializePanes() {
    this._togglePanesButton.addEventListener("click", this._onTogglePanesButtonPressed);

    this._stackframesAndBreakpoints.setAttribute("width", Prefs.stackframesWidth);
    this._variables.setAttribute("width", Prefs.variablesWidth);

    this.showStackframesAndBreakpointsPane(Prefs.stackframesPaneVisible);
    this.showVariablesPane(Prefs.variablesPaneVisible);
  },

  





  initializeEditor: function DV_initializeEditor(aCallback) {
    let placeholder = document.getElementById("editor");

    let config = {
      mode: SourceEditor.MODES.JAVASCRIPT,
      showLineNumbers: true,
      readOnly: true,
      showAnnotationRuler: true,
      showOverviewRuler: true,
    };

    this.editor = new SourceEditor();
    this.editor.init(placeholder, config, function() {
      this._onEditorLoad();
      aCallback();
    }.bind(this));
  },

  


  destroyPanes: function DV_destroyPanes() {
    this._togglePanesButton.removeEventListener("click", this._onTogglePanesButtonPressed);

    Prefs.stackframesWidth = this._stackframesAndBreakpoints.getAttribute("width");
    Prefs.variablesWidth = this._variables.getAttribute("width");

    this._breakpoints.parentNode.removeChild(this._breakpoints);
    this._stackframes.parentNode.removeChild(this._stackframes);
    this._stackframesAndBreakpoints.parentNode.removeChild(this._stackframesAndBreakpoints);
    this._variables.parentNode.removeChild(this._variables);
    this._globalSearch.parentNode.removeChild(this._globalSearch);
  },

  


  destroyEditor: function DV_destroyEditor() {
    DebuggerController.Breakpoints.destroy();
    this.editor = null;
  },

  



  _onEditorLoad: function DV__onEditorLoad() {
    DebuggerController.Breakpoints.initialize();
    this.editor.focus();
  },

  


  _onTogglePanesButtonPressed: function DV__onTogglePanesButtonPressed() {
    this.showStackframesAndBreakpointsPane(
      this._togglePanesButton.getAttribute("stackframesAndBreakpointsHidden"), true);

    this.showVariablesPane(
      this._togglePanesButton.getAttribute("variablesHidden"), true);
  },

  



  showCloseButton: function DV_showCloseButton(aVisibleFlag) {
    document.getElementById("close").setAttribute("hidden", !aVisibleFlag);
  },

  




  showStackframesAndBreakpointsPane:
  function DV_showStackframesAndBreakpointsPane(aVisibleFlag, aAnimatedFlag) {
    if (aAnimatedFlag) {
      this._stackframesAndBreakpoints.setAttribute("animated", "");
    } else {
      this._stackframesAndBreakpoints.removeAttribute("animated");
    }
    if (aVisibleFlag) {
      this._stackframesAndBreakpoints.style.marginLeft = "0";
      this._togglePanesButton.removeAttribute("stackframesAndBreakpointsHidden");
    } else {
      let margin = parseInt(this._stackframesAndBreakpoints.getAttribute("width")) + 1;
      this._stackframesAndBreakpoints.style.marginLeft = -margin + "px";
      this._togglePanesButton.setAttribute("stackframesAndBreakpointsHidden", "true");
    }
    Prefs.stackframesPaneVisible = aVisibleFlag;
  },

  




  showVariablesPane:
  function DV_showVariablesPane(aVisibleFlag, aAnimatedFlag) {
    if (aAnimatedFlag) {
      this._variables.setAttribute("animated", "");
    } else {
      this._variables.removeAttribute("animated");
    }
    if (aVisibleFlag) {
      this._variables.style.marginRight = "0";
      this._togglePanesButton.removeAttribute("variablesHidden");
    } else {
      let margin = parseInt(this._variables.getAttribute("width")) + 1;
      this._variables.style.marginRight = -margin + "px";
      this._togglePanesButton.setAttribute("variablesHidden", "true");
    }
    Prefs.variablesPaneVisible = aVisibleFlag;
  },

  


  _togglePanesButton: null,
  _stackframesAndBreakpoints: null,
  _stackframes: null,
  _breakpoints: null,
  _variables: null,
  _globalSearch: null
};




function RemoteDebuggerPrompt() {

  


  this.remote = {};
}

RemoteDebuggerPrompt.prototype = {

  





  show: function RDP_show(aIsReconnectingFlag) {
    let check = { value: Prefs.remoteAutoConnect };
    let input = { value: Prefs.remoteHost + ":" + Prefs.remotePort };
    let parts;

    while (true) {
      let result = Services.prompt.prompt(null,
        L10N.getStr("remoteDebuggerPromptTitle"),
        L10N.getStr(aIsReconnectingFlag
          ? "remoteDebuggerReconnectMessage"
          : "remoteDebuggerPromptMessage"), input,
        L10N.getStr("remoteDebuggerPromptCheck"), check);

      Prefs.remoteAutoConnect = check.value;

      if (!result) {
        return false;
      }
      if ((parts = input.value.split(":")).length === 2) {
        let [host, port] = parts;

        if (host.length && port.length) {
          this.remote = { host: host, port: port };
          return true;
        }
      }
    }
  }
};




function GlobalSearchView() {
  this._onFetchScriptFinished = this._onFetchScriptFinished.bind(this);
  this._onFetchScriptsFinished = this._onFetchScriptsFinished.bind(this);
  this._onLineClick = this._onLineClick.bind(this);
  this._onMatchClick = this._onMatchClick.bind(this);
  this._onResultsScroll = this._onResultsScroll.bind(this);
  this._startSearch = this._startSearch.bind(this);
}

GlobalSearchView.prototype = {

  



  set hidden(value) {
    this._pane.hidden = value;
    this._splitter.hidden = value;
  },

  


  empty: function DVGS_empty() {
    while (this._pane.firstChild) {
      this._pane.removeChild(this._pane.firstChild);
    }
    this._pane.scrollTop = 0;
    this._pane.scrollLeft = 0;
    this._currentlyFocusedMatch = -1;
  },

  


  hideAndEmpty: function DVGS_hideAndEmpty() {
    this.hidden = true;
    this.empty();
    DebuggerController.dispatchEvent("Debugger:GlobalSearch:ViewCleared");
  },

  


  clearCache: function DVGS_clearCache() {
    this._scriptSources = new Map();
    DebuggerController.dispatchEvent("Debugger:GlobalSearch:CacheCleared");
  },

  










  fetchScripts:
  function DVGS_fetchScripts(aFetchCallback = null,
                             aFetchedCallback = null,
                             aUrls = DebuggerView.Scripts.scriptLocations) {

    
    if (this._scriptSources.size() === aUrls.length) {
      aFetchedCallback && aFetchedCallback();
      return;
    }

    
    for (let url of aUrls) {
      if (this._scriptSources.has(url)) {
        continue;
      }
      DebuggerController.dispatchEvent("Debugger:LoadSource", {
        url: url,
        options: {
          silent: true,
          callback: aFetchCallback
        }
      });
    }
  },

  


  scheduleSearch: function DVGS_scheduleSearch() {
    window.clearTimeout(this._searchTimeout);
    this._searchTimeout = window.setTimeout(this._startSearch, GLOBAL_SEARCH_ACTION_DELAY);
  },

  


  _startSearch: function DVGS__startSearch() {
    let scriptLocations = DebuggerView.Scripts.scriptLocations;
    this._scriptCount = scriptLocations.length;

    this.fetchScripts(
      this._onFetchScriptFinished, this._onFetchScriptsFinished, scriptLocations);
  },

  







  _onFetchScriptFinished: function DVGS__onFetchScriptFinished(aScriptUrl, aSourceText) {
    this._scriptSources.set(aScriptUrl, aSourceText);

    if (this._scriptSources.size() === this._scriptCount) {
      this._onFetchScriptsFinished();
    }
  },

  


  _onFetchScriptsFinished: function DVGS__onFetchScriptsFinished() {
    this.empty();

    let token = DebuggerView.Scripts.searchToken;
    let lowerCaseToken = token.toLowerCase();

    
    if (!token) {
      DebuggerController.dispatchEvent("Debugger:GlobalSearch:TokenEmpty");
      this.hidden = true;
      return;
    }

    
    let globalResults = new Map();

    for (let [url, text] of this._scriptSources) {
      
      if (text.toLowerCase().indexOf(lowerCaseToken) === -1) {
        continue;
      }
      let lines = text.split("\n");
      let scriptResults = {
        lineResults: [],
        matchCount: 0
      };

      for (let i = 0, len = lines.length; i < len; i++) {
        let line = lines[i];
        let lowerCaseLine = line.toLowerCase();

        
        if (lowerCaseLine.indexOf(lowerCaseToken) === -1) {
          continue;
        }

        let lineNumber = i;
        let lineContents = [];

        lowerCaseLine.split(lowerCaseToken).reduce(function(prev, curr, index, {length}) {
          let unmatched = line.substr(prev.length, curr.length);
          lineContents.push({ string: unmatched });

          if (index !== length - 1) {
            let matched = line.substr(prev.length + curr.length, token.length);
            let range = {
              start: prev.length + curr.length,
              length: matched.length
            };
            lineContents.push({
              string: matched,
              range: range,
              match: true
            });
            scriptResults.matchCount++;
          }
          return prev + token + curr;
        }, "");

        scriptResults.lineResults.push({
          lineNumber: lineNumber,
          lineContents: lineContents
        });
      }
      if (scriptResults.matchCount) {
        globalResults.set(url, scriptResults);
      }
    }

    if (globalResults.size()) {
      this._createGlobalResultsUI(globalResults);
      this.hidden = false;
      DebuggerController.dispatchEvent("Debugger:GlobalSearch:MatchFound");
    } else {
      this.hidden = true;
      DebuggerController.dispatchEvent("Debugger:GlobalSearch:MatchNotFound");
    }
  },

  





  _createGlobalResultsUI:
  function DVGS__createGlobalResultsUI(aGlobalResults) {
    let i = 0;

    for (let [scriptUrl, scriptResults] of aGlobalResults) {
      if (i++ === 0) {
        this._createScriptResultsUI(scriptUrl, scriptResults, true);
      } else {
        
        
        
        Services.tm.currentThread.dispatch({ run:
          this._createScriptResultsUI.bind(this, scriptUrl, scriptResults) }, 0);
      }
    }
  },

  









  _createScriptResultsUI:
  function DVGS__createScriptResultsUI(aScriptUrl, aScriptResults, aExpandFlag) {
    let { lineResults, matchCount } = aScriptResults;
    let element;

    for (let lineResult of lineResults) {
      element = this._createLineSearchResultsUI({
        scriptUrl: aScriptUrl,
        matchCount: matchCount,
        lineNumber: lineResult.lineNumber + 1,
        lineContents: lineResult.lineContents
      });
    }
    if (aExpandFlag) {
      element.expand(true);
    }
  },

  







  _createLineSearchResultsUI:
  function DVGS__createLineSearchresultsUI(aLineResults) {
    let scriptResultsId = "search-results-" + aLineResults.scriptUrl;
    let scriptResults = document.getElementById(scriptResultsId);

    
    if (!scriptResults) {
      let trimFunc = DebuggerController.SourceScripts.trimUrlLength;
      let urlLabel = trimFunc(aLineResults.scriptUrl, GLOBAL_SEARCH_URL_MAX_SIZE);

      let resultsUrl = document.createElement("label");
      resultsUrl.className = "plain script-url";
      resultsUrl.setAttribute("value", urlLabel);

      let resultsCount = document.createElement("label");
      resultsCount.className = "plain match-count";
      resultsCount.setAttribute("value", "(" + aLineResults.matchCount + ")");

      let arrow = document.createElement("box");
      arrow.className = "arrow";

      let resultsHeader = document.createElement("hbox");
      resultsHeader.className = "dbg-results-header";
      resultsHeader.setAttribute("align", "center")
      resultsHeader.appendChild(arrow);
      resultsHeader.appendChild(resultsUrl);
      resultsHeader.appendChild(resultsCount);

      let resultsContainer = document.createElement("vbox");
      resultsContainer.className = "dbg-results-container";

      scriptResults = document.createElement("vbox");
      scriptResults.id = scriptResultsId;
      scriptResults.className = "dbg-script-results";
      scriptResults.header = resultsHeader;
      scriptResults.container = resultsContainer;
      scriptResults.appendChild(resultsHeader);
      scriptResults.appendChild(resultsContainer);
      this._pane.appendChild(scriptResults);

      







      scriptResults.expand = function DVGS_element_expand(aSkipAnimationFlag) {
        resultsContainer.setAttribute("open", "");
        arrow.setAttribute("open", "");

        if (!aSkipAnimationFlag) {
          resultsContainer.setAttribute("animated", "");
        }
        return scriptResults;
      };

      




      scriptResults.collapse = function DVGS_element_collapse() {
        resultsContainer.removeAttribute("animated");
        resultsContainer.removeAttribute("open");
        arrow.removeAttribute("open");
        return scriptResults;
      };

      




      scriptResults.toggle = function DVGS_element_toggle(e) {
        if (e instanceof Event) {
          scriptResults._userToggle = true;
        }
        scriptResults.expanded = !scriptResults.expanded;
        return scriptResults;
      };

      




      Object.defineProperty(scriptResults, "expanded", {
        get: function DVP_element_getExpanded() {
          return arrow.hasAttribute("open");
        },
        set: function DVP_element_setExpanded(value) {
          if (value) {
            scriptResults.expand();
          } else {
            scriptResults.collapse();
          }
        }
      });

      


      resultsHeader.addEventListener("click", scriptResults.toggle, false);
    }

    let lineNumber = document.createElement("label");
    lineNumber.className = "plain line-number";
    lineNumber.setAttribute("value", aLineResults.lineNumber);

    let lineContents = document.createElement("hbox");
    lineContents.setAttribute("flex", "1");
    lineContents.className = "line-contents";
    lineContents.addEventListener("click", this._onLineClick, false);

    let lineContent;
    let totalLength = 0;
    let ellipsis = Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString);

    for (lineContent of aLineResults.lineContents) {
      let string = lineContent.string;
      let match = lineContent.match;

      string = string.substr(0, GLOBAL_SEARCH_LINE_MAX_SIZE - totalLength);
      totalLength += string.length;

      let label = document.createElement("label");
      label.className = "plain string";
      label.setAttribute("value", string);
      label.setAttribute("match", match || false);
      lineContents.appendChild(label);

      if (match) {
        label.addEventListener("click", this._onMatchClick, false);
        label.setUserData("lineResults", aLineResults, null);
        label.setUserData("lineContentRange", lineContent.range, null);
        label.container = scriptResults;
      }
      if (totalLength >= GLOBAL_SEARCH_LINE_MAX_SIZE) {
        label = document.createElement("label");
        label.className = "plain string";
        label.setAttribute("value", ellipsis.data);
        lineContents.appendChild(label);
        break;
      }
    }

    let searchResult = document.createElement("hbox");
    searchResult.className = "dbg-search-result";
    searchResult.appendChild(lineNumber);
    searchResult.appendChild(lineContents);

    let resultsContainer = scriptResults.container;
    resultsContainer.appendChild(searchResult);

    
    return scriptResults;
  },

  


  focusNextMatch: function DVGS_focusNextMatch() {
    let matches = this._pane.querySelectorAll(".string[match=true]");
    if (!matches.length) {
      return;
    }
    if (++this._currentlyFocusedMatch >= matches.length) {
      this._currentlyFocusedMatch = 0;
    }
    this._onMatchClick({ target: matches[this._currentlyFocusedMatch] });
  },

  


  _onLineClick: function DVGS__onLineClick(e) {
    let firstMatch = e.target.parentNode.querySelector(".string[match=true]");
    this._onMatchClick({ target: firstMatch });
  },

  


  _onMatchClick: function DVGLS__onMatchClick(e) {
    if (e instanceof Event) {
      e.preventDefault();
      e.stopPropagation();
    }
    let match = e.target;

    match.container.expand(true);
    this._scrollMatchIntoViewIfNeeded(match);
    this._animateMatchBounce(match);

    let results = match.getUserData("lineResults");
    let range = match.getUserData("lineContentRange");

    let stackframes = DebuggerController.StackFrames;
    stackframes.updateEditorToLocation(results.scriptUrl, results.lineNumber, 0, 0, 1);

    let editor = DebuggerView.editor;
    let offset = editor.getCaretOffset();
    editor.setSelection(offset + range.start, offset + range.start + range.length);
  },

  


  _onResultsScroll: function DVGS__onResultsScroll(e) {
    this._expandAllVisibleResults();
  },

  


  _expandAllVisibleResults: function DVGS__expandAllVisibleResults() {
    let collapsed = this._pane.querySelectorAll(".dbg-results-container:not([open])");

    for (let i = 0, l = collapsed.length; i < l; i++) {
      this._expandResultsIfNeeded(collapsed[i].parentNode);
    }
  },

  



  _expandResultsIfNeeded: function DVGS__expandResultsIfNeeded(aTarget) {
    if (aTarget.expanded || aTarget._userToggle) {
      return;
    }
    let { clientHeight } = this._pane;
    let { top, height } = aTarget.getBoundingClientRect();

    if (top - height <= clientHeight || this._forceExpandResults) {
      aTarget.expand(true);
    }
  },

  



  _scrollMatchIntoViewIfNeeded: function DVGS__scrollMatchIntoViewIfNeeded(aTarget) {
    let { clientHeight } = this._pane;
    let { top, height } = aTarget.getBoundingClientRect();

    let style = window.getComputedStyle(aTarget);
    let topBorderSize = window.parseInt(style.getPropertyValue("border-top-width"));
    let bottomBorderSize = window.parseInt(style.getPropertyValue("border-bottom-width"));

    let marginY = top - (height + topBorderSize + bottomBorderSize) * 2;
    if (marginY <= 0) {
      this._pane.scrollTop += marginY;
    }
    if (marginY + height > clientHeight) {
      this._pane.scrollTop += height - (clientHeight - marginY);
    }
  },

  



  _animateMatchBounce: function DVGS__animateMatchBounce(aTarget) {
    aTarget.setAttribute("focused", "");

    window.setTimeout(function() {
     aTarget.removeAttribute("focused");
    }, GLOBAL_SEARCH_MATCH_FLASH_DURATION);
  },

  


  _scriptSources: new Map(),

  


  _currentlyFocusedMatch: -1,

  


  _pane: null,
  _splitter: null,

  


  initialize: function DVS_initialize() {
    this._pane = document.getElementById("globalsearch");
    this._splitter = document.getElementById("globalsearch-splitter");

    this._pane.addEventListener("scroll", this._onResultsScroll, false);
  },

  


  destroy: function DVS_destroy() {
    this._pane.removeEventListener("scroll", this._onResultsScroll, false);

    this.hideAndEmpty();
    this._pane = null;
    this._splitter = null;
    this._scriptSources = null;
  }
};




function ScriptsView() {
  this._onScriptsChange = this._onScriptsChange.bind(this);
  this._onScriptsSearch = this._onScriptsSearch.bind(this);
  this._onScriptsKeyUp = this._onScriptsKeyUp.bind(this);
}

ScriptsView.prototype = {

  


  empty: function DVS_empty() {
    this._scripts.selectedIndex = -1;
    this._scripts.setAttribute("label", L10N.getStr("noScriptsText"));
    this._scripts.removeAttribute("tooltiptext");

    while (this._scripts.firstChild) {
      this._scripts.removeChild(this._scripts.firstChild);
    }
  },

  


  clearSearch: function DVS_clearSearch() {
    this._searchbox.value = "";
    this._onScriptsSearch({});
  },

  







  containsIgnoringQuery: function DVS_containsIgnoringQuery(aUrl) {
    let sourceScripts = DebuggerController.SourceScripts;
    aUrl = sourceScripts.trimUrlQuery(aUrl);

    if (this._tmpScripts.some(function(element) {
      return sourceScripts.trimUrlQuery(element.script.url) == aUrl;
    })) {
      return true;
    }
    if (this.scriptLocations.some(function(url) {
      return sourceScripts.trimUrlQuery(url) == aUrl;
    })) {
      return true;
    }
    return false;
  },

  







  contains: function DVS_contains(aUrl) {
    if (this._tmpScripts.some(function(element) {
      return element.script.url == aUrl;
    })) {
      return true;
    }
    if (this._scripts.getElementsByAttribute("value", aUrl).length > 0) {
      return true;
    }
    return false;
  },

  







  containsLabel: function DVS_containsLabel(aLabel) {
    if (this._tmpScripts.some(function(element) {
      return element.label == aLabel;
    })) {
      return true;
    }
    if (this._scripts.getElementsByAttribute("label", aLabel).length > 0) {
      return true;
    }
    return false;
  },

  





  selectIndex: function DVS_selectIndex(aIndex) {
    this._scripts.selectedIndex = aIndex;
  },

  





  selectScript: function DVS_selectScript(aUrl) {
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      if (this._scripts.getItemAtIndex(i).value == aUrl) {
        this._scripts.selectedIndex = i;
        return;
      }
    }
  },

  





  isSelected: function DVS_isSelected(aUrl) {
    if (this._scripts.selectedItem &&
        this._scripts.selectedItem.value == aUrl) {
      return true;
    }
    return false;
  },

  



  get selected() {
    return this._scripts.selectedItem ?
           this._scripts.selectedItem.value : null;
  },

  



  get preferredScriptUrl()
    this._preferredScriptUrl ? this._preferredScriptUrl : null,

  



  set preferredScriptUrl(value) this._preferredScriptUrl = value,

  







  getScriptByLabel: function DVS_getScriptByLabel(aLabel) {
    return this._scripts.getElementsByAttribute("label", aLabel)[0];
  },

  



  get scriptLabels() {
    let labels = [];
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      labels.push(this._scripts.getItemAtIndex(i).label);
    }
    return labels;
  },

  







  getScriptByLocation: function DVS_getScriptByLocation(aUrl) {
    return this._scripts.getElementsByAttribute("value", aUrl)[0];
  },

  



  get scriptLocations() {
    let locations = [];
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      locations.push(this._scripts.getItemAtIndex(i).value);
    }
    return locations;
  },

  



  get visibleItemsCount() {
    let count = 0;
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      count += this._scripts.getItemAtIndex(i).hidden ? 0 : 1;
    }
    return count;
  },

  
















  addScript: function DVS_addScript(aLabel, aScript, aForceFlag) {
    
    if (!aForceFlag) {
      this._tmpScripts.push({ label: aLabel, script: aScript });
      return;
    }

    
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      if (this._scripts.getItemAtIndex(i).label > aLabel) {
        this._createScriptElement(aLabel, aScript, i);
        return;
      }
    }
    
    this._createScriptElement(aLabel, aScript, -1);
  },

  



  commitScripts: function DVS_commitScripts() {
    let newScripts = this._tmpScripts;
    this._tmpScripts = [];

    if (!newScripts || !newScripts.length) {
      return;
    }
    newScripts.sort(function(a, b) {
      return a.label.toLowerCase() > b.label.toLowerCase();
    });

    for (let i = 0, l = newScripts.length; i < l; i++) {
      let item = newScripts[i];
      this._createScriptElement(item.label, item.script, -1);
    }
  },

  











  _createScriptElement: function DVS__createScriptElement(aLabel, aScript, aIndex)
  {
    
    if (aLabel == "null" || this.containsLabel(aLabel) || this.contains(aScript.url)) {
      return;
    }

    let scriptItem =
      aIndex == -1 ? this._scripts.appendItem(aLabel, aScript.url)
                   : this._scripts.insertItemAt(aIndex, aLabel, aScript.url);

    scriptItem.setAttribute("tooltiptext", aScript.url);
    scriptItem.setUserData("sourceScript", aScript, null);
  },

  





  get searchboxInfo() {
    let file, line, token, isGlobal;

    let rawValue = this._searchbox.value;
    let rawLength = rawValue.length;
    let lineFlagIndex = rawValue.lastIndexOf(SEARCH_LINE_FLAG);
    let tokenFlagIndex = rawValue.lastIndexOf(SEARCH_TOKEN_FLAG);
    let globalFlagIndex = rawValue.lastIndexOf(SEARCH_GLOBAL_FLAG);

    if (globalFlagIndex !== 0) {
      let fileEnd = lineFlagIndex !== -1 ? lineFlagIndex : tokenFlagIndex !== -1 ? tokenFlagIndex : rawLength;
      let lineEnd = tokenFlagIndex !== -1 ? tokenFlagIndex : rawLength;

      file = rawValue.slice(0, fileEnd);
      line = window.parseInt(rawValue.slice(fileEnd + 1, lineEnd)) || -1;
      token = rawValue.slice(lineEnd + 1);
      isGlobal = false;
    } else {
      file = "";
      line = -1;
      token = rawValue.slice(1);
      isGlobal = true;
    }

    return [file, line, token, isGlobal];
  },

  


  get searchToken() this.searchboxInfo[2],

  


  _onScriptsChange: function DVS__onScriptsChange() {
    let selectedItem = this._scripts.selectedItem;
    if (!selectedItem) {
      return;
    }

    this._preferredScript = selectedItem;
    this._preferredScriptUrl = selectedItem.value;
    this._scripts.setAttribute("tooltiptext", selectedItem.value);
    DebuggerController.SourceScripts.showScript(selectedItem.getUserData("sourceScript"));
  },

  





  _performFileSearch: function DVS__performFileSearch(aFile) {
    let scripts = this._scripts;

    
    scripts.selectedItem = this._preferredScript;
    scripts.setAttribute("label", this._preferredScript.label);
    scripts.setAttribute("tooltiptext", this._preferredScript.value);

    
    if (!aFile && this._someScriptsHidden) {
      this._someScriptsHidden = false;

      for (let i = 0, l = scripts.itemCount; i < l; i++) {
        scripts.getItemAtIndex(i).hidden = false;
      }
    } else if (this._prevSearchedFile !== aFile) {
      let lowerCaseFile = aFile.toLowerCase();
      let found = false;

      for (let i = 0, l = scripts.itemCount; i < l; i++) {
        let item = scripts.getItemAtIndex(i);
        let lowerCaseLabel = item.label.toLowerCase();

        
        if (lowerCaseLabel.match(aFile)) {
          item.hidden = false;

          if (!found) {
            found = true;
            scripts.selectedItem = item;
            scripts.setAttribute("label", item.label);
            scripts.setAttribute("tooltiptext", item.value);
          }
        }
        
        else {
          item.hidden = true;
          this._someScriptsHidden = true;
        }
      }
      if (!found) {
        scripts.setAttribute("label", L10N.getStr("noMatchingScriptsText"));
        scripts.removeAttribute("tooltiptext");
      }
    }
    this._prevSearchedFile = aFile;
  },

  





  _performLineSearch: function DVS__performLineSearch(aLine) {
    
    if (this._prevSearchedLine !== aLine && aLine > -1) {
      DebuggerView.editor.setCaretPosition(aLine - 1);
    }
    this._prevSearchedLine = aLine;
  },

  





  _performTokenSearch: function DVS__performTokenSearch(aToken) {
    
    if (this._prevSearchedToken !== aToken && aToken.length > 0) {
      let editor = DebuggerView.editor;
      let offset = editor.find(aToken, { ignoreCase: true });
      if (offset > -1) {
        editor.setSelection(offset, offset + aToken.length)
      }
    }
    this._prevSearchedToken = aToken;
  },

  


  _onScriptsSearch: function DVS__onScriptsSearch() {
    
    if (!this._scripts.itemCount) {
      return;
    }
    let [file, line, token, isGlobal] = this.searchboxInfo;

    
    
    if (isGlobal) {
      DebuggerView.GlobalSearch.scheduleSearch();
    } else {
      DebuggerView.GlobalSearch.hideAndEmpty();
      this._performFileSearch(file);
      this._performLineSearch(line);
      this._performTokenSearch(token);
    }
  },

  


  _onScriptsKeyUp: function DVS__onScriptsKeyUp(e) {
    if (e.keyCode === e.DOM_VK_ESCAPE) {
      DebuggerView.editor.focus();
      return;
    }

    if (e.keyCode === e.DOM_VK_RETURN || e.keyCode === e.DOM_VK_ENTER) {
      let token = this.searchboxInfo[2];
      let isGlobal = this.searchboxInfo[3];

      if (!token.length) {
        return;
      }
      if (isGlobal) {
        DebuggerView.GlobalSearch.focusNextMatch();
        return;
      }

      let editor = DebuggerView.editor;
      let offset = editor.findNext(true);
      if (offset > -1) {
        editor.setSelection(offset, offset + token.length)
      }
    }
  },

  


  _onSearch: function DVS__onSearch(aValue = "") {
    this._searchbox.focus();
    this._searchbox.value = aValue;
    DebuggerView.GlobalSearch.hideAndEmpty();
  },

  


  _onLineSearch: function DVS__onLineSearch() {
    this._onSearch(SEARCH_LINE_FLAG);
  },

  


  _onTokenSearch: function DVS__onTokenSearch() {
    this._onSearch(SEARCH_TOKEN_FLAG);
  },

  


  _onGlobalSearch: function DVS__onGlobalSearch() {
    this._onSearch(SEARCH_GLOBAL_FLAG);
  },

  


  _scripts: null,
  _searchbox: null,

  


  initialize: function DVS_initialize() {
    this._scripts = document.getElementById("scripts");
    this._searchbox = document.getElementById("scripts-search");
    this._scripts.addEventListener("select", this._onScriptsChange, false);
    this._searchbox.addEventListener("select", this._onScriptsSearch, false);
    this._searchbox.addEventListener("input", this._onScriptsSearch, false);
    this._searchbox.addEventListener("keyup", this._onScriptsKeyUp, false);
    this.commitScripts();
  },

  


  destroy: function DVS_destroy() {
    this._scripts.removeEventListener("select", this._onScriptsChange, false);
    this._searchbox.removeEventListener("select", this._onScriptsSearch, false);
    this._searchbox.removeEventListener("input", this._onScriptsSearch, false);
    this._searchbox.removeEventListener("keyup", this._onScriptsKeyUp, false);

    this.empty();
    this._scripts = null;
    this._searchbox = null;
  }
};




function StackFramesView() {
  this._onFramesScroll = this._onFramesScroll.bind(this);
  this._onPauseExceptionsClick = this._onPauseExceptionsClick.bind(this);
  this._onCloseButtonClick = this._onCloseButtonClick.bind(this);
  this._onResume = this._onResume.bind(this);
  this._onStepOver = this._onStepOver.bind(this);
  this._onStepIn = this._onStepIn.bind(this);
  this._onStepOut = this._onStepOut.bind(this);
}

StackFramesView.prototype = {

  





   updateState: function DVF_updateState(aState) {
     let resume = document.getElementById("resume");

     
     if (aState == "paused") {
       resume.setAttribute("tooltiptext", L10N.getStr("resumeTooltip"));
       resume.setAttribute("checked", true);
     }
     
     else if (aState == "attached") {
       resume.setAttribute("tooltiptext", L10N.getStr("pauseTooltip"));
       resume.removeAttribute("checked");
     }
   },

  


  empty: function DVF_empty() {
    while (this._frames.firstChild) {
      this._frames.removeChild(this._frames.firstChild);
    }
  },

  



  emptyText: function DVF_emptyText() {
    
    this.empty();

    let item = document.createElement("label");

    
    item.className = "list-item empty";
    item.setAttribute("value", L10N.getStr("emptyStackText"));

    this._frames.appendChild(item);
  },

  













  addFrame: function DVF_addFrame(aDepth, aFrameNameText, aFrameDetailsText) {
    
    if (document.getElementById("stackframe-" + aDepth)) {
      return null;
    }

    let frame = document.createElement("box");
    let frameName = document.createElement("label");
    let frameDetails = document.createElement("label");

    
    frame.id = "stackframe-" + aDepth;
    frame.className = "dbg-stackframe list-item";

    
    frameName.className = "dbg-stackframe-name plain";
    frameDetails.className = "dbg-stackframe-details plain";
    frameName.setAttribute("value", aFrameNameText);
    frameDetails.setAttribute("value", aFrameDetailsText);

    let spacer = document.createElement("spacer");
    spacer.setAttribute("flex", "1");

    frame.appendChild(frameName);
    frame.appendChild(spacer);
    frame.appendChild(frameDetails);

    this._frames.appendChild(frame);

    
    return frame;
  },

  







  highlightFrame: function DVF_highlightFrame(aDepth, aFlag) {
    let frame = document.getElementById("stackframe-" + aDepth);

    
    if (!frame) {
      return;
    }

    
    if (!aFlag && !frame.classList.contains("selected")) {
      frame.classList.add("selected");
    }
    
    else if (aFlag && frame.classList.contains("selected")) {
      frame.classList.remove("selected");
    }
  },

  





  unhighlightFrame: function DVF_unhighlightFrame(aDepth) {
    this.highlightFrame(aDepth, true);
  },

  





  get dirty() {
    return this._dirty;
  },

  





  set dirty(aValue) {
    this._dirty = aValue;
  },

  


  _onFramesClick: function DVF__onFramesClick(aEvent) {
    let target = aEvent.target;

    while (target) {
      if (target.debuggerFrame) {
        DebuggerController.StackFrames.selectFrame(target.debuggerFrame.depth);
        return;
      }
      target = target.parentNode;
    }
  },

  


  _onFramesScroll: function DVF__onFramesScroll(aEvent) {
    
    if (this._dirty) {
      let clientHeight = this._frames.clientHeight;
      let scrollTop = this._frames.scrollTop;
      let scrollHeight = this._frames.scrollHeight;

      
      
      if (scrollTop >= (scrollHeight - clientHeight) * 0.95) {
        this._dirty = false;

        DebuggerController.StackFrames.addMoreFrames();
      }
    }
  },

  


  _onCloseButtonClick: function DVF__onCloseButtonClick() {
    DebuggerController.dispatchEvent("Debugger:Close");
  },

  


  _onPauseExceptionsClick: function DVF__onPauseExceptionsClick() {
    let option = document.getElementById("pause-exceptions");
    DebuggerController.StackFrames.updatePauseOnExceptions(option.checked);
  },

  


  _onResume: function DVF__onResume(e) {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.resume();
    } else {
      DebuggerController.activeThread.interrupt();
    }
  },

  


  _onStepOver: function DVF__onStepOver(e) {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepOver();
    }
  },

  


  _onStepIn: function DVF__onStepIn(e) {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepIn();
    }
  },

  


  _onStepOut: function DVF__onStepOut(e) {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepOut();
    }
  },

  


  _dirty: false,

  


  _frames: null,

  


  initialize: function DVF_initialize() {
    let close = document.getElementById("close");
    let pauseOnExceptions = document.getElementById("pause-exceptions");
    let resume = document.getElementById("resume");
    let stepOver = document.getElementById("step-over");
    let stepIn = document.getElementById("step-in");
    let stepOut = document.getElementById("step-out");
    let frames = document.getElementById("stackframes");

    close.addEventListener("click", this._onCloseButtonClick, false);
    pauseOnExceptions.checked = DebuggerController.StackFrames.pauseOnExceptions;
    pauseOnExceptions.addEventListener("click", this._onPauseExceptionsClick, false);
    resume.addEventListener("click", this._onResume, false);
    stepOver.addEventListener("click", this._onStepOver, false);
    stepIn.addEventListener("click", this._onStepIn, false);
    stepOut.addEventListener("click", this._onStepOut, false);
    frames.addEventListener("click", this._onFramesClick, false);
    frames.addEventListener("scroll", this._onFramesScroll, false);
    window.addEventListener("resize", this._onFramesScroll, false);

    this._frames = frames;
    this.emptyText();
  },

  


  destroy: function DVF_destroy() {
    let close = document.getElementById("close");
    let pauseOnExceptions = document.getElementById("pause-exceptions");
    let resume = document.getElementById("resume");
    let stepOver = document.getElementById("step-over");
    let stepIn = document.getElementById("step-in");
    let stepOut = document.getElementById("step-out");
    let frames = this._frames;

    close.removeEventListener("click", this._onCloseButtonClick, false);
    pauseOnExceptions.removeEventListener("click", this._onPauseExceptionsClick, false);
    resume.removeEventListener("click", this._onResume, false);
    stepOver.removeEventListener("click", this._onStepOver, false);
    stepIn.removeEventListener("click", this._onStepIn, false);
    stepOut.removeEventListener("click", this._onStepOut, false);
    frames.removeEventListener("click", this._onFramesClick, false);
    frames.removeEventListener("scroll", this._onFramesScroll, false);
    window.removeEventListener("resize", this._onFramesScroll, false);

    this.empty();
    this._frames = null;
  }
};




function BreakpointsView() {
  this._onBreakpointClick = this._onBreakpointClick.bind(this);
  this._onBreakpointCheckboxChange = this._onBreakpointCheckboxChange.bind(this);
}

BreakpointsView.prototype = {

  


  empty: function DVB_empty() {
    let firstChild;

    while (firstChild = this._breakpoints.firstChild) {
      this._destroyContextMenu(firstChild);
      this._breakpoints.removeChild(firstChild);
    }
  },

  



  emptyText: function DVB_emptyText() {
    
    this.empty();

    let item = document.createElement("label");

    
    item.className = "list-item empty";
    item.setAttribute("value", L10N.getStr("emptyBreakpointsText"));

    this._breakpoints.appendChild(item);
  },

  











  getBreakpoint: function DVB_getBreakpoint(aUrl, aLine) {
    return this._breakpoints.getElementsByAttribute("location", aUrl + ":" + aLine)[0];
  },

  






  removeBreakpoint: function DVB_removeBreakpoint(aId) {
    let breakpoint = document.getElementById("breakpoint-" + aId);

    
    if (!breakpoint) {
      return;
    }
    this._destroyContextMenu(breakpoint);
    this._breakpoints.removeChild(breakpoint);

    if (!this.count) {
      this.emptyText();
    }
  },

  


















  addBreakpoint: function DVB_addBreakpoint(aId, aLineInfo, aLineText, aUrl, aLine) {
    
    if (document.getElementById("breakpoint-" + aId)) {
      return null;
    }
    
    if (!this.count) {
      this.empty();
    }

    
    let breakpoint = this.getBreakpoint(aUrl, aLine);
    if (breakpoint) {
      breakpoint.id = "breakpoint-" + aId;
      breakpoint.breakpointActor = aId;
      breakpoint.getElementsByTagName("checkbox")[0].setAttribute("checked", "true");
      return;
    }

    breakpoint = document.createElement("box");
    let bkpCheckbox = document.createElement("checkbox");
    let bkpLineInfo = document.createElement("label");
    let bkpLineText = document.createElement("label");

    
    breakpoint.id = "breakpoint-" + aId;
    breakpoint.className = "dbg-breakpoint list-item";
    breakpoint.setAttribute("location", aUrl + ":" + aLine);
    breakpoint.breakpointUrl = aUrl;
    breakpoint.breakpointLine = aLine;
    breakpoint.breakpointActor = aId;

    aLineInfo = aLineInfo.trim();
    aLineText = aLineText.trim();

    
    bkpCheckbox.setAttribute("checked", "true");
    bkpCheckbox.addEventListener("click", this._onBreakpointCheckboxChange, false);

    
    bkpLineInfo.className = "dbg-breakpoint-info plain";
    bkpLineText.className = "dbg-breakpoint-text plain";
    bkpLineInfo.setAttribute("value", aLineInfo);
    bkpLineText.setAttribute("value", aLineText);
    bkpLineInfo.setAttribute("crop", "end");
    bkpLineText.setAttribute("crop", "end");
    bkpLineText.setAttribute("tooltiptext", aLineText.substr(0, BREAKPOINT_LINE_TOOLTIP_MAX_SIZE));

    
    let menupopupId = this._createContextMenu(breakpoint);
    breakpoint.setAttribute("contextmenu", menupopupId);

    let state = document.createElement("vbox");
    state.className = "state";
    state.appendChild(bkpCheckbox);

    let content = document.createElement("vbox");
    content.className = "content";
    content.setAttribute("flex", "1");
    content.appendChild(bkpLineInfo);
    content.appendChild(bkpLineText);

    breakpoint.appendChild(state);
    breakpoint.appendChild(content);

    this._breakpoints.appendChild(breakpoint);

    
    return breakpoint;
  },

  











  enableBreakpoint:
  function DVB_enableBreakpoint(aTarget, aCallback, aNoCheckboxUpdate) {
    let { breakpointUrl: url, breakpointLine: line } = aTarget;
    let breakpoint = DebuggerController.Breakpoints.getBreakpoint(url, line)

    if (!breakpoint) {
      if (!aNoCheckboxUpdate) {
        aTarget.getElementsByTagName("checkbox")[0].setAttribute("checked", "true");
      }
      DebuggerController.Breakpoints.
        addBreakpoint({ url: url, line: line }, aCallback);

      return true;
    }
    return false;
  },

  











  disableBreakpoint:
  function DVB_disableBreakpoint(aTarget, aCallback, aNoCheckboxUpdate) {
    let { breakpointUrl: url, breakpointLine: line } = aTarget;
    let breakpoint = DebuggerController.Breakpoints.getBreakpoint(url, line)

    if (breakpoint) {
      if (!aNoCheckboxUpdate) {
        aTarget.getElementsByTagName("checkbox")[0].removeAttribute("checked");
      }
      DebuggerController.Breakpoints.
        removeBreakpoint(breakpoint, aCallback, false, true);

      return true;
    }
    return false;
  },

  


  get count() {
    return this._breakpoints.getElementsByClassName("dbg-breakpoint").length;
  },

  





  _iterate: function DVB_iterate(aCallback) {
    Array.forEach(Array.slice(this._breakpoints.childNodes), aCallback);
  },

  



  _getBreakpointTarget: function DVB__getBreakpointTarget(aEvent) {
    let target = aEvent.target;

    while (target) {
      if (target.breakpointActor) {
        return target;
      }
      target = target.parentNode;
    }
  },

  


  _onBreakpointClick: function DVB__onBreakpointClick(aEvent) {
    let target = this._getBreakpointTarget(aEvent);
    let { breakpointUrl: url, breakpointLine: line } = target;

    DebuggerController.StackFrames.updateEditorToLocation(url, line, 0, 0, 1);
  },

  


  _onBreakpointCheckboxChange: function DVB__onBreakpointCheckboxChange(aEvent) {
    aEvent.stopPropagation();

    let target = this._getBreakpointTarget(aEvent);
    let { breakpointUrl: url, breakpointLine: line } = target;

    if (aEvent.target.getAttribute("checked") === "true") {
      this.disableBreakpoint(target, null, true);
    } else {
      this.enableBreakpoint(target, null, true);
    }
  },

  





  _onEnableSelf: function DVB__onEnableSelf(aTarget) {
    if (!aTarget) {
      return;
    }
    if (this.enableBreakpoint(aTarget)) {
      aTarget.enableSelf.menuitem.setAttribute("hidden", "true");
      aTarget.disableSelf.menuitem.removeAttribute("hidden");
    }
  },

  





  _onDisableSelf: function DVB__onDisableSelf(aTarget) {
    if (!aTarget) {
      return;
    }
    if (this.disableBreakpoint(aTarget)) {
      aTarget.enableSelf.menuitem.removeAttribute("hidden");
      aTarget.disableSelf.menuitem.setAttribute("hidden", "true");
    }
  },

  





  _onDeleteSelf: function DVB__onDeleteSelf(aTarget) {
    let { breakpointUrl: url, breakpointLine: line } = aTarget;
    let breakpoint = DebuggerController.Breakpoints.getBreakpoint(url, line)

    if (aTarget) {
      this.removeBreakpoint(aTarget.breakpointActor);
    }
    if (breakpoint) {
      DebuggerController.Breakpoints.removeBreakpoint(breakpoint);
    }
  },

  





  _onEnableOthers: function DVB__onEnableOthers(aTarget) {
    this._iterate(function(element) {
      if (element !== aTarget) {
        this._onEnableSelf(element);
      }
    }.bind(this));
  },

  





  _onDisableOthers: function DVB__onDisableOthers(aTarget) {
    this._iterate(function(element) {
      if (element !== aTarget) {
        this._onDisableSelf(element);
      }
    }.bind(this));
  },

  





  _onDeleteOthers: function DVB__onDeleteOthers(aTarget) {
    this._iterate(function(element) {
      if (element !== aTarget) {
        this._onDeleteSelf(element);
      }
    }.bind(this));
  },

  





  _onEnableAll: function DVB__onEnableAll(aTarget) {
    this._onEnableOthers(aTarget);
    this._onEnableSelf(aTarget);
  },

  





  _onDisableAll: function DVB__onDisableAll(aTarget) {
    this._onDisableOthers(aTarget);
    this._onDisableSelf(aTarget);
  },

  





  _onDeleteAll: function DVB__onDeleteAll(aTarget) {
    this._onDeleteOthers(aTarget);
    this._onDeleteSelf(aTarget);
  },

  


  _breakpoints: null,

  







  _createContextMenu: function DVB_createContextMenu(aBreakpoint) {
    let commandsetId = "breakpointMenuCommands-" + aBreakpoint.id;
    let menupopupId = "breakpointContextMenu-" + aBreakpoint.id;

    let commandset = document.createElement("commandset");
    commandset.setAttribute("id", commandsetId);

    let menupopup = document.createElement("menupopup");
    menupopup.setAttribute("id", menupopupId);

    








    function createMenuItem(aName, aHiddenFlag) {
      let menuitem = document.createElement("menuitem");
      let command = document.createElement("command");

      let func = this["_on" + aName.charAt(0).toUpperCase() + aName.slice(1)];
      let label = L10N.getStr("breakpointMenuItem." + aName);

      let prefix = "bp-cMenu-";
      let commandId = prefix + aName + "-" + aBreakpoint.id + "-command";
      let menuitemId = prefix + aName + "-" + aBreakpoint.id + "-menuitem";

      command.setAttribute("id", commandId);
      command.setAttribute("label", label);
      command.addEventListener("command", func.bind(this, aBreakpoint), true);

      menuitem.setAttribute("id", menuitemId);
      menuitem.setAttribute("command", commandId);
      menuitem.setAttribute("hidden", aHiddenFlag);

      commandset.appendChild(command);
      menupopup.appendChild(menuitem);

      aBreakpoint[aName] = {
        menuitem: menuitem,
        command: command
      };
    }

    



    function createMenuSeparator() {
      let menuseparator = document.createElement("menuseparator");
      menupopup.appendChild(menuseparator);
    }

    createMenuItem.call(this, "enableSelf", true);
    createMenuItem.call(this, "disableSelf");
    createMenuItem.call(this, "deleteSelf");
    createMenuSeparator();
    createMenuItem.call(this, "enableOthers");
    createMenuItem.call(this, "disableOthers");
    createMenuItem.call(this, "deleteOthers");
    createMenuSeparator();
    createMenuItem.call(this, "enableAll");
    createMenuItem.call(this, "disableAll");
    createMenuSeparator();
    createMenuItem.call(this, "deleteAll");

    let popupset = document.getElementById("debugger-popups");
    popupset.appendChild(menupopup);
    document.documentElement.appendChild(commandset);

    aBreakpoint.commandsetId = commandsetId;
    aBreakpoint.menupopupId = menupopupId;

    return menupopupId;
  },

  





  _destroyContextMenu: function DVB__destroyContextMenu(aBreakpoint) {
    if (!aBreakpoint.commandsetId || !aBreakpoint.menupopupId) {
      return;
    }

    let commandset = document.getElementById(aBreakpoint.commandsetId);
    let menupopup = document.getElementById(aBreakpoint.menupopupId);

    commandset.parentNode.removeChild(commandset);
    menupopup.parentNode.removeChild(menupopup);
  },

  


  initialize: function DVB_initialize() {
    let breakpoints = document.getElementById("breakpoints");
    breakpoints.addEventListener("click", this._onBreakpointClick, false);

    this._breakpoints = breakpoints;
    this.emptyText();
  },

  


  destroy: function DVB_destroy() {
    let breakpoints = this._breakpoints;
    breakpoints.removeEventListener("click", this._onBreakpointClick, false);

    this.empty();
    this._breakpoints = null;
  }
};




function PropertiesView() {
  this.addScope = this._addScope.bind(this);
  this._addVar = this._addVar.bind(this);
  this._addProperties = this._addProperties.bind(this);
}

PropertiesView.prototype = {

  



  _idCount: 1,

  












  _addScope: function DVP__addScope(aName, aId) {
    
    if (!this._vars) {
      return null;
    }

    
    aId = aId || aName.toLowerCase().trim().replace(/\s+/g, "-") + this._idCount++;

    
    let element = this._createPropertyElement(aName, aId, "scope", this._vars);

    
    if (!element) {
      return null;
    }
    element._identifier = aName;

    


    element.addVar = this._addVar.bind(this, element);

    


    element.addToHierarchy = this.addScopeToHierarchy.bind(this, element);

    
    element.refresh(function() {
      let title = element.getElementsByClassName("title")[0];
      title.classList.add("devtools-toolbar");
    }.bind(this));

    
    return element;
  },

  


  empty: function DVP_empty() {
    while (this._vars.firstChild) {
      this._vars.removeChild(this._vars.firstChild);
    }
  },

  



  emptyText: function DVP_emptyText() {
    
    this.empty();

    let item = document.createElement("label");

    
    item.className = "list-item empty";
    item.setAttribute("value", L10N.getStr("emptyVariablesText"));

    this._vars.appendChild(item);
  },

  















  _addVar: function DVP__addVar(aScope, aName, aFlags, aId) {
    
    if (!aScope) {
      return null;
    }

    
    aId = aId || (aScope.id + "->" + aName + "-variable");

    
    let element = this._createPropertyElement(aName, aId, "variable",
                                              aScope.getElementsByClassName("details")[0]);

    
    if (!element) {
      return null;
    }
    element._identifier = aName;

    


    element.setGrip = this._setGrip.bind(this, element);

    


    element.addProperties = this._addProperties.bind(this, element);

    
    element.refresh(function() {
      let separatorLabel = document.createElement("label");
      let valueLabel = document.createElement("label");
      let title = element.getElementsByClassName("title")[0];

      
      this._setAttributes(element, aName, aFlags);

      
      separatorLabel.className = "plain";
      separatorLabel.setAttribute("value", ":");

      
      valueLabel.className = "value plain";

      
      valueLabel.addEventListener("click", this._activateElementInputMode.bind({
        scope: this,
        element: element,
        valueLabel: valueLabel
      }));

      
      Object.defineProperty(element, "token", {
        value: aName,
        writable: false,
        enumerable: true,
        configurable: true
      });

      title.appendChild(separatorLabel);
      title.appendChild(valueLabel);

      
      this._saveHierarchy({
        parent: aScope,
        element: element,
        valueLabel: valueLabel
      });
    }.bind(this));

    
    return element;
  },

  









  _setAttributes: function DVP_setAttributes(aVar, aName, aFlags) {
    if (aFlags) {
      if (!aFlags.configurable) {
        aVar.setAttribute("non-configurable", "");
      }
      if (!aFlags.enumerable) {
        aVar.setAttribute("non-enumerable", "");
      }
      if (!aFlags.writable) {
        aVar.setAttribute("non-writable", "");
      }
    }
    if (aName === "this") {
      aVar.setAttribute("self", "");
    }
    if (aName === "__proto__ ") {
      aVar.setAttribute("proto", "");
    }
  },

  




















  _setGrip: function DVP__setGrip(aVar, aGrip) {
    
    if (!aVar) {
      return null;
    }
    if (aGrip === undefined) {
      aGrip = { type: "undefined" };
    }
    if (aGrip === null) {
      aGrip = { type: "null" };
    }

    let valueLabel = aVar.getElementsByClassName("value")[0];

    
    if (!valueLabel) {
      return null;
    }

    this._applyGrip(valueLabel, aGrip);
    return aVar;
  },

  








  _applyGrip: function DVP__applyGrip(aValueLabel, aGrip) {
    let prevGrip = aValueLabel.currentGrip;
    if (prevGrip) {
      aValueLabel.classList.remove(this._propertyColor(prevGrip));
    }

    aValueLabel.setAttribute("value", this._propertyString(aGrip));
    aValueLabel.classList.add(this._propertyColor(aGrip));
    aValueLabel.currentGrip = aGrip;
  },

  





















  _addProperties: function DVP__addProperties(aVar, aProperties) {
    
    for (let i in aProperties) {
      
      if (Object.getOwnPropertyDescriptor(aProperties, i)) {

        
        let desc = aProperties[i];

        
        
        let value = desc["value"];

        
        
        let getter = desc["get"];
        let setter = desc["set"];

        
        if (value !== undefined) {
          this._addProperty(aVar, [i, value], desc);
        }
        if (getter !== undefined || setter !== undefined) {
          let prop = this._addProperty(aVar, [i]).expand();
          prop.getter = this._addProperty(prop, ["get", getter], desc);
          prop.setter = this._addProperty(prop, ["set", setter], desc);
        }
      }
    }
    return aVar;
  },

  

























  _addProperty: function DVP__addProperty(aVar, aProperty, aFlags, aName, aId) {
    
    if (!aVar) {
      return null;
    }

    
    aId = aId || (aVar.id + "->" + aProperty[0] + "-property");

    
    let element = this._createPropertyElement(aName, aId, "property",
                                              aVar.getElementsByClassName("details")[0]);

    
    if (!element) {
      return null;
    }
    element._identifier = aName;

    


    element.setGrip = this._setGrip.bind(this, element);

    


    element.addProperties = this._addProperties.bind(this, element);

    
    element.refresh(function(pKey, pGrip) {
      let title = element.getElementsByClassName("title")[0];
      let nameLabel = title.getElementsByClassName("name")[0];
      let separatorLabel = document.createElement("label");
      let valueLabel = document.createElement("label");

      
      this._setAttributes(element, pKey, aFlags);

      if ("undefined" !== typeof pKey) {
        
        nameLabel.className = "key plain";
        nameLabel.setAttribute("value", pKey.trim());
        title.appendChild(nameLabel);
      }
      if ("undefined" !== typeof pGrip) {
        
        separatorLabel.className = "plain";
        separatorLabel.setAttribute("value", ":");

        
        valueLabel.className = "value plain";
        this._applyGrip(valueLabel, pGrip);

        title.appendChild(separatorLabel);
        title.appendChild(valueLabel);
      }

      
      valueLabel.addEventListener("click", this._activateElementInputMode.bind({
        scope: this,
        element: element,
        valueLabel: valueLabel
      }));

      
      Object.defineProperty(element, "token", {
        value: aVar.token + "['" + pKey + "']",
        writable: false,
        enumerable: true,
        configurable: true
      });

      
      this._saveHierarchy({
        parent: aVar,
        element: element,
        valueLabel: valueLabel
      });

      
      Object.defineProperty(aVar, pKey, { value: element,
                                          writable: false,
                                          enumerable: true,
                                          configurable: true });
    }.bind(this), aProperty);

    
    return element;
  },

  











  _activateElementInputMode: function DVP__activateElementInputMode(aEvent) {
    if (aEvent) {
      aEvent.stopPropagation();
    }

    let self = this.scope;
    let element = this.element;
    let valueLabel = this.valueLabel;
    let titleNode = valueLabel.parentNode;
    let initialValue = valueLabel.getAttribute("value");

    
    
    element._previouslyExpanded = element.expanded;
    element._preventExpand = true;
    element.collapse();
    element.forceHideArrow();

    
    
    let textbox = document.createElement("textbox");
    textbox.setAttribute("value", initialValue);
    textbox.className = "element-input";
    textbox.width = valueLabel.clientWidth + 1;

    
    function DVP_element_textbox_blur(aTextboxEvent) {
      DVP_element_textbox_save();
    }

    function DVP_element_textbox_keyup(aTextboxEvent) {
      if (aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_LEFT ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_RIGHT ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_UP ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_DOWN) {
        return;
      }
      if (aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_RETURN ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_ENTER) {
        DVP_element_textbox_save();
        return;
      }
      if (aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_ESCAPE) {
        valueLabel.setAttribute("value", initialValue);
        DVP_element_textbox_clear();
        return;
      }
    }

    
    function DVP_element_textbox_save() {
      if (textbox.value !== valueLabel.getAttribute("value")) {
        valueLabel.setAttribute("value", textbox.value);

        let expr = "(" + element.token + "=" + textbox.value + ")";
        DebuggerController.StackFrames.evaluate(expr);
      }
      DVP_element_textbox_clear();
    }

    
    function DVP_element_textbox_clear() {
      element._preventExpand = false;
      if (element._previouslyExpanded) {
        element._previouslyExpanded = false;
        element.expand();
      }
      element.showArrow();

      textbox.removeEventListener("blur", DVP_element_textbox_blur, false);
      textbox.removeEventListener("keyup", DVP_element_textbox_keyup, false);
      titleNode.removeChild(textbox);
      titleNode.appendChild(valueLabel);
    }

    textbox.addEventListener("blur", DVP_element_textbox_blur, false);
    textbox.addEventListener("keyup", DVP_element_textbox_keyup, false);
    titleNode.removeChild(valueLabel);
    titleNode.appendChild(textbox);

    textbox.select();

    
    
    
    if (valueLabel.getAttribute("value").match(/^"[^"]*"$/)) {
      textbox.selectionEnd--;
      textbox.selectionStart++;
    }
  },

  







  _propertyString: function DVP__propertyString(aGrip) {
    if (aGrip && "object" === typeof aGrip) {
      switch (aGrip.type) {
        case "undefined":
          return "undefined";
        case "null":
          return "null";
        default:
          return "[" + aGrip.type + " " + aGrip.class + "]";
      }
    } else {
      switch (typeof aGrip) {
        case "string":
          return "\"" + aGrip + "\"";
        case "boolean":
          return aGrip ? "true" : "false";
        default:
          return aGrip + "";
      }
    }
    return aGrip + "";
  },

  








  _propertyColor: function DVP__propertyColor(aGrip) {
    if (aGrip && "object" === typeof aGrip) {
      switch (aGrip.type) {
        case "undefined":
          return "token-undefined";
        case "null":
          return "token-null";
      }
    } else {
      switch (typeof aGrip) {
        case "string":
          return "token-string";
        case "boolean":
          return "token-boolean";
        case "number":
          return "token-number";
      }
    }
    return "token-other";
  },

  
















  _createPropertyElement: function DVP__createPropertyElement(aName, aId, aClass, aParent) {
    
    if (document.getElementById(aId)) {
      return null;
    }
    if (!aParent) {
      return null;
    }

    let element = document.createElement("vbox");
    let arrow = document.createElement("box");
    let name = document.createElement("label");

    let title = document.createElement("box");
    let details = document.createElement("vbox");

    
    element.id = aId;
    element.className = aClass;

    
    arrow.className = "arrow";
    arrow.style.visibility = "hidden";

    
    name.className = "name plain";
    name.setAttribute("value", aName || "");

    
    title.className = "title";
    title.setAttribute("align", "center")

    
    details.className = "details";

    
    if (aClass === "scope") {
      title.addEventListener("click", function() element.toggle(), false);
    } else {
      arrow.addEventListener("click", function() element.toggle(), false);
      name.addEventListener("click", function() element.toggle(), false);
      name.addEventListener("mouseover", function() element.updateTooltip(name), false);
    }

    title.appendChild(arrow);
    title.appendChild(name);

    element.appendChild(title);
    element.appendChild(details);

    aParent.appendChild(element);

    




    element.show = function DVP_element_show() {
      element.style.display = "-moz-box";

      if ("function" === typeof element.onshow) {
        element.onshow(element);
      }
      return element;
    };

    




    element.hide = function DVP_element_hide() {
      element.style.display = "none";

      if ("function" === typeof element.onhide) {
        element.onhide(element);
      }
      return element;
    };

    







    element.expand = function DVP_element_expand(aSkipAnimationFlag) {
      if (element._preventExpand) {
        return;
      }
      arrow.setAttribute("open", "");
      details.setAttribute("open", "");

      if (!aSkipAnimationFlag) {
        details.setAttribute("animated", "");
      }
      if ("function" === typeof element.onexpand) {
        element.onexpand(element);
      }
      return element;
    };

    




    element.collapse = function DVP_element_collapse() {
      if (element._preventCollapse) {
        return;
      }
      arrow.removeAttribute("open");
      details.removeAttribute("open");
      details.removeAttribute("animated");

      if ("function" === typeof element.oncollapse) {
        element.oncollapse(element);
      }
      return element;
    };

    




    element.toggle = function DVP_element_toggle() {
      element.expanded = !element.expanded;

      if ("function" === typeof element.ontoggle) {
        element.ontoggle(element);
      }
      return element;
    };

    




    element.showArrow = function DVP_element_showArrow() {
      if (element._forceShowArrow || details.childNodes.length) {
        arrow.style.visibility = "visible";
      }
      return element;
    };

    




    element.hideArrow = function DVP_element_hideArrow() {
      if (!element._forceShowArrow) {
        arrow.style.visibility = "hidden";
      }
      return element;
    };

    






    element.forceShowArrow = function DVP_element_forceShowArrow() {
      element._forceShowArrow = true;
      arrow.style.visibility = "visible";
      return element;
    };

    






    element.forceHideArrow = function DVP_element_forceHideArrow() {
      arrow.style.visibility = "hidden";
      return element;
    };

    




    Object.defineProperty(element, "visible", {
      get: function DVP_element_getVisible() {
        return element.style.display !== "none";
      },
      set: function DVP_element_setVisible(value) {
        if (value) {
          element.show();
        } else {
          element.hide();
        }
      }
    });

    




    Object.defineProperty(element, "expanded", {
      get: function DVP_element_getExpanded() {
        return arrow.hasAttribute("open");
      },
      set: function DVP_element_setExpanded(value) {
        if (value) {
          element.expand();
        } else {
          element.collapse();
        }
      }
    });

    




    element.empty = function DVP_element_empty() {
      
      arrow.style.visibility = "hidden";
      while (details.firstChild) {
        details.removeChild(details.firstChild);
      }

      if ("function" === typeof element.onempty) {
        element.onempty(element);
      }
      return element;
    };

    




    element.remove = function DVP_element_remove() {
      element.parentNode.removeChild(element);

      if ("function" === typeof element.onremove) {
        element.onremove(element);
      }
      return element;
    };

    




    Object.defineProperty(element, "arrowVisible", {
      get: function DVP_element_getArrowVisible() {
        return arrow.style.visibility !== "hidden";
      },
      set: function DVP_element_setExpanded(value) {
        if (value) {
          element.showArrow();
        } else {
          element.hideArrow();
        }
      }
    });

    





    element.updateTooltip = function DVP_element_updateTooltip(aAnchor) {
      let tooltip = document.getElementById("element-tooltip");
      if (tooltip) {
        document.documentElement.removeChild(tooltip);
      }

      tooltip = document.createElement("tooltip");
      tooltip.id = "element-tooltip";

      let configurableLabel = document.createElement("label");
      configurableLabel.id = "configurableLabel";
      configurableLabel.setAttribute("value", "configurable");

      let enumerableLabel = document.createElement("label");
      enumerableLabel.id = "enumerableLabel";
      enumerableLabel.setAttribute("value", "enumerable");

      let writableLabel = document.createElement("label");
      writableLabel.id = "writableLabel";
      writableLabel.setAttribute("value", "writable");

      tooltip.setAttribute("orient", "horizontal")
      tooltip.appendChild(configurableLabel);
      tooltip.appendChild(enumerableLabel);
      tooltip.appendChild(writableLabel);

      if (element.hasAttribute("non-configurable")) {
        configurableLabel.setAttribute("non-configurable", "");
      }
      if (element.hasAttribute("non-enumerable")) {
        enumerableLabel.setAttribute("non-enumerable", "");
      }
      if (element.hasAttribute("non-writable")) {
        writableLabel.setAttribute("non-writable", "");
      }

      document.documentElement.appendChild(tooltip);
      aAnchor.setAttribute("tooltip", tooltip.id);
    };

    








    element.refresh = function DVP_element_refresh(aFunction, aArguments) {
      if ("function" === typeof aFunction) {
        aFunction.apply(this, aArguments);
      }

      let node = aParent.parentNode;
      let arrow = node.getElementsByClassName("arrow")[0];
      let children = node.getElementsByClassName("details")[0].childNodes.length;

      
      
      if (children) {
        arrow.style.visibility = "visible";
      } else {
        arrow.style.visibility = "hidden";
      }
    }.bind(this);

    
    return element;
  },

  





  _saveHierarchy: function DVP__saveHierarchy(aProperties) {
    let parent = aProperties.parent;
    let element = aProperties.element;
    let valueLabel = aProperties.valueLabel;
    let store = aProperties.store || parent._children;

    
    if (!element || !store) {
      return;
    }

    let relation = {
      root: parent ? (parent._root || parent) : null,
      parent: parent || null,
      element: element,
      valueLabel: valueLabel,
      children: {}
    };

    store[element._identifier] = relation;
    element._root = relation.root;
    element._children = relation.children;
  },

  



  createHierarchyStore: function DVP_createHierarchyStore() {
    this._prevHierarchy = this._currHierarchy;
    this._currHierarchy = {};
  },

  





  addScopeToHierarchy: function DVP_addScopeToHierarchy(aScope) {
    this._saveHierarchy({ element: aScope, store: this._currHierarchy });
  },

  


  commitHierarchy: function DVS_commitHierarchy() {
    for (let i in this._currHierarchy) {
      let currScope = this._currHierarchy[i];
      let prevScope = this._prevHierarchy[i];

      if (!prevScope) {
        continue;
      }

      for (let v in currScope.children) {
        let currVar = currScope.children[v];
        let prevVar = prevScope.children[v];

        let action = "";

        if (prevVar) {
          let prevValue = prevVar.valueLabel.getAttribute("value");
          let currValue = currVar.valueLabel.getAttribute("value");

          if (currValue != prevValue) {
            action = "changed";
          } else {
            action = "unchanged";
          }
        } else {
          action = "added";
        }

        if (action) {
          currVar.element.setAttribute(action, "");

          window.setTimeout(function() {
           currVar.element.removeAttribute(action);
          }, PROPERTY_VIEW_FLASH_DURATION);
        }
      }
    }
  },

  



  _currHierarchy: null,
  _prevHierarchy: null,

  


  _vars: null,

  


  initialize: function DVP_initialize() {
    this._vars = document.getElementById("variables");

    this.emptyText();
    this.createHierarchyStore();
  },

  


  destroy: function DVP_destroy() {
    this.empty();

    this._currHierarchy = null;
    this._prevHierarchy = null;
    this._vars = null;
  }
};




DebuggerView.GlobalSearch = new GlobalSearchView();
DebuggerView.Scripts = new ScriptsView();
DebuggerView.StackFrames = new StackFramesView();
DebuggerView.Breakpoints = new BreakpointsView();
DebuggerView.Properties = new PropertiesView();




Object.defineProperty(window, "editor", {
  get: function() { return DebuggerView.editor; }
});
