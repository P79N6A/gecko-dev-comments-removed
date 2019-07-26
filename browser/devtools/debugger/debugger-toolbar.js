




"use strict";





function ToolbarView() {
  dumpn("ToolbarView was instantiated");
  this._onCloseClick = this._onCloseClick.bind(this);
  this._onTogglePanesPressed = this._onTogglePanesPressed.bind(this);
  this._onResumePressed = this._onResumePressed.bind(this);
  this._onStepOverPressed = this._onStepOverPressed.bind(this);
  this._onStepInPressed = this._onStepInPressed.bind(this);
  this._onStepOutPressed = this._onStepOutPressed.bind(this);
}

ToolbarView.prototype = {
  


  initialize: function DVT_initialize() {
    dumpn("Initializing the ToolbarView");
    this._closeButton = document.getElementById("close");
    this._togglePanesButton = document.getElementById("toggle-panes");
    this._resumeButton = document.getElementById("resume");
    this._stepOverButton = document.getElementById("step-over");
    this._stepInButton = document.getElementById("step-in");
    this._stepOutButton = document.getElementById("step-out");
    this._chromeGlobals = document.getElementById("chrome-globals");
    this._scripts = document.getElementById("sources");

    let resumeKey = LayoutHelpers.prettyKey(document.getElementById("resumeKey"));
    let stepOverKey = LayoutHelpers.prettyKey(document.getElementById("stepOverKey"));
    let stepInKey = LayoutHelpers.prettyKey(document.getElementById("stepInKey"));
    let stepOutKey = LayoutHelpers.prettyKey(document.getElementById("stepOutKey"));
    this._resumeTooltip = L10N.getFormatStr("resumeButtonTooltip", [resumeKey]);
    this._pauseTooltip = L10N.getFormatStr("pauseButtonTooltip", [resumeKey]);
    this._stepOverTooltip = L10N.getFormatStr("stepOverTooltip", [stepOverKey]);
    this._stepInTooltip = L10N.getFormatStr("stepInTooltip", [stepInKey]);
    this._stepOutTooltip = L10N.getFormatStr("stepOutTooltip", [stepOutKey]);

    this._closeButton.addEventListener("click", this._onCloseClick, false);
    this._togglePanesButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.addEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.addEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.addEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.addEventListener("mousedown", this._onStepOutPressed, false);

    this._stepOverButton.setAttribute("tooltiptext", this._stepOverTooltip);
    this._stepInButton.setAttribute("tooltiptext", this._stepInTooltip);
    this._stepOutButton.setAttribute("tooltiptext", this._stepOutTooltip);

    this.toggleCloseButton(!window._isRemoteDebugger && !window._isChromeDebugger);
    this.toggleChromeGlobalsContainer(window._isChromeDebugger);
  },

  


  destroy: function DVT_destroy() {
    dumpn("Destroying the ToolbarView");
    this._closeButton.removeEventListener("click", this._onCloseClick, false);
    this._togglePanesButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.removeEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.removeEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.removeEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.removeEventListener("mousedown", this._onStepOutPressed, false);
  },

  





  toggleCloseButton: function DVT_toggleCloseButton(aVisibleFlag) {
    this._closeButton.setAttribute("hidden", !aVisibleFlag);
  },

  





  toggleResumeButtonState: function DVT_toggleResumeButtonState(aState) {
    
    if (aState == "paused") {
      this._resumeButton.setAttribute("checked", "true");
      this._resumeButton.setAttribute("tooltiptext", this._resumeTooltip);
    }
    
    else if (aState == "attached") {
      this._resumeButton.removeAttribute("checked");
      this._resumeButton.setAttribute("tooltiptext", this._pauseTooltip);
    }
  },

  





  toggleChromeGlobalsContainer: function DVT_toggleChromeGlobalsContainer(aVisibleFlag) {
    this._chromeGlobals.setAttribute("hidden", !aVisibleFlag);
  },

  





  toggleSourcesContainer: function DVT_toggleSourcesContainer(aVisibleFlag) {
    this._sources.setAttribute("hidden", !aVisibleFlag);
  },

  


  _onCloseClick: function DVT__onCloseClick() {
    DebuggerController._shutdownDebugger();
  },

  


  _onTogglePanesPressed: function DVT__onTogglePanesPressed() {
    DebuggerView.togglePanes({
      visible: DebuggerView.panesHidden,
      animated: true
    });
  },

  


  _onResumePressed: function DVT__onResumePressed() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.resume();
    } else {
      DebuggerController.activeThread.interrupt();
    }
  },

  


  _onStepOverPressed: function DVT__onStepOverPressed() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepOver();
    }
  },

  


  _onStepInPressed: function DVT__onStepInPressed() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepIn();
    }
  },

  


  _onStepOutPressed: function DVT__onStepOutPressed() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepOut();
    }
  },

  _closeButton: null,
  _togglePanesButton: null,
  _resumeButton: null,
  _stepOverButton: null,
  _stepInButton: null,
  _stepOutButton: null,
  _chromeGlobals: null,
  _sources: null,
  _resumeTooltip: "",
  _pauseTooltip: "",
  _stepOverTooltip: "",
  _stepInTooltip: "",
  _stepOutTooltip: ""
};




