




"use strict";





function ToolbarView() {
  dumpn("ToolbarView was instantiated");
  this._onTogglePanesPressed = this._onTogglePanesPressed.bind(this);
  this._onResumePressed = this._onResumePressed.bind(this);
  this._onStepOverPressed = this._onStepOverPressed.bind(this);
  this._onStepInPressed = this._onStepInPressed.bind(this);
  this._onStepOutPressed = this._onStepOutPressed.bind(this);
}

ToolbarView.prototype = {
  


  initialize: function DVT_initialize() {
    dumpn("Initializing the ToolbarView");
    this._togglePanesButton = document.getElementById("toggle-panes");
    this._resumeButton = document.getElementById("resume");
    this._stepOverButton = document.getElementById("step-over");
    this._stepInButton = document.getElementById("step-in");
    this._stepOutButton = document.getElementById("step-out");
    this._chromeGlobals = document.getElementById("chrome-globals");
    this._sources = document.getElementById("sources");

    let resumeKey = LayoutHelpers.prettyKey(document.getElementById("resumeKey"), true);
    let stepOverKey = LayoutHelpers.prettyKey(document.getElementById("stepOverKey"), true);
    let stepInKey = LayoutHelpers.prettyKey(document.getElementById("stepInKey"), true);
    let stepOutKey = LayoutHelpers.prettyKey(document.getElementById("stepOutKey"), true);
    this._resumeTooltip = L10N.getFormatStr("resumeButtonTooltip", [resumeKey]);
    this._pauseTooltip = L10N.getFormatStr("pauseButtonTooltip", [resumeKey]);
    this._stepOverTooltip = L10N.getFormatStr("stepOverTooltip", [stepOverKey]);
    this._stepInTooltip = L10N.getFormatStr("stepInTooltip", [stepInKey]);
    this._stepOutTooltip = L10N.getFormatStr("stepOutTooltip", [stepOutKey]);

    this._togglePanesButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.addEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.addEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.addEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.addEventListener("mousedown", this._onStepOutPressed, false);

    this._stepOverButton.setAttribute("tooltiptext", this._stepOverTooltip);
    this._stepInButton.setAttribute("tooltiptext", this._stepInTooltip);
    this._stepOutButton.setAttribute("tooltiptext", this._stepOutTooltip);

    
    
  },

  


  destroy: function DVT_destroy() {
    dumpn("Destroying the ToolbarView");
    this._togglePanesButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.removeEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.removeEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.removeEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.removeEventListener("mousedown", this._onStepOutPressed, false);
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

  


  _onTogglePanesPressed: function DVT__onTogglePanesPressed() {
    DebuggerView.togglePanes({
      visible: DebuggerView.panesHidden,
      animated: true,
      delayed: true
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
  this._toggleShowVariablesOnlyEnum = this._toggleShowVariablesOnlyEnum.bind(this);
  this._toggleShowVariablesFilterBox = this._toggleShowVariablesFilterBox.bind(this);
}

OptionsView.prototype = {
  


  initialize: function DVO_initialize() {
    dumpn("Initializing the OptionsView");
    this._button = document.getElementById("debugger-options");
    this._pauseOnExceptionsItem = document.getElementById("pause-on-exceptions");
    this._showPanesOnStartupItem = document.getElementById("show-panes-on-startup");
    this._showVariablesOnlyEnumItem = document.getElementById("show-vars-only-enum");
    this._showVariablesFilterBoxItem = document.getElementById("show-vars-filter-box");

    this._pauseOnExceptionsItem.setAttribute("checked", Prefs.pauseOnExceptions);
    this._showPanesOnStartupItem.setAttribute("checked", Prefs.panesVisibleOnStartup);
    this._showVariablesOnlyEnumItem.setAttribute("checked", Prefs.variablesOnlyEnumVisible);
    this._showVariablesFilterBoxItem.setAttribute("checked", Prefs.variablesSearchboxVisible);
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
    DebuggerController.activeThread.pauseOnExceptions(Prefs.pauseOnExceptions =
      this._pauseOnExceptionsItem.getAttribute("checked") == "true");
  },

  


  _toggleShowPanesOnStartup: function DVO__toggleShowPanesOnStartup() {
    Prefs.panesVisibleOnStartup =
      this._showPanesOnStartupItem.getAttribute("checked") == "true";
  },

  


  _toggleShowVariablesOnlyEnum: function DVO__toggleShowVariablesOnlyEnum() {
    DebuggerView.Variables.onlyEnumVisible = Prefs.variablesOnlyEnumVisible =
      this._showVariablesOnlyEnumItem.getAttribute("checked") == "true";
  },

  


  _toggleShowVariablesFilterBox: function DVO__toggleShowVariablesFilterBox() {
    DebuggerView.Variables.searchEnabled = Prefs.variablesSearchboxVisible =
      this._showVariablesFilterBoxItem.getAttribute("checked") == "true";
  },

  _button: null,
  _pauseOnExceptionsItem: null,
  _showPanesOnStartupItem: null,
  _showVariablesOnlyEnumItem: null,
  _showVariablesFilterBoxItem: null
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
    this.node = document.getElementById("chrome-globals");
    this._emptyLabel = L10N.getStr("noGlobalsText");
    this._unavailableLabel = L10N.getStr("noMatchingGlobalsText");

    this.node.addEventListener("select", this._onSelect, false);
    this.node.addEventListener("click", this._onClick, false);

    this.empty();
  },

  


  destroy: function DVT_destroy() {
    dumpn("Destroying the ChromeGlobalsView");
    this.node.removeEventListener("select", this._onSelect, false);
    this.node.removeEventListener("click", this._onClick, false);
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
    this.node = document.getElementById("sources");
    this._emptyLabel = L10N.getStr("noScriptsText");
    this._unavailableLabel = L10N.getStr("noMatchingScriptsText");

    this.node.addEventListener("select", this._onSelect, false);
    this.node.addEventListener("click", this._onClick, false);

    this.empty();
  },

  


  destroy: function DVS_destroy() {
    dumpn("Destroying the SourcesView");
    this.node.removeEventListener("select", this._onSelect, false);
    this.node.removeEventListener("click", this._onClick, false);
  },

  



  set preferredSource(aValue) {
    this._preferredValue = aValue;

    
    
    if (this.containsValue(aValue)) {
      this.selectedValue = aValue;
    }
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

  



  clearLabelsCache: function SU_clearLabelsCache() {
    this._labelsCache = new Map();
  },

  











  getSourceLabel: function SU_getSourceLabel(aUrl, aLength, aSection) {
    let id = [aUrl, aLength, aSection].join();
    aLength = aLength || SOURCE_URL_DEFAULT_MAX_LENGTH;
    aSection = aSection || "end";

    if (this._labelsCache.has(id)) {
      return this._labelsCache.get(id);
    }
    let sourceLabel = this.trimUrlLength(this.trimUrl(aUrl), aLength, aSection);
    this._labelsCache.set(id, sourceLabel);
    return sourceLabel;
  },

  












  trimUrlLength: function SU_trimUrlLength(aUrl, aLength, aSection) {
    aLength = aLength || SOURCE_URL_DEFAULT_MAX_LENGTH;
    aSection = aSection || "end";

    if (aUrl.length > aLength) {
      switch (aSection) {
        case "start":
          return L10N.ellipsis + aUrl.slice(-aLength);
          break;
        case "center":
          return aUrl.substr(0, aLength / 2 - 1) + L10N.ellipsis + aUrl.slice(-aLength / 2 + 1);
          break;
        case "end":
          return aUrl.substr(0, aLength) + L10N.ellipsis;
          break;
      }
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
      if (DebuggerView.Sources.containsTrimmedValue(aUrl.spec, SourceUtils.trimUrlQuery)) {
        
        
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




function StackFramesView() {
  dumpn("StackFramesView was instantiated");
  MenuContainer.call(this);
  this._createItemView = this._createItemView.bind(this);
  this._onStackframeRemoved = this._onStackframeRemoved.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onScroll = this._onScroll.bind(this);
  this._afterScroll = this._afterScroll.bind(this);
  this._selectFrame = this._selectFrame.bind(this);
}

create({ constructor: StackFramesView, proto: MenuContainer.prototype }, {
  


  initialize: function DVSF_initialize() {
    dumpn("Initializing the StackFramesView");

    let commandset = this._commandset = document.createElement("commandset");
    let menupopup = this._menupopup = document.createElement("menupopup");
    commandset.setAttribute("id", "stackframesCommandset");
    menupopup.setAttribute("id", "stackframesMenupopup");

    document.getElementById("debuggerPopupset").appendChild(menupopup);
    document.getElementById("debuggerCommands").appendChild(commandset);

    this.node = new BreadcrumbsWidget(document.getElementById("stackframes"));
    this.decorateWidgetMethods("parentNode");
    this.node.addEventListener("click", this._onClick, false);
    this.node.addEventListener("scroll", this._onScroll, true);
    window.addEventListener("resize", this._onScroll, true);

    this._cache = new Map();
  },

  


  destroy: function DVSF_destroy() {
    dumpn("Destroying the StackFramesView");
    this.node.removeEventListener("click", this._onClick, false);
    this.node.removeEventListener("scroll", this._onScroll, true);
    window.removeEventListener("resize", this._onScroll, true);
  },

  











  addFrame:
  function DVSF_addFrame(aFrameTitle, aSourceLocation, aLineNumber, aDepth) {
    
    let stackframeFragment = this._createItemView.apply(this, arguments);
    let stackframePopup = this._createMenuItem.apply(this, arguments);

    
    let stackframeItem = this.push(stackframeFragment, {
      index: FIRST, 
      relaxed: true, 
      attachment: {
        popup: stackframePopup,
        depth: aDepth
      }
    });

    let element = stackframeItem.target;
    element.id = "stackframe-" + aDepth;
    element.classList.add("dbg-stackframe");
    element.setAttribute("tooltiptext", aSourceLocation + ":" + aLineNumber);
    element.setAttribute("contextmenu", "stackframesMenupopup");

    stackframeItem.finalize = this._onStackframeRemoved;
    this._cache.set(aDepth, stackframeItem);
  },

  





  highlightFrame: function DVSF_highlightFrame(aDepth) {
    let cache = this._cache;
    let selectedItem = this.selectedItem = cache.get(aDepth);

    for (let [, item] of cache) {
      if (item != selectedItem) {
        item.attachment.popup.menuitem.removeAttribute("checked");
      } else {
        item.attachment.popup.menuitem.setAttribute("checked", "");
      }
    }
  },

  


  dirty: false,

  









  _createItemView:
  function DVSF__createItemView(aFrameTitle, aSourceLocation, aLineNumber) {
    let frameTitleNode = document.createElement("label");
    let frameDetailsNode = document.createElement("label");

    let frameDetails = SourceUtils.getSourceLabel(aSourceLocation,
      STACK_FRAMES_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_SOURCE_URL_TRIM_SECTION) +
      SEARCH_LINE_FLAG + aLineNumber;

    frameTitleNode.className = "plain dbg-stackframe-title inspector-breadcrumbs-tag";
    frameTitleNode.setAttribute("value", aFrameTitle);

    frameDetailsNode.className = "plain dbg-stackframe-details inspector-breadcrumbs-id";
    frameDetailsNode.setAttribute("value", " " + frameDetails);

    let fragment = document.createDocumentFragment();
    fragment.appendChild(frameTitleNode);
    fragment.appendChild(frameDetailsNode);

    return fragment;
  },

  











  _createMenuItem:
  function DVSF__createMenuItem(aFrameTitle, aSourceLocation, aLineNumber, aDepth) {
    let menuitem = document.createElement("menuitem");
    let command = document.createElement("command");

    let frameDescription = SourceUtils.getSourceLabel(aSourceLocation,
      STACK_FRAMES_POPUP_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_POPUP_SOURCE_URL_TRIM_SECTION) +
      SEARCH_LINE_FLAG + aLineNumber;

    let prefix = "sf-cMenu-"; 
    let commandId = prefix + aDepth + "-" + "-command";
    let menuitemId = prefix + aDepth + "-" + "-menuitem";

    command.id = commandId;
    command.addEventListener("command", this._selectFrame.bind(this, aDepth), false);

    menuitem.id = menuitemId;
    menuitem.className = "dbg-stackframe-menuitem";
    menuitem.setAttribute("type", "checkbox");
    menuitem.setAttribute("command", commandId);
    menuitem.setAttribute("tooltiptext", aSourceLocation + ":" + aLineNumber);

    let labelNode = document.createElement("label");
    labelNode.className = "plain dbg-stackframe-menuitem-title";
    labelNode.setAttribute("value", aFrameTitle);
    labelNode.setAttribute("flex", "1");

    let descriptionNode = document.createElement("label");
    descriptionNode.className = "plain dbg-stackframe-menuitem-details";
    descriptionNode.setAttribute("value", frameDescription);

    menuitem.appendChild(labelNode);
    menuitem.appendChild(descriptionNode);

    this._commandset.appendChild(command);
    this._menupopup.appendChild(menuitem);

    return {
      command: command,
      menuitem: menuitem
    };
  },

  





  _destroyMenuItem: function DVSF__destroyMenuItem(aPopup) {
    let command = aPopup.command;
    let menuitem = aPopup.menuitem;

    command.parentNode.removeChild(command);
    menuitem.parentNode.removeChild(menuitem);
  },

  


  _onStackframeRemoved: function DVSF__onStackframeRemoved(aItem) {
    this._destroyMenuItem(aItem.attachment.popup);
  },

  


  _onClick: function DVSF__onClick(e) {
    if (e && e.button != 0) {
      
      return;
    }
    let item = this.getItemForElement(e.target);
    if (item) {
      
      this._selectFrame(item.attachment.depth);
    }
  },

  


  _onScroll: function DVSF__onScroll() {
    
    if (!this.dirty) {
      return;
    }
    window.clearTimeout(this._scrollTimeout);
    this._scrollTimeout = window.setTimeout(this._afterScroll, STACK_FRAMES_SCROLL_DELAY);
  },

  


  _afterScroll: function DVSF__afterScroll() {
    let list = this.node._list;
    let scrollPosition = list.scrollPosition;
    let scrollWidth = list.scrollWidth;

    
    
    if (scrollPosition - scrollWidth / 10 < 1) {
      list.ensureElementIsVisible(this.getItemAtIndex(CALL_STACK_PAGE_SIZE - 1).target);
      this.dirty = false;

      
      DebuggerController.StackFrames.addMoreFrames();
    }
  },

  





  _selectFrame: function DVSF__selectFrame(aDepth) {
    DebuggerController.StackFrames.selectFrame(aDepth);
  },

  _commandset: null,
  _menupopup: null,
  _cache: null,
  _scrollTimeout: null,
});




let StackFrameUtils = {
  






  getFrameTitle: function SFU_getFrameTitle(aFrame) {
    if (aFrame.type == "call") {
      let c = aFrame.callee;
      return (c.name || c.userDisplayName || c.displayName || "(anonymous)");
    }
    return "(" + aFrame.type + ")";
  },

  







  getScopeLabel: function SFU_getScopeLabel(aEnv) {
    let name = "";

    
    if (!aEnv.parent) {
      name = L10N.getStr("globalScopeLabel");
    }
    
    else {
      name = aEnv.type.charAt(0).toUpperCase() + aEnv.type.slice(1);
    }

    let label = L10N.getFormatStr("scopeLabel", [name]);
    switch (aEnv.type) {
      case "with":
      case "object":
        label += " [" + aEnv.object.class + "]";
        break;
      case "function":
        let f = aEnv.function;
        label += " [" +
          (f.name || f.userDisplayName || f.displayName || "(anonymous)") +
        "]";
        break;
    }
    return label;
  },
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
    this._variableOperatorButton = document.getElementById("variable-operator-button");
    this._variableOperatorLabel = document.getElementById("variable-operator-label");

    this._fileSearchKey = LayoutHelpers.prettyKey(document.getElementById("fileSearchKey"), true);
    this._globalSearchKey = LayoutHelpers.prettyKey(document.getElementById("globalSearchKey"), true);
    this._tokenSearchKey = LayoutHelpers.prettyKey(document.getElementById("tokenSearchKey"), true);
    this._lineSearchKey = LayoutHelpers.prettyKey(document.getElementById("lineSearchKey"), true);
    this._variableSearchKey = LayoutHelpers.prettyKey(document.getElementById("variableSearchKey"), true);

    this._searchbox.addEventListener("click", this._onClick, false);
    this._searchbox.addEventListener("select", this._onSearch, false);
    this._searchbox.addEventListener("input", this._onSearch, false);
    this._searchbox.addEventListener("keypress", this._onKeyPress, false);
    this._searchbox.addEventListener("blur", this._onBlur, false);

    this._globalOperatorButton.setAttribute("label", SEARCH_GLOBAL_FLAG);
    this._tokenOperatorButton.setAttribute("label", SEARCH_TOKEN_FLAG);
    this._lineOperatorButton.setAttribute("label", SEARCH_LINE_FLAG);
    this._variableOperatorButton.setAttribute("label", SEARCH_VARIABLE_FLAG);

    this._globalOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGlobal", [this._globalSearchKey]));
    this._tokenOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelToken", [this._tokenSearchKey]));
    this._lineOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelLine", [this._lineSearchKey]));
    this._variableOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelVariable", [this._variableSearchKey]));

    
    
    
    
    this.target = DebuggerView.Sources;
    
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
    let placeholder = "";
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
    let file, line, token, isGlobal, isVariable;

    let rawValue = this._searchbox.value;
    let rawLength = rawValue.length;
    let globalFlagIndex = rawValue.indexOf(SEARCH_GLOBAL_FLAG);
    let variableFlagIndex = rawValue.indexOf(SEARCH_VARIABLE_FLAG);
    let lineFlagIndex = rawValue.lastIndexOf(SEARCH_LINE_FLAG);
    let tokenFlagIndex = rawValue.lastIndexOf(SEARCH_TOKEN_FLAG);

    
    if (globalFlagIndex != 0 && variableFlagIndex != 0) {
      let fileEnd = lineFlagIndex != -1
        ? lineFlagIndex
        : tokenFlagIndex != -1 ? tokenFlagIndex : rawLength;

      let lineEnd = tokenFlagIndex != -1
        ? tokenFlagIndex
        : rawLength;

      file = rawValue.slice(0, fileEnd);
      line = ~~(rawValue.slice(fileEnd + 1, lineEnd)) || 0;
      token = rawValue.slice(lineEnd + 1);
      isGlobal = false;
      isVariable = false;
    }
    
    else if (globalFlagIndex == 0) {
      file = "";
      line = 0;
      token = rawValue.slice(1);
      isGlobal = true;
      isVariable = false;
    }
    
    else if (variableFlagIndex == 0) {
      file = "";
      line = 0;
      token = rawValue.slice(1);
      isGlobal = false;
      isVariable = true;
    }

    return [file, line, token, isGlobal, isVariable];
  },

  



  get searchedFile() this.searchboxInfo[0],

  



  get searchedLine() this.searchboxInfo[1],

  



  get searchedToken() this.searchboxInfo[2],

  


  clearSearch: function DVF_clearSearch() {
    this._searchbox.value = "";
    this._searchboxPanel.hidePopup();
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
            view.refresh();
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
    
    DebuggerView.FilteredSources.syncFileSearch();

    this._prevSearchedFile = aFile;
  },

  






  _performLineSearch: function DVF__performLineSearch(aLine) {
    
    if (this._prevSearchedLine != aLine && aLine) {
      DebuggerView.editor.setCaretPosition(aLine - 1);
    }
    
    if (this._prevSearchedToken && !aLine) {
      this._target.refresh();
    }
    this._prevSearchedLine = aLine;
  },

  






  _performTokenSearch: function DVF__performTokenSearch(aToken) {
    
    if (this._prevSearchedToken != aToken && aToken) {
      let editor = DebuggerView.editor;
      let offset = editor.find(aToken, { ignoreCase: true });
      if (offset > -1) {
        editor.setSelection(offset, offset + aToken.length)
      }
    }
    
    if (this._prevSearchedLine && !aToken) {
      this._target.refresh();
    }
    this._prevSearchedToken = aToken;
  },

  


  _onClick: function DVF__onClick() {
    this._searchboxPanel.openPopup(this._searchbox);
  },

  


  _onSearch: function DVF__onScriptsSearch() {
    this._searchboxPanel.hidePopup();
    let [file, line, token, isGlobal, isVariable] = this.searchboxInfo;

    
    
    if (isGlobal) {
      DebuggerView.GlobalSearch.scheduleSearch(token);
      this._prevSearchedToken = token;
      return;
    }

    
    
    if (isVariable) {
      DebuggerView.Variables.scheduleSearch(token);
      this._prevSearchedToken = token;
      return;
    }

    DebuggerView.GlobalSearch.clearView();
    this._performFileSearch(file);
    this._performLineSearch(line);
    this._performTokenSearch(token);
  },

  


  _onKeyPress: function DVF__onScriptsKeyPress(e) {
    
    e.char = String.fromCharCode(e.charCode);

    let [file, line, token, isGlobal, isVariable] = this.searchboxInfo;
    let isFileSearch, isLineSearch, isDifferentToken, isReturnKey;
    let action = -1;

    if (file && !line && !token) {
      isFileSearch = true;
    }
    if (line && !token) {
      isLineSearch = true;
    }
    if (this._prevSearchedToken != token) {
      isDifferentToken = true;
    }

    
    if ((e.char == "g" && e.metaKey) || e.char == "n" && e.ctrlKey) {
      action = 0;
    }
    
    else if ((e.char == "G" && e.metaKey) || e.char == "p" && e.ctrlKey) {
      action = 1;
    }
    
    
    else switch (e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        isReturnKey = true;
        
      case e.DOM_VK_DOWN:
        action = 0;
        break;
      case e.DOM_VK_UP:
        action = 1;
        break;
      case e.DOM_VK_ESCAPE:
        action = 2;
        break;
    }

    if (action == 2) {
      DebuggerView.editor.focus();
      return;
    }
    if (action == -1 || (!file && !line && !token)) {
      DebuggerView.FilteredSources.hidden = true;
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    
    if (isFileSearch) {
      if (isReturnKey) {
        DebuggerView.FilteredSources.hidden = true;
        DebuggerView.editor.focus();
        this.clearSearch();
      } else {
        DebuggerView.FilteredSources[["focusNext", "focusPrev"][action]]();
      }
      this._prevSearchedFile = file;
      return;
    }

    
    if (isGlobal) {
      if (isReturnKey && (isDifferentToken || DebuggerView.GlobalSearch.hidden)) {
        DebuggerView.GlobalSearch.performSearch(token);
      } else {
        DebuggerView.GlobalSearch[["focusNextMatch", "focusPrevMatch"][action]]();
      }
      this._prevSearchedToken = token;
      return;
    }

    
    if (isVariable) {
      if (isReturnKey && isDifferentToken) {
        DebuggerView.Variables.performSearch(token);
      } else {
        DebuggerView.Variables.expandFirstSearchResults();
      }
      this._prevSearchedToken = token;
      return;
    }

    
    if (isLineSearch && !isReturnKey) {
      line += action == 0 ? 1 : -1;
      let lineCount = DebuggerView.editor.getLineCount();
      let lineTarget = line < 1 ? 1 : line > lineCount ? lineCount : line;

      DebuggerView.editor.setCaretPosition(lineTarget - 1);
      this._searchbox.value = file + SEARCH_LINE_FLAG + lineTarget;
      this._prevSearchedLine = lineTarget;
      return;
    }

    let editor = DebuggerView.editor;
    let offset = editor[["findNext", "findPrevious"][action]](true);
    if (offset > -1) {
      editor.setSelection(offset, offset + token.length)
    }
  },

  


  _onBlur: function DVF__onBlur() {
    DebuggerView.GlobalSearch.clearView();
    DebuggerView.Variables.performSearch(null);
    this._searchboxPanel.hidePopup();
  },

  





  _doSearch: function DVF__doSearch(aOperator = "") {
    this._searchbox.focus();
    this._searchbox.value = aOperator;
  },

  


  _doFileSearch: function DVF__doFileSearch() {
    this._doSearch();
    this._searchboxPanel.openPopup(this._searchbox);
  },

  


  _doGlobalSearch: function DVF__doGlobalSearch() {
    this._doSearch(SEARCH_GLOBAL_FLAG);
    this._searchboxPanel.hidePopup();
  },

  


  _doTokenSearch: function DVF__doTokenSearch() {
    this._doSearch(SEARCH_TOKEN_FLAG);
    this._searchboxPanel.hidePopup();
  },

  


  _doLineSearch: function DVF__doLineSearch() {
    this._doSearch(SEARCH_LINE_FLAG);
    this._searchboxPanel.hidePopup();
  },

  


  _doVariableSearch: function DVF__doVariableSearch() {
    DebuggerView.Variables.performSearch("");
    this._doSearch(SEARCH_VARIABLE_FLAG);
    this._searchboxPanel.hidePopup();
  },

  


  _doVariablesFocus: function DVG__doVariablesFocus() {
    DebuggerView.showPanesSoon();
    DebuggerView.Variables.focusFirstVisibleNode();
  },

  _searchbox: null,
  _searchboxPanel: null,
  _globalOperatorButton: null,
  _globalOperatorLabel: null,
  _tokenOperatorButton: null,
  _tokenOperatorLabel: null,
  _lineOperatorButton: null,
  _lineOperatorLabel: null,
  _variableOperatorButton: null,
  _variableOperatorLabel: null,
  _fileSearchKey: "",
  _globalSearchKey: "",
  _tokenSearchKey: "",
  _lineSearchKey: "",
  _variableSearchKey: "",
  _target: null,
  _prevSearchedFile: "",
  _prevSearchedLine: 0,
  _prevSearchedToken: ""
};




function FilteredSourcesView() {
  MenuContainer.call(this);
  this._onClick = this._onClick.bind(this);
}

create({ constructor: FilteredSourcesView, proto: MenuContainer.prototype }, {
  


  initialize: function DVFS_initialize() {
    dumpn("Initializing the FilteredSourcesView");

    let panel = this._panel = document.createElement("panel");
    panel.id = "filtered-sources-panel";
    panel.setAttribute("noautofocus", "true");
    panel.setAttribute("level", "top");
    panel.setAttribute("position", FILTERED_SOURCES_POPUP_POSITION);
    document.documentElement.appendChild(panel);

    this._searchbox = document.getElementById("searchbox");
    this.node = new StackList(panel);

    this.node.itemFactory = this._createItemView;
    this.node.itemType = "vbox";
    this.node.addEventListener("click", this._onClick, false);
  },

  


  destroy: function DVFS_destroy() {
    dumpn("Destroying the FilteredSourcesView");
    document.documentElement.removeChild(this._panel);
    this.node.removeEventListener("click", this._onClick, false);
  },

  



  set hidden(aFlag) {
    if (aFlag) {
      this.node._parent.hidePopup();
    } else {
      this.node._parent.openPopup(this._searchbox);
    }
  },

  


  syncFileSearch: function DVFS_syncFileSearch() {
    this.empty();

    
    
    if (!DebuggerView.Filtering.searchedFile ||
        !DebuggerView.Sources.visibleItems.length) {
      this.hidden = true;
      return;
    }

    
    let visibleItems = DebuggerView.Sources.visibleItems;
    let displayedItems = visibleItems.slice(0, FILTERED_SOURCES_MAX_RESULTS);

    for (let item of displayedItems) {
      
      let trimmedLabel = SourceUtils.trimUrlLength(item.label);
      let trimmedValue = SourceUtils.trimUrlLength(item.value);

      let locationItem = this.push([trimmedLabel, trimmedValue], {
        relaxed: true, 
        attachment: {
          fullLabel: item.label,
          fullValue: item.value
        }
      });

      let element = locationItem.target;
      element.className = "dbg-source-item list-item";
      element.labelNode.className = "dbg-source-item-name plain";
      element.valueNode.className = "dbg-source-item-details plain";
    }

    this._updateSelection(this.getItemAtIndex(0));
    this.hidden = false;
  },

  


  focusNext: function DVFS_focusNext() {
    let nextIndex = this.selectedIndex + 1;
    if (nextIndex >= this.itemCount) {
      nextIndex = 0;
    }
    this._updateSelection(this.getItemAtIndex(nextIndex));
  },

  


  focusPrev: function DVFS_focusPrev() {
    let prevIndex = this.selectedIndex - 1;
    if (prevIndex < 0) {
      prevIndex = this.itemCount - 1;
    }
    this._updateSelection(this.getItemAtIndex(prevIndex));
  },

  


  _onClick: function DVFS__onClick(e) {
    let locationItem = this.getItemForElement(e.target);
    if (locationItem) {
      this._updateSelection(locationItem);
    }
  },

  





  _updateSelection: function DVFS__updateSelection(aItem) {
    this.selectedItem = aItem;
    DebuggerView.Filtering._target.selectedValue = aItem.attachment.fullValue;
  },

  







  _createItemView: function DVFS__createItemView(aElementNode, aLabel, aValue) {
    let labelNode = document.createElement("label");
    let valueNode = document.createElement("label");

    labelNode.setAttribute("value", aLabel);
    valueNode.setAttribute("value", aValue);

    aElementNode.appendChild(labelNode);
    aElementNode.appendChild(valueNode);

    aElementNode.labelNode = labelNode;
    aElementNode.valueNode = valueNode;
  },

  _panel: null,
  _searchbox: null
});




DebuggerView.Toolbar = new ToolbarView();
DebuggerView.Options = new OptionsView();
DebuggerView.ChromeGlobals = new ChromeGlobalsView();
DebuggerView.Sources = new SourcesView();
DebuggerView.Filtering = new FilterView();
DebuggerView.FilteredSources = new FilteredSourcesView();
