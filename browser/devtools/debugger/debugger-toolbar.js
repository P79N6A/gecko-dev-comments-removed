




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

    this._instrumentsPaneToggleButton = document.getElementById("instruments-pane-toggle");
    this._resumeButton = document.getElementById("resume");
    this._stepOverButton = document.getElementById("step-over");
    this._stepInButton = document.getElementById("step-in");
    this._stepOutButton = document.getElementById("step-out");
    this._chromeGlobals = document.getElementById("chrome-globals");

    let resumeKey = LayoutHelpers.prettyKey(document.getElementById("resumeKey"), true);
    let stepOverKey = LayoutHelpers.prettyKey(document.getElementById("stepOverKey"), true);
    let stepInKey = LayoutHelpers.prettyKey(document.getElementById("stepInKey"), true);
    let stepOutKey = LayoutHelpers.prettyKey(document.getElementById("stepOutKey"), true);
    this._resumeTooltip = L10N.getFormatStr("resumeButtonTooltip", [resumeKey]);
    this._pauseTooltip = L10N.getFormatStr("pauseButtonTooltip", [resumeKey]);
    this._stepOverTooltip = L10N.getFormatStr("stepOverTooltip", [stepOverKey]);
    this._stepInTooltip = L10N.getFormatStr("stepInTooltip", [stepInKey]);
    this._stepOutTooltip = L10N.getFormatStr("stepOutTooltip", [stepOutKey]);

    this._instrumentsPaneToggleButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
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

    this._instrumentsPaneToggleButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
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

  


  _onTogglePanesPressed: function DVT__onTogglePanesPressed() {
    DebuggerView.toggleInstrumentsPane({
      visible: DebuggerView.instrumentsPaneHidden,
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

  _instrumentsPaneToggleButton: null,
  _resumeButton: null,
  _stepOverButton: null,
  _stepInButton: null,
  _stepOutButton: null,
  _chromeGlobals: null,
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

  this._onSelect = this._onSelect.bind(this);
  this._onClick = this._onClick.bind(this);
}

create({ constructor: ChromeGlobalsView, proto: MenuContainer.prototype }, {
  


  initialize: function DVCG_initialize() {
    dumpn("Initializing the ChromeGlobalsView");

    this.node = document.getElementById("chrome-globals");
    this.emptyText = L10N.getStr("noGlobalsText");
    this.unavailableText = L10N.getStr("noMatchingGlobalsText");

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




function StackFramesView() {
  dumpn("StackFramesView was instantiated");

  this._framesCache = new Map(); 
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
    commandset.id = "stackframesCommandset";
    menupopup.id = "stackframesMenupopup";

    document.getElementById("debuggerPopupset").appendChild(menupopup);
    document.getElementById("debuggerCommands").appendChild(commandset);

    this.node = new BreadcrumbsWidget(document.getElementById("stackframes"));
    this.node.addEventListener("mousedown", this._onClick, false);
    this.node.addEventListener("scroll", this._onScroll, true);
    window.addEventListener("resize", this._onScroll, true);
  },

  


  destroy: function DVSF_destroy() {
    dumpn("Destroying the StackFramesView");

    this.node.removeEventListener("mousedown", this._onClick, false);
    this.node.removeEventListener("scroll", this._onScroll, true);
    window.removeEventListener("resize", this._onScroll, true);
  },

  











  addFrame: function DVSF_addFrame(aFrameTitle, aSourceLocation, aLineNumber, aDepth) {
    
    let frameView = this._createFrameView.apply(this, arguments);
    let menuEntry = this._createMenuEntry.apply(this, arguments);

    
    let stackframeItem = this.push(frameView, {
      index: 0, 
      relaxed: true, 
      attachment: {
        popup: menuEntry,
        depth: aDepth
      },
      attributes: [
        ["contextmenu", "stackframesMenupopup"],
        ["tooltiptext", aSourceLocation]
      ],
      
      
      finalize: this._onStackframeRemoved
    });

    this._framesCache.set(aDepth, stackframeItem);
  },

  





  highlightFrame: function DVSF_highlightFrame(aDepth) {
    let selectedItem = this.selectedItem = this._framesCache.get(aDepth);

    for (let item in this) {
      if (item != selectedItem) {
        item.attachment.popup.menuitem.removeAttribute("checked");
      } else {
        item.attachment.popup.menuitem.setAttribute("checked", "");
      }
    }
  },

  


  dirty: false,

  













  _createFrameView:
  function DVSF__createFrameView(aFrameTitle, aSourceLocation, aLineNumber, aDepth) {
    let frameDetails = SourceUtils.getSourceLabel(aSourceLocation,
      STACK_FRAMES_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_SOURCE_URL_TRIM_SECTION) +
      SEARCH_LINE_FLAG + aLineNumber;

    let frameTitleNode = document.createElement("label");
    frameTitleNode.className = "plain dbg-stackframe-title breadcrumbs-widget-item-tag";
    frameTitleNode.setAttribute("value", aFrameTitle);

    let frameDetailsNode = document.createElement("label");
    frameDetailsNode.className = "plain dbg-stackframe-details breadcrumbs-widget-item-id";
    frameDetailsNode.setAttribute("value", frameDetails);

    let container = document.createElement("hbox");
    container.id = "stackframe-" + aDepth;
    container.className = "dbg-stackframe";

    container.appendChild(frameTitleNode);
    container.appendChild(frameDetailsNode);

    return container;
  },

  













  _createMenuEntry:
  function DVSF__createMenuEntry(aFrameTitle, aSourceLocation, aLineNumber, aDepth) {
    let frameDescription = SourceUtils.getSourceLabel(aSourceLocation,
      STACK_FRAMES_POPUP_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_POPUP_SOURCE_URL_TRIM_SECTION) +
      SEARCH_LINE_FLAG + aLineNumber;

    let prefix = "sf-cMenu-"; 
    let commandId = prefix + aDepth + "-" + "-command";
    let menuitemId = prefix + aDepth + "-" + "-menuitem";

    let command = document.createElement("command");
    command.id = commandId;
    command.addEventListener("command", this._selectFrame.bind(this, aDepth), false);

    let menuitem = document.createElement("menuitem");
    menuitem.id = menuitemId;
    menuitem.className = "dbg-stackframe-menuitem";
    menuitem.setAttribute("type", "checkbox");
    menuitem.setAttribute("command", commandId);
    menuitem.setAttribute("tooltiptext", aSourceLocation);

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

  





  _destroyMenuEntry: function DVSF__destroyMenuEntry(aMenuEntry) {
    dumpn("Destroying context menu: " +
      aMenuEntry.command.id + " & " + aMenuEntry.menuitem.id);

    let command = aMenuEntry.command;
    let menuitem = aMenuEntry.menuitem;
    command.parentNode.removeChild(command);
    menuitem.parentNode.removeChild(menuitem);
  },

  





  _onStackframeRemoved: function DVSF__onStackframeRemoved(aItem) {
    dumpn("Finalizing stackframe item: " + aItem);

    let { popup, depth } = aItem.attachment;
    this._destroyMenuEntry(popup);
    this._framesCache.delete(depth);
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

  _framesCache: null,
  _commandset: null,
  _menupopup: null,
  _scrollTimeout: null
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
    this._searchboxHelpPanel = document.getElementById("searchbox-help-panel");
    this._globalOperatorButton = document.getElementById("global-operator-button");
    this._globalOperatorLabel = document.getElementById("global-operator-label");
    this._functionOperatorButton = document.getElementById("function-operator-button");
    this._functionOperatorLabel = document.getElementById("function-operator-label");
    this._tokenOperatorButton = document.getElementById("token-operator-button");
    this._tokenOperatorLabel = document.getElementById("token-operator-label");
    this._lineOperatorButton = document.getElementById("line-operator-button");
    this._lineOperatorLabel = document.getElementById("line-operator-label");
    this._variableOperatorButton = document.getElementById("variable-operator-button");
    this._variableOperatorLabel = document.getElementById("variable-operator-label");

    this._fileSearchKey = LayoutHelpers.prettyKey(document.getElementById("fileSearchKey"), true);
    this._globalSearchKey = LayoutHelpers.prettyKey(document.getElementById("globalSearchKey"), true);
    this._filteredFunctionsKey = LayoutHelpers.prettyKey(document.getElementById("functionSearchKey"), true);
    this._tokenSearchKey = LayoutHelpers.prettyKey(document.getElementById("tokenSearchKey"), true);
    this._lineSearchKey = LayoutHelpers.prettyKey(document.getElementById("lineSearchKey"), true);
    this._variableSearchKey = LayoutHelpers.prettyKey(document.getElementById("variableSearchKey"), true);

    this._searchbox.addEventListener("click", this._onClick, false);
    this._searchbox.addEventListener("select", this._onSearch, false);
    this._searchbox.addEventListener("input", this._onSearch, false);
    this._searchbox.addEventListener("keypress", this._onKeyPress, false);
    this._searchbox.addEventListener("blur", this._onBlur, false);

    this._globalOperatorButton.setAttribute("label", SEARCH_GLOBAL_FLAG);
    this._functionOperatorButton.setAttribute("label", SEARCH_FUNCTION_FLAG);
    this._tokenOperatorButton.setAttribute("label", SEARCH_TOKEN_FLAG);
    this._lineOperatorButton.setAttribute("label", SEARCH_LINE_FLAG);
    this._variableOperatorButton.setAttribute("label", SEARCH_VARIABLE_FLAG);

    this._globalOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGlobal", [this._globalSearchKey]));
    this._functionOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelFunction", [this._filteredFunctionsKey]));
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

  



  get target() this._target,

  



  get searchboxInfo() {
    let operator, file, line, token;

    let rawValue = this._searchbox.value;
    let rawLength = rawValue.length;
    let globalFlagIndex = rawValue.indexOf(SEARCH_GLOBAL_FLAG);
    let functionFlagIndex = rawValue.indexOf(SEARCH_FUNCTION_FLAG);
    let variableFlagIndex = rawValue.indexOf(SEARCH_VARIABLE_FLAG);
    let lineFlagIndex = rawValue.lastIndexOf(SEARCH_LINE_FLAG);
    let tokenFlagIndex = rawValue.lastIndexOf(SEARCH_TOKEN_FLAG);

    
    if (globalFlagIndex != 0 && functionFlagIndex != 0 && variableFlagIndex != 0) {
      let fileEnd = lineFlagIndex != -1
        ? lineFlagIndex
        : tokenFlagIndex != -1 ? tokenFlagIndex : rawLength;

      let lineEnd = tokenFlagIndex != -1
        ? tokenFlagIndex
        : rawLength;

      operator = "";
      file = rawValue.slice(0, fileEnd);
      line = ~~(rawValue.slice(fileEnd + 1, lineEnd)) || 0;
      token = rawValue.slice(lineEnd + 1);
    }
    
    else if (globalFlagIndex == 0) {
      operator = SEARCH_GLOBAL_FLAG;
      file = "";
      line = 0;
      token = rawValue.slice(1);
    }
    
    else if (functionFlagIndex == 0) {
      operator = SEARCH_FUNCTION_FLAG;
      file = "";
      line = 0;
      token = rawValue.slice(1);
    }
    
    else if (variableFlagIndex == 0) {
      operator = SEARCH_VARIABLE_FLAG;
      file = "";
      line = 0;
      token = rawValue.slice(1);
    }

    return [operator, file, line, token];
  },

  



  get currentOperator() this.searchboxInfo[0],

  



  get searchedFile() this.searchboxInfo[1],

  



  get searchedLine() this.searchboxInfo[2],

  



  get searchedToken() this.searchboxInfo[3],

  


  clearSearch: function DVF_clearSearch() {
    this._searchbox.value = "";
    this._searchboxHelpPanel.hidePopup();
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

    
    view.node.hideEmptyGroups();

    
    view.node.ensureSelectionIsVisible(true);

    
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
    this._searchboxHelpPanel.openPopup(this._searchbox);
  },

  


  _onSearch: function DVF__onScriptsSearch() {
    this._searchboxHelpPanel.hidePopup();
    let [operator, file, line, token] = this.searchboxInfo;

    
    
    if (operator == SEARCH_GLOBAL_FLAG) {
      DebuggerView.GlobalSearch.scheduleSearch(token);
      this._prevSearchedToken = token;
      return;
    }

    
    
    if (operator == SEARCH_FUNCTION_FLAG) {
      DebuggerView.FilteredFunctions.scheduleSearch(token);
      this._prevSearchedToken = token;
      return;
    }

    
    
    if (operator == SEARCH_VARIABLE_FLAG) {
      DebuggerView.Variables.scheduleSearch(token);
      this._prevSearchedToken = token;
      return;
    }

    DebuggerView.GlobalSearch.clearView();
    DebuggerView.FilteredFunctions.clearView();

    this._performFileSearch(file);
    this._performLineSearch(line);
    this._performTokenSearch(token);
  },

  


  _onKeyPress: function DVF__onScriptsKeyPress(e) {
    
    e.char = String.fromCharCode(e.charCode);

    let [operator, file, line, token] = this.searchboxInfo;
    let isGlobal = operator == SEARCH_GLOBAL_FLAG;
    let isFunction = operator == SEARCH_FUNCTION_FLAG;
    let isVariable = operator == SEARCH_VARIABLE_FLAG;
    let action = -1;

    if (file && !line && !token) {
      var isFileSearch = true;
    }
    if (line && !token) {
      var isLineSearch = true;
    }
    if (this._prevSearchedToken != token) {
      var isDifferentToken = true;
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
        var isReturnKey = true;
        
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
    if (action == -1 || (!operator && !file && !line && !token)) {
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    
    if (isFileSearch) {
      if (isReturnKey) {
        DebuggerView.FilteredSources.clearView();
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

    
    if (isFunction) {
      if (isReturnKey && (isDifferentToken || DebuggerView.FilteredFunctions.hidden)) {
        DebuggerView.FilteredFunctions.performSearch(token);
      } else if (!isReturnKey) {
        DebuggerView.FilteredFunctions[["focusNext", "focusPrev"][action]]();
      } else {
        DebuggerView.FilteredFunctions.clearView();
        DebuggerView.editor.focus();
        this.clearSearch();
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
    DebuggerView.FilteredSources.clearView();
    DebuggerView.FilteredFunctions.clearView();
    DebuggerView.Variables.performSearch(null);
    this._searchboxHelpPanel.hidePopup();
  },

  





  _doSearch: function DVF__doSearch(aOperator = "") {
    this._searchbox.focus();
    this._searchbox.value = ""; 
    this._searchbox.value = aOperator;
  },

  


  _doFileSearch: function DVF__doFileSearch() {
    this._doSearch();
    this._searchboxHelpPanel.openPopup(this._searchbox);
  },

  


  _doGlobalSearch: function DVF__doGlobalSearch() {
    this._doSearch(SEARCH_GLOBAL_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doFunctionSearch: function DVF__doFunctionSearch() {
    this._doSearch(SEARCH_FUNCTION_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doTokenSearch: function DVF__doTokenSearch() {
    this._doSearch(SEARCH_TOKEN_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doLineSearch: function DVF__doLineSearch() {
    this._doSearch(SEARCH_LINE_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doVariableSearch: function DVF__doVariableSearch() {
    DebuggerView.Variables.performSearch("");
    this._doSearch(SEARCH_VARIABLE_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doVariablesFocus: function DVG__doVariablesFocus() {
    DebuggerView.showInstrumentsPane();
    DebuggerView.Variables.focusFirstVisibleNode();
  },

  _searchbox: null,
  _searchboxHelpPanel: null,
  _globalOperatorButton: null,
  _globalOperatorLabel: null,
  _functionOperatorButton: null,
  _functionOperatorLabel: null,
  _tokenOperatorButton: null,
  _tokenOperatorLabel: null,
  _lineOperatorButton: null,
  _lineOperatorLabel: null,
  _variableOperatorButton: null,
  _variableOperatorLabel: null,
  _fileSearchKey: "",
  _globalSearchKey: "",
  _filteredFunctionsKey: "",
  _tokenSearchKey: "",
  _lineSearchKey: "",
  _variableSearchKey: "",
  _target: null,
  _prevSearchedFile: "",
  _prevSearchedLine: 0,
  _prevSearchedToken: ""
};




function FilteredSourcesView() {
  dumpn("FilteredSourcesView was instantiated");
  ResultsPanelContainer.call(this);

  this.onClick = this.onClick.bind(this);
  this.onSelect = this.onSelect.bind(this);
}

create({ constructor: FilteredSourcesView, proto: ResultsPanelContainer.prototype }, {
  


  initialize: function DVFS_initialize() {
    dumpn("Initializing the FilteredSourcesView");

    this.anchor = document.getElementById("searchbox");
  },

  


  destroy: function DVFS_destroy() {
    dumpn("Destroying the FilteredSourcesView");

    this.anchor = null;
  },

  


  syncFileSearch: function DVFS_syncFileSearch() {
    this.empty();

    
    
    if (!DebuggerView.Filtering.searchedFile ||
        !DebuggerView.Sources.visibleItems.length) {
      this.hidden = true;
      return;
    }

    
    let visibleItems = DebuggerView.Sources.visibleItems;
    let displayedItems = visibleItems.slice(0, RESULTS_PANEL_MAX_RESULTS);

    for (let item of displayedItems) {
      
      let trimmedLabel = SourceUtils.trimUrlLength(item.label);
      let trimmedValue = SourceUtils.trimUrlLength(item.value, 0, "start");
      let locationItem = this.push([trimmedLabel, trimmedValue], {
        relaxed: true, 
        attachment: {
          fullLabel: item.label,
          fullValue: item.value
        }
      });
    }

    
    this.select(0);

    
    this.hidden = this.itemCount == 0;
  },

  


  onClick: function DVFS_onClick(e) {
    let locationItem = this.getItemForElement(e.target);
    if (locationItem) {
      this.select(locationItem);
      DebuggerView.Filtering.clearSearch();
    }
  },

  





  onSelect: function DVFS_onSelect(e) {
    let locationItem = this.getItemForElement(e.target);
    if (locationItem) {
      DebuggerView.Sources.selectedValue = locationItem.attachment.fullValue;
    }
  }
});




function FilteredFunctionsView() {
  dumpn("FilteredFunctionsView was instantiated");
  ResultsPanelContainer.call(this);

  this._performFunctionSearch = this._performFunctionSearch.bind(this);
  this.onClick = this.onClick.bind(this);
  this.onSelect = this.onSelect.bind(this);
}

create({ constructor: FilteredFunctionsView, proto: ResultsPanelContainer.prototype }, {
  


  initialize: function DVFF_initialize() {
    dumpn("Initializing the FilteredFunctionsView");

    this.anchor = document.getElementById("searchbox");
  },

  


  destroy: function DVFF_destroy() {
    dumpn("Destroying the FilteredFunctionsView");

    this.anchor = null;
  },

  


  delayedSearch: true,

  





  scheduleSearch: function DVFF_scheduleSearch(aQuery) {
    if (!this.delayedSearch) {
      this.performSearch(aQuery);
      return;
    }
    let delay = Math.max(FUNCTION_SEARCH_ACTION_MAX_DELAY / aQuery.length, 0);

    window.clearTimeout(this._searchTimeout);
    this._searchFunction = this._startSearch.bind(this, aQuery);
    this._searchTimeout = window.setTimeout(this._searchFunction, delay);
  },

  





  performSearch: function DVFF_performSearch(aQuery) {
    window.clearTimeout(this._searchTimeout);
    this._searchFunction = null;
    this._startSearch(aQuery);
  },

  





  _startSearch: function DVFF__startSearch(aQuery) {
    this._searchedToken = aQuery;

    DebuggerController.SourceScripts.fetchSources(DebuggerView.Sources.values, {
      onFinished: this._performFunctionSearch
    });
  },

  



  _performFunctionSearch: function DVFF__performFunctionSearch() {
    
    
    
    let token = this._searchedToken;

    
    
    let sourcesCache = DebuggerController.SourceScripts.getCache();
    let currentUrl = DebuggerView.Sources.selectedValue;
    sourcesCache.sort(function([sourceUrl]) sourceUrl == currentUrl ? -1 : 1);

    
    if (!token) {
      sourcesCache.splice(1);
    }

    
    let searchResults = [];

    for (let [location, contents] of sourcesCache) {
      let parserMethods = DebuggerController.Parser.get(location, contents);
      let sourceResults = parserMethods.getNamedFunctionDefinitions(token);

      for (let scriptResult of sourceResults) {
        for (let parseResult of scriptResult.parseResults) {
          searchResults.push({
            sourceUrl: scriptResult.sourceUrl,
            scriptOffset: scriptResult.scriptOffset,
            functionName: parseResult.functionName,
            functionLocation: parseResult.functionLocation,
            inferredName: parseResult.inferredName,
            inferredChain: parseResult.inferredChain,
            inferredLocation: parseResult.inferredLocation
          });

          
          
          if (searchResults.length >= RESULTS_PANEL_MAX_RESULTS) {
            this._syncFunctionSearch(searchResults);
            return;
          }
        }
      }
    }
    
    
    this._syncFunctionSearch(searchResults);
  },

  





  _syncFunctionSearch: function DVFF__syncFunctionSearch(aSearchResults) {
    this.empty();

    
    
    if (!aSearchResults.length) {
      this.hidden = true;
      return;
    }

    for (let item of aSearchResults) {
      
      
      
      if (item.functionName && item.inferredName &&
          item.functionName != item.inferredName) {
        let s = " " + L10N.getStr("functionSearchSeparatorLabel") + " ";
        item.displayedName = item.inferredName + s + item.functionName;
      }
      
      else if (item.inferredName) {
        item.displayedName = item.inferredName;
      }
      
      else {
        item.displayedName = item.functionName;
      }

      
      
      if (item.inferredLocation) {
        item.actualLocation = item.inferredLocation;
      } else {
        item.actualLocation = item.functionLocation;
      }

      
      let trimmedLabel = SourceUtils.trimUrlLength(item.displayedName + "()");
      let trimmedValue = SourceUtils.trimUrlLength(item.sourceUrl, 0, "start");
      let description = (item.inferredChain || []).join(".");

      let functionItem = this.push([trimmedLabel, trimmedValue, description], {
        index: -1, 
        relaxed: true, 
        attachment: item
      });
    }

    
    this.select(0);
    this.hidden = this.itemCount == 0;
  },

  


  onClick: function DVFF_onClick(e) {
    let functionItem = this.getItemForElement(e.target);
    if (functionItem) {
      this.select(functionItem);
      DebuggerView.Filtering.clearSearch();
    }
  },

  


  onSelect: function DVFF_onSelect(e) {
    let functionItem = this.getItemForElement(e.target);
    if (functionItem) {
      let sourceUrl = functionItem.attachment.sourceUrl;
      let scriptOffset = functionItem.attachment.scriptOffset;
      let actualLocation = functionItem.attachment.actualLocation;

      DebuggerView.updateEditor(sourceUrl, actualLocation.start.line, {
        charOffset: scriptOffset,
        columnOffset: actualLocation.start.column,
        noDebug: true
      });
    }
  },

  _searchTimeout: null,
  _searchFunction: null,
  _searchedToken: ""
});




DebuggerView.Toolbar = new ToolbarView();
DebuggerView.Options = new OptionsView();
DebuggerView.Filtering = new FilterView();
DebuggerView.FilteredSources = new FilteredSourcesView();
DebuggerView.FilteredFunctions = new FilteredFunctionsView();
DebuggerView.ChromeGlobals = new ChromeGlobalsView();
DebuggerView.StackFrames = new StackFramesView();