function OptionsView() {
  dumpn("OptionsView was instantiated");
  this._togglePauseOnExceptions = this._togglePauseOnExceptions.bind(this);
  this._toggleShowPanesOnStartup = this._toggleShowPanesOnStartup.bind(this);
  this._toggleShowNonEnum = this._toggleShowNonEnum.bind(this);
}

OptionsView.prototype = {
  


  initialize: function DVO_initialize() {
    dumpn("Initializing the OptionsView");
    this._button = document.getElementById("debugger-options");
    this._pauseOnExceptionsItem = document.getElementById("pause-on-exceptions");
    this._showPanesOnStartupItem = document.getElementById("show-panes-on-startup");
    this._showNonEnumItem = document.getElementById("show-nonenum");

    this._pauseOnExceptionsItem.setAttribute("checked", "false");
    this._showPanesOnStartupItem.setAttribute("checked", Prefs.panesVisibleOnStartup);
    this._showNonEnumItem.setAttribute("checked", Prefs.nonEnumVisible);
  },

  


  destroy: function DVO_destroy() {
    dumpn("Destroying the OptionsView");
    
  },

  


  _onPopupShowing: function DVO__onPopupShowing() {
    this._button.setAttribute("open", "true");
  },

  


  _onPopupHiding: function DVO__onPopupHiding() {
    this._button.removeAttribute("open");
  },

  


  _togglePauseOnExceptions: function DVO__togglePauseOnExceptions() {
    DebuggerController.activeThread.pauseOnExceptions(
      this._pauseOnExceptionsItem.getAttribute("checked") == "true");
  },

  


  _toggleShowPanesOnStartup: function DVO__toggleShowPanesOnStartup() {
    Prefs.panesVisibleOnStartup =
      this._showPanesOnStartupItem.getAttribute("checked") == "true";
  },

  


  _toggleShowNonEnum: function DVO__toggleShowNonEnum() {
    DebuggerView.Variables.nonEnumVisible = Prefs.nonEnumVisible =
      this._showNonEnumItem.getAttribute("checked") == "true";
  },

  _button: null,
  _pauseOnExceptionsItem: null,
  _showPanesOnStartupItem: null,
  _showNonEnumItem: null
};




function ChromeGlobalsView() {
  dumpn("ChromeGlobalsView was instantiated");
  MenuContainer.call(this);
  this._onSelect = this._onSelect.bind(this);
  this._onClick = this._onClick.bind(this);
}

create({ constructor: ChromeGlobalsView, proto: MenuContainer.prototype }, {
  


  initialize: function DVCG_initialize() {
    dumpn("Initializing the ChromeGlobalsView");
    this._container = document.getElementById("chrome-globals");
    this._emptyLabel = L10N.getStr("noGlobalsText");
    this._unavailableLabel = L10N.getStr("noMatchingGlobalsText");

    this._container.addEventListener("select", this._onSelect, false);
    this._container.addEventListener("click", this._onClick, false);

    this.empty();
  },

  


  destroy: function DVT_destroy() {
    dumpn("Destroying the ChromeGlobalsView");
    this._container.removeEventListener("select", this._onSelect, false);
    this._container.removeEventListener("click", this._onClick, false);
  },

  


  _onSelect: function DVCG__onSelect() {
    if (!this.refresh()) {
      return;
    }
    
  },

  


  _onClick: function DVCG__onClick() {
    DebuggerView.Filtering.target = this;
  }
});




function SourcesView() {
  dumpn("SourcesView was instantiated");
  MenuContainer.call(this);
  this._onSelect = this._onSelect.bind(this);
  this._onClick = this._onClick.bind(this);
}

create({ constructor: SourcesView, proto: MenuContainer.prototype }, {
  


  initialize: function DVS_initialize() {
    dumpn("Initializing the SourcesView");
    this._container = document.getElementById("sources");
    this._emptyLabel = L10N.getStr("noScriptsText");
    this._unavailableLabel = L10N.getStr("noMatchingScriptsText");

    this._container.addEventListener("select", this._onSelect, false);
    this._container.addEventListener("click", this._onClick, false);

    this.empty();
  },

  


  destroy: function DVS_destroy() {
    dumpn("Destroying the SourcesView");
    this._container.removeEventListener("select", this._onSelect, false);
    this._container.removeEventListener("click", this._onClick, false);
  },

  


  _onSelect: function DVS__onSelect() {
    if (!this.refresh()) {
      return;
    }
    DebuggerView.setEditorSource(this.selectedItem.attachment);
  },

  


  _onClick: function DVS__onClick() {
    DebuggerView.Filtering.target = this;
  }
});




let SourceUtils = {
  _labelsCache: new Map(),

  







  getSourceLabel: function SU_getSourceLabel(aUrl) {
    if (!this._labelsCache.has(aUrl)) {
      this._labelsCache.set(aUrl, this.trimUrlLength(this.trimUrl(aUrl)));
    }
    return this._labelsCache.get(aUrl);
  },

  



  clearLabelsCache: function SU_clearLabelsCache() {
    this._labelsCache = new Map();
  },

  










  trimUrlLength: function SU_trimUrlLength(aUrl, aMaxLength = SOURCE_URL_MAX_LENGTH) {
    if (aUrl.length > aMaxLength) {
      return aUrl.substring(0, aMaxLength) + L10N.ellipsis;
    }
    return aUrl;
  },

  







  trimUrlQuery: function SU_trimUrlQuery(aUrl) {
    let length = aUrl.length;
    let q1 = aUrl.indexOf('?');
    let q2 = aUrl.indexOf('&');
    let q3 = aUrl.indexOf('#');
    let q = Math.min(q1 != -1 ? q1 : length,
                     q2 != -1 ? q2 : length,
                     q3 != -1 ? q3 : length);

    return aUrl.slice(0, q);
  },

  












  trimUrl: function SU_trimUrl(aUrl, aLabel, aSeq) {
    if (!(aUrl instanceof Ci.nsIURL)) {
      try {
        
        aUrl = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
      } catch (e) {
        
        return aUrl;
      }
    }
    if (!aSeq) {
      let name = aUrl.fileName;
      if (name) {
        
        

        
        
        aLabel = aUrl.fileName.replace(/\&.*/, "");
      } else {
        
        
        aLabel = "";
      }
      aSeq = 1;
    }

    
    if (aLabel && aLabel.indexOf("?") != 0) {
      if (DebuggerView.Sources.containsTrimmedValue(aUrl.spec)) {
        
        
        return aLabel;
      }
      if (!DebuggerView.Sources.containsLabel(aLabel)) {
        
        return aLabel;
      }
    }

    
    if (aSeq == 1) {
      let query = aUrl.query;
      if (query) {
        return this.trimUrl(aUrl, aLabel + "?" + query, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 2) {
      let ref = aUrl.ref;
      if (ref) {
        return this.trimUrl(aUrl, aLabel + "#" + aUrl.ref, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 3) {
      let dir = aUrl.directory;
      if (dir) {
        return this.trimUrl(aUrl, dir.replace(/^\//, "") + aLabel, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 4) {
      let host = aUrl.hostPort;
      if (host) {
        return this.trimUrl(aUrl, host + "/" + aLabel, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 5) {
      return this.trimUrl(aUrl, aUrl.specIgnoringRef, aSeq + 1);
    }
    
    return aUrl.spec;
  }
};




function FilterView() {
  dumpn("FilterView was instantiated");
  this._onClick = this._onClick.bind(this);
  this._onSearch = this._onSearch.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
  this._onBlur = this._onBlur.bind(this);
}

FilterView.prototype = {
  


  initialize: function DVF_initialize() {
    dumpn("Initializing the FilterView");
    this._searchbox = document.getElementById("searchbox");
    this._searchboxPanel = document.getElementById("searchbox-panel");
    this._globalOperatorButton = document.getElementById("global-operator-button");
    this._globalOperatorLabel = document.getElementById("global-operator-label");
    this._tokenOperatorButton = document.getElementById("token-operator-button");
    this._tokenOperatorLabel = document.getElementById("token-operator-label");
    this._lineOperatorButton = document.getElementById("line-operator-button");
    this._lineOperatorLabel = document.getElementById("line-operator-label");

    this._globalSearchKey = LayoutHelpers.prettyKey(document.getElementById("globalSearchKey"));
    this._fileSearchKey = LayoutHelpers.prettyKey(document.getElementById("fileSearchKey"));
    this._lineSearchKey = LayoutHelpers.prettyKey(document.getElementById("lineSearchKey"));
    this._tokenSearchKey = LayoutHelpers.prettyKey(document.getElementById("tokenSearchKey"));

    this._searchbox.addEventListener("click", this._onClick, false);
    this._searchbox.addEventListener("select", this._onSearch, false);
    this._searchbox.addEventListener("input", this._onSearch, false);
    this._searchbox.addEventListener("keypress", this._onKeyPress, false);
    this._searchbox.addEventListener("blur", this._onBlur, false);

    this._globalOperatorButton.setAttribute("label", SEARCH_GLOBAL_FLAG);
    this._tokenOperatorButton.setAttribute("label", SEARCH_TOKEN_FLAG);
    this._lineOperatorButton.setAttribute("label", SEARCH_LINE_FLAG);

    this._globalOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGlobal", [this._globalSearchKey]));
    this._tokenOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelToken", [this._tokenSearchKey]));
    this._lineOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelLine", [this._lineSearchKey]));

    if (window._isChromeDebugger) {
      this.target = DebuggerView.ChromeGlobals;
    } else {
      this.target = DebuggerView.Sources;
    }
  },

  


  destroy: function DVF_destroy() {
    dumpn("Destroying the FilterView");
    this._searchbox.removeEventListener("click", this._onClick, false);
    this._searchbox.removeEventListener("select", this._onSearch, false);
    this._searchbox.removeEventListener("input", this._onSearch, false);
    this._searchbox.removeEventListener("keypress", this._onKeyPress, false);
    this._searchbox.removeEventListener("blur", this._onBlur, false);
  },

  



  set target(aView) {
    var placeholder = "";
    switch (aView) {
      case DebuggerView.ChromeGlobals:
        placeholder = L10N.getFormatStr("emptyChromeGlobalsFilterText", [this._fileSearchKey]);
        break;
      case DebuggerView.Sources:
        placeholder = L10N.getFormatStr("emptyFilterText", [this._fileSearchKey]);
        break;
    }
    this._searchbox.setAttribute("placeholder", placeholder);
    this._target = aView;
  },

  



  get searchboxInfo() {
    let file, line, token, global;

    let rawValue = this._searchbox.value;
    let rawLength = rawValue.length;
    let globalFlagIndex = rawValue.indexOf(SEARCH_GLOBAL_FLAG);
    let lineFlagIndex = rawValue.lastIndexOf(SEARCH_LINE_FLAG);
    let tokenFlagIndex = rawValue.lastIndexOf(SEARCH_TOKEN_FLAG);

    
    if (globalFlagIndex != 0) {
      let fileEnd = lineFlagIndex != -1
        ? lineFlagIndex
        : tokenFlagIndex != -1 ? tokenFlagIndex : rawLength;

      let lineEnd = tokenFlagIndex != -1
        ? tokenFlagIndex
        : rawLength;

      file = rawValue.slice(0, fileEnd);
      line = ~~(rawValue.slice(fileEnd + 1, lineEnd)) || -1;
      token = rawValue.slice(lineEnd + 1);
      global = false;
    }
    
    else {
      file = "";
      line = -1;
      token = rawValue.slice(1);
      global = true;
    }

    return [file, line, token, global];
  },

  



  get searchedFile() this.searchboxInfo[0],

  



  get searchedLine() this.searchboxInfo[1],

  



  get searchedToken() this.searchboxInfo[2],

  


  clearSearch: function DVF_clearSearch() {
    this._searchbox.value = "";
    this._onSearch();
  },

  





  _performFileSearch: function DVF__performFileSearch(aFile) {
    
    if (this._prevSearchedFile == aFile) {
      return;
    }

    let view = this._target;

    
    if (!aFile) {
      for (let item in view) {
        item.target.hidden = false;
      }
      view.refresh();
    }
    
    else {
      let found = false;
      let lowerCaseFile = aFile.toLowerCase();

      for (let item in view) {
        let element = item.target;
        let lowerCaseLabel = item.label.toLowerCase();

        
        if (lowerCaseLabel.match(lowerCaseFile)) {
          element.hidden = false;

          
          if (!found) {
            found = true;
            view.selectedItem = item;
          }
        }
        
        else {
          element.hidden = true;
        }
      }
      
      if (!found) {
        view.setUnavailable();
      }
    }
    this._prevSearchedFile = aFile;
  },

  






  _performLineSearch: function DVF__performLineSearch(aLine) {
    
    if (this._prevSearchedLine != aLine && aLine > -1) {
      DebuggerView.editor.setCaretPosition(aLine - 1);
    }
    this._prevSearchedLine = aLine;
  },

  






  _performTokenSearch: function DVF__performTokenSearch(aToken) {
    
    if (this._prevSearchedToken != aToken && aToken.length > 0) {
      let editor = DebuggerView.editor;
      let offset = editor.find(aToken, { ignoreCase: true });
      if (offset > -1) {
        editor.setSelection(offset, offset + aToken.length)
      }
    }
    this._prevSearchedToken = aToken;
  },

  


  _onClick: function DVF__onClick() {
    this._searchboxPanel.openPopup(this._searchbox);
  },

  


  _onSearch: function DVF__onScriptsSearch() {
    this._searchboxPanel.hidePopup();
    let [file, line, token, global] = this.searchboxInfo;

    
    
    if (global) {
      DebuggerView.GlobalSearch.scheduleSearch();
    } else {
      DebuggerView.GlobalSearch.clearView();
      this._performFileSearch(file);
      this._performLineSearch(line);
      this._performTokenSearch(token);
    }
  },

  


  _onKeyPress: function DVF__onScriptsKeyPress(e) {
    let [file, line, token, global] = this.searchboxInfo;
    let action;

    switch (e.keyCode) {
      case e.DOM_VK_DOWN:
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        action = 0;
        break;
      case e.DOM_VK_UP:
        action = 1;
        break;
      case e.DOM_VK_ESCAPE:
        action = 2;
        break;
      default:
        action = -1;
    }

    if (action == 2) {
      DebuggerView.editor.focus();
      return;
    }
    if (action == -1 || !token) {
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    if (global) {
      if (DebuggerView.GlobalSearch.hidden) {
        DebuggerView.GlobalSearch.scheduleSearch();
      } else {
        DebuggerView.GlobalSearch[["focusNextMatch", "focusPrevMatch"][action]]();
      }
    } else {
      let editor = DebuggerView.editor;
      let offset = editor[["findNext", "findPrevious"][action]](true);
      if (offset > -1) {
        editor.setSelection(offset, offset + token.length)
      }
    }
  },

  


  _onBlur: function DVF__onBlur() {
    DebuggerView.GlobalSearch.clearView();
    this._searchboxPanel.hidePopup();
  },

  





  _doSearch: function DVF__doSearch(aOperator = "") {
    this._searchbox.focus();
    this._searchbox.value = aOperator;
    DebuggerView.GlobalSearch.clearView();
  },

  


  _doFileSearch: function DVF__doFileSearch() {
    this._doSearch();
    this._searchboxPanel.openPopup(this._searchbox);
  },

  


  _doLineSearch: function DVF__doLineSearch() {
    this._doSearch(SEARCH_LINE_FLAG);
    this._searchboxPanel.hidePopup();
  },

  


  _doTokenSearch: function DVF__doTokenSearch() {
    this._doSearch(SEARCH_TOKEN_FLAG);
    this._searchboxPanel.hidePopup();
  },

  


  _doGlobalSearch: function DVF__doGlobalSearch() {
    this._doSearch(SEARCH_GLOBAL_FLAG);
    this._searchboxPanel.hidePopup();
  },

  _searchbox: null,
  _searchboxPanel: null,
  _globalOperatorButton: null,
  _globalOperatorLabel: null,
  _tokenOperatorButton: null,
  _tokenOperatorLabel: null,
  _lineOperatorButton: null,
  _lineOperatorLabel: null,
  _globalSearchKey: "",
  _fileSearchKey: "",
  _lineSearchKey: "",
  _tokenSearchKey: "",
  _target: null,
  _prevSearchedFile: "",
  _prevSearchedLine: -1,
  _prevSearchedToken: ""
};




DebuggerView.Toolbar = new ToolbarView();
DebuggerView.Options = new OptionsView();
DebuggerView.ChromeGlobals = new ChromeGlobalsView();
DebuggerView.Sources = new SourcesView();
DebuggerView.Filtering = new FilterView();
