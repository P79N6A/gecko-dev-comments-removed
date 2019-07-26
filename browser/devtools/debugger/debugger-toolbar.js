




"use strict";



const POPUP_HIDDEN_DELAY = 100; 





function ToolbarView() {
  dumpn("ToolbarView was instantiated");

  this._onTogglePanesPressed = this._onTogglePanesPressed.bind(this);
  this._onResumePressed = this._onResumePressed.bind(this);
  this._onStepOverPressed = this._onStepOverPressed.bind(this);
  this._onStepInPressed = this._onStepInPressed.bind(this);
  this._onStepOutPressed = this._onStepOutPressed.bind(this);
}

ToolbarView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the ToolbarView");

    this._instrumentsPaneToggleButton = document.getElementById("instruments-pane-toggle");
    this._resumeOrderPanel = document.getElementById("resumption-order-panel");
    this._resumeButton = document.getElementById("resume");
    this._stepOverButton = document.getElementById("step-over");
    this._stepInButton = document.getElementById("step-in");
    this._stepOutButton = document.getElementById("step-out");
    this._chromeGlobals = document.getElementById("chrome-globals");

    let resumeKey = DevtoolsHelpers.prettyKey(document.getElementById("resumeKey"), true);
    let stepOverKey = DevtoolsHelpers.prettyKey(document.getElementById("stepOverKey"), true);
    let stepInKey = DevtoolsHelpers.prettyKey(document.getElementById("stepInKey"), true);
    let stepOutKey = DevtoolsHelpers.prettyKey(document.getElementById("stepOutKey"), true);
    this._resumeTooltip = L10N.getFormatStr("resumeButtonTooltip", resumeKey);
    this._pauseTooltip = L10N.getFormatStr("pauseButtonTooltip", resumeKey);
    this._stepOverTooltip = L10N.getFormatStr("stepOverTooltip", stepOverKey);
    this._stepInTooltip = L10N.getFormatStr("stepInTooltip", stepInKey);
    this._stepOutTooltip = L10N.getFormatStr("stepOutTooltip", stepOutKey);

    this._instrumentsPaneToggleButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.addEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.addEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.addEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.addEventListener("mousedown", this._onStepOutPressed, false);

    this._stepOverButton.setAttribute("tooltiptext", this._stepOverTooltip);
    this._stepInButton.setAttribute("tooltiptext", this._stepInTooltip);
    this._stepOutButton.setAttribute("tooltiptext", this._stepOutTooltip);

    
    
  },

  


  destroy: function() {
    dumpn("Destroying the ToolbarView");

    this._instrumentsPaneToggleButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.removeEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.removeEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.removeEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.removeEventListener("mousedown", this._onStepOutPressed, false);
  },

  






  showResumeWarning: function(aPausedUrl) {
    let label = L10N.getFormatStr("resumptionOrderPanelTitle", aPausedUrl);
    let descriptionNode = document.getElementById("resumption-panel-desc");
    descriptionNode.setAttribute("value", label);

    this._resumeOrderPanel.openPopup(this._resumeButton);
  },

  





  toggleResumeButtonState: function(aState) {
    
    if (aState == "paused") {
      this._resumeButton.setAttribute("checked", "true");
      this._resumeButton.setAttribute("tooltiptext", this._resumeTooltip);
    }
    
    else if (aState == "attached") {
      this._resumeButton.removeAttribute("checked");
      this._resumeButton.setAttribute("tooltiptext", this._pauseTooltip);
    }
  },

  





  toggleChromeGlobalsContainer: function(aVisibleFlag) {
    this._chromeGlobals.setAttribute("hidden", !aVisibleFlag);
  },

  


  _onTogglePanesPressed: function() {
    DebuggerView.toggleInstrumentsPane({
      visible: DebuggerView.instrumentsPaneHidden,
      animated: true,
      delayed: true
    });
  },

  


  _onResumePressed: function() {
    if (DebuggerController.activeThread.paused) {
      let warn = DebuggerController._ensureResumptionOrder;
      DebuggerController.activeThread.resume(warn);
    } else {
      DebuggerController.activeThread.interrupt();
    }
  },

  


  _onStepOverPressed: function() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepOver();
    }
  },

  


  _onStepInPressed: function() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepIn();
    }
  },

  


  _onStepOutPressed: function() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.stepOut();
    }
  },

  _instrumentsPaneToggleButton: null,
  _resumeOrderPanel: null,
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
  this._toggleIgnoreCaughtExceptions = this._toggleIgnoreCaughtExceptions.bind(this);
  this._toggleShowPanesOnStartup = this._toggleShowPanesOnStartup.bind(this);
  this._toggleShowVariablesOnlyEnum = this._toggleShowVariablesOnlyEnum.bind(this);
  this._toggleShowVariablesFilterBox = this._toggleShowVariablesFilterBox.bind(this);
  this._toggleShowOriginalSource = this._toggleShowOriginalSource.bind(this);
}

OptionsView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the OptionsView");

    this._button = document.getElementById("debugger-options");
    this._pauseOnExceptionsItem = document.getElementById("pause-on-exceptions");
    this._ignoreCaughtExceptionsItem = document.getElementById("ignore-caught-exceptions");
    this._showPanesOnStartupItem = document.getElementById("show-panes-on-startup");
    this._showVariablesOnlyEnumItem = document.getElementById("show-vars-only-enum");
    this._showVariablesFilterBoxItem = document.getElementById("show-vars-filter-box");
    this._showOriginalSourceItem = document.getElementById("show-original-source");

    this._pauseOnExceptionsItem.setAttribute("checked", Prefs.pauseOnExceptions);
    this._ignoreCaughtExceptionsItem.setAttribute("checked", Prefs.ignoreCaughtExceptions);
    this._showPanesOnStartupItem.setAttribute("checked", Prefs.panesVisibleOnStartup);
    this._showVariablesOnlyEnumItem.setAttribute("checked", Prefs.variablesOnlyEnumVisible);
    this._showVariablesFilterBoxItem.setAttribute("checked", Prefs.variablesSearchboxVisible);
    this._showOriginalSourceItem.setAttribute("checked", Prefs.sourceMapsEnabled);
  },

  


  destroy: function() {
    dumpn("Destroying the OptionsView");
    
  },

  


  _onPopupShowing: function() {
    this._button.setAttribute("open", "true");
  },

  


  _onPopupHiding: function() {
    this._button.removeAttribute("open");
  },

  


  _onPopupHidden: function() {
    window.dispatchEvent(document, "Debugger:OptionsPopupHidden");
  },

  


  _togglePauseOnExceptions: function() {
    Prefs.pauseOnExceptions =
      this._pauseOnExceptionsItem.getAttribute("checked") == "true";

    DebuggerController.activeThread.pauseOnExceptions(
      Prefs.pauseOnExceptions,
      Prefs.ignoreCaughtExceptions);
  },

  _toggleIgnoreCaughtExceptions: function() {
    Prefs.ignoreCaughtExceptions =
      this._ignoreCaughtExceptionsItem.getAttribute("checked") == "true";

    DebuggerController.activeThread.pauseOnExceptions(
      Prefs.pauseOnExceptions,
      Prefs.ignoreCaughtExceptions);
  },

  


  _toggleShowPanesOnStartup: function() {
    Prefs.panesVisibleOnStartup =
      this._showPanesOnStartupItem.getAttribute("checked") == "true";
  },

  


  _toggleShowVariablesOnlyEnum: function() {
    let pref = Prefs.variablesOnlyEnumVisible =
      this._showVariablesOnlyEnumItem.getAttribute("checked") == "true";

    DebuggerView.Variables.onlyEnumVisible = pref;
  },

  


  _toggleShowVariablesFilterBox: function() {
    let pref = Prefs.variablesSearchboxVisible =
      this._showVariablesFilterBoxItem.getAttribute("checked") == "true";

    DebuggerView.Variables.searchEnabled = pref;
  },

  


  _toggleShowOriginalSource: function() {
    let pref = Prefs.sourceMapsEnabled =
      this._showOriginalSourceItem.getAttribute("checked") == "true";

    
    window.addEventListener("Debugger:OptionsPopupHidden", function onHidden() {
      window.removeEventListener("Debugger:OptionsPopupHidden", onHidden, false);

      
      window.setTimeout(() => {
        DebuggerController.reconfigureThread(pref);
      }, POPUP_HIDDEN_DELAY);
    }, false);
  },

  _button: null,
  _pauseOnExceptionsItem: null,
  _showPanesOnStartupItem: null,
  _showVariablesOnlyEnumItem: null,
  _showVariablesFilterBoxItem: null,
  _showOriginalSourceItem: null
};




function ChromeGlobalsView() {
  dumpn("ChromeGlobalsView was instantiated");

  this._onSelect = this._onSelect.bind(this);
  this._onClick = this._onClick.bind(this);
}

ChromeGlobalsView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the ChromeGlobalsView");

    this.widget = document.getElementById("chrome-globals");
    this.emptyText = L10N.getStr("noGlobalsText");

    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("click", this._onClick, false);

    
    this.empty();
  },

  


  destroy: function() {
    dumpn("Destroying the ChromeGlobalsView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("click", this._onClick, false);
  },

  


  _onSelect: function() {
    
  },

  


  _onClick: function() {
    
    DebuggerView.Filtering.target = this;
  }
});




function StackFramesView() {
  dumpn("StackFramesView was instantiated");

  this._onStackframeRemoved = this._onStackframeRemoved.bind(this);
  this._onSelect = this._onSelect.bind(this);
  this._onScroll = this._onScroll.bind(this);
  this._afterScroll = this._afterScroll.bind(this);
}

StackFramesView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the StackFramesView");

    let commandset = this._commandset = document.createElement("commandset");
    let menupopup = this._menupopup = document.createElement("menupopup");
    commandset.id = "stackframesCommandset";
    menupopup.id = "stackframesMenupopup";

    document.getElementById("debuggerPopupset").appendChild(menupopup);
    document.getElementById("debuggerCommands").appendChild(commandset);

    this.widget = new BreadcrumbsWidget(document.getElementById("stackframes"));
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("scroll", this._onScroll, true);
    window.addEventListener("resize", this._onScroll, true);

    this.autoFocusOnFirstItem = false;
    this.autoFocusOnSelection = false;
  },

  


  destroy: function() {
    dumpn("Destroying the StackFramesView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("scroll", this._onScroll, true);
    window.removeEventListener("resize", this._onScroll, true);
  },

  













  addFrame: function(aTitle, aUrl, aLine, aDepth, aIsBlackBoxed) {
    
    
    if (aIsBlackBoxed) {
      if (this._prevBlackBoxedUrl == aUrl) {
        return;
      }
      this._prevBlackBoxedUrl = aUrl;
    } else {
      this._prevBlackBoxedUrl = null;
    }

    
    let frameView = this._createFrameView.apply(this, arguments);
    let menuEntry = this._createMenuEntry.apply(this, arguments);

    
    this.push([frameView, aTitle, aUrl], {
      index: 0, 
      attachment: {
        popup: menuEntry,
        depth: aDepth
      },
      attributes: [
        ["contextmenu", "stackframesMenupopup"]
      ],
      
      
      finalize: this._onStackframeRemoved
    });
  },

  



  set selectedDepth(aDepth) {
    this.selectedItem = aItem => aItem.attachment.depth == aDepth;
  },

  


  dirty: false,

  















  _createFrameView: function(aTitle, aUrl, aLine, aDepth, aIsBlackBoxed) {
    let container = document.createElement("hbox");
    container.id = "stackframe-" + aDepth;
    container.className = "dbg-stackframe";

    let frameDetails = SourceUtils.trimUrlLength(
      SourceUtils.getSourceLabel(aUrl),
      STACK_FRAMES_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_SOURCE_URL_TRIM_SECTION);

    if (aIsBlackBoxed) {
      container.classList.add("dbg-stackframe-black-boxed");
    } else {
      let frameTitleNode = document.createElement("label");
      frameTitleNode.className = "plain dbg-stackframe-title breadcrumbs-widget-item-tag";
      frameTitleNode.setAttribute("value", aTitle);
      container.appendChild(frameTitleNode);

      frameDetails += SEARCH_LINE_FLAG + aLine;
    }

    let frameDetailsNode = document.createElement("label");
    frameDetailsNode.className = "plain dbg-stackframe-details breadcrumbs-widget-item-id";
    frameDetailsNode.setAttribute("value", frameDetails);
    container.appendChild(frameDetailsNode);

    return container;
  },

  















  _createMenuEntry: function(aTitle, aUrl, aLine, aDepth, aIsBlackBoxed) {
    let frameDescription = SourceUtils.trimUrlLength(
      SourceUtils.getSourceLabel(aUrl),
      STACK_FRAMES_POPUP_SOURCE_URL_MAX_LENGTH,
      STACK_FRAMES_POPUP_SOURCE_URL_TRIM_SECTION) +
      SEARCH_LINE_FLAG + aLine;

    let prefix = "sf-cMenu-"; 
    let commandId = prefix + aDepth + "-" + "-command";
    let menuitemId = prefix + aDepth + "-" + "-menuitem";

    let command = document.createElement("command");
    command.id = commandId;
    command.addEventListener("command", () => this.selectedDepth = aDepth, false);

    let menuitem = document.createElement("menuitem");
    menuitem.id = menuitemId;
    menuitem.className = "dbg-stackframe-menuitem";
    menuitem.setAttribute("type", "checkbox");
    menuitem.setAttribute("command", commandId);
    menuitem.setAttribute("tooltiptext", aUrl);

    let labelNode = document.createElement("label");
    labelNode.className = "plain dbg-stackframe-menuitem-title";
    labelNode.setAttribute("value", aTitle);
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

  





  _onStackframeRemoved: function(aItem) {
    dumpn("Finalizing stackframe item: " + aItem);

    
    let contextItem = aItem.attachment.popup;
    contextItem.command.remove();
    contextItem.menuitem.remove();

    
    this._prevBlackBoxedUrl = null;
  },

  


  _onSelect: function(e) {
    let stackframeItem = this.selectedItem;
    if (stackframeItem) {
      
      DebuggerController.StackFrames.selectFrame(stackframeItem.attachment.depth);

      
      
      for (let otherItem in this) {
        if (otherItem != stackframeItem) {
          otherItem.attachment.popup.menuitem.removeAttribute("checked");
        } else {
          otherItem.attachment.popup.menuitem.setAttribute("checked", "");
        }
      }
    }
  },

  


  _onScroll: function() {
    
    if (!this.dirty) {
      return;
    }
    
    setNamedTimeout("stack-scroll", STACK_FRAMES_SCROLL_DELAY, this._afterScroll);
  },

  


  _afterScroll: function() {
    
    
    let list = this.widget._list;
    let scrollPosition = list.scrollPosition;
    let scrollWidth = list.scrollWidth;

    
    
    if (scrollPosition - scrollWidth / 10 < 1) {
      list.ensureElementIsVisible(this.getItemAtIndex(CALL_STACK_PAGE_SIZE - 1).target);
      this.dirty = false;

      
      DebuggerController.StackFrames.addMoreFrames();
    }
  },

  _commandset: null,
  _menupopup: null,
  _prevBlackBoxedUrl: null
});




let StackFrameUtils = {
  






  getFrameTitle: function(aFrame) {
    if (aFrame.type == "call") {
      let c = aFrame.callee;
      return (c.userDisplayName || c.displayName || c.name || "(anonymous)");
    }
    return "(" + aFrame.type + ")";
  },

  







  getScopeLabel: function(aEnv) {
    let name = "";

    
    if (!aEnv.parent) {
      name = L10N.getStr("globalScopeLabel");
    }
    
    else {
      name = aEnv.type.charAt(0).toUpperCase() + aEnv.type.slice(1);
    }

    let label = L10N.getFormatStr("scopeLabel", name);
    switch (aEnv.type) {
      case "with":
      case "object":
        label += " [" + aEnv.object.class + "]";
        break;
      case "function":
        let f = aEnv.function;
        label += " [" +
          (f.userDisplayName || f.displayName || f.name || "(anonymous)") +
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
  


  initialize: function() {
    dumpn("Initializing the FilterView");

    this._searchbox = document.getElementById("searchbox");
    this._searchboxHelpPanel = document.getElementById("searchbox-help-panel");
    this._filterLabel = document.getElementById("filter-label");
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

    this._fileSearchKey = DevtoolsHelpers.prettyKey(document.getElementById("fileSearchKey"), true);
    this._globalSearchKey = DevtoolsHelpers.prettyKey(document.getElementById("globalSearchKey"), true);
    this._filteredFunctionsKey = DevtoolsHelpers.prettyKey(document.getElementById("functionSearchKey"), true);
    this._tokenSearchKey = DevtoolsHelpers.prettyKey(document.getElementById("tokenSearchKey"), true);
    this._lineSearchKey = DevtoolsHelpers.prettyKey(document.getElementById("lineSearchKey"), true);
    this._variableSearchKey = DevtoolsHelpers.prettyKey(document.getElementById("variableSearchKey"), true);

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

    this._filterLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelFilter", this._fileSearchKey));
    this._globalOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGlobal", this._globalSearchKey));
    this._functionOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelFunction", this._filteredFunctionsKey));
    this._tokenOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelToken", this._tokenSearchKey));
    this._lineOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGoToLine", this._lineSearchKey));
    this._variableOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelVariable", this._variableSearchKey));

    
    
    
    
    this.target = DebuggerView.Sources;
    
  },

  


  destroy: function() {
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
        placeholder = L10N.getFormatStr("emptyChromeGlobalsFilterText", this._fileSearchKey);
        break;
      case DebuggerView.Sources:
        placeholder = L10N.getFormatStr("emptySearchText", this._fileSearchKey);
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
        : tokenFlagIndex != -1
          ? tokenFlagIndex
          : rawLength;

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

  


  clearSearch: function() {
    this._searchbox.value = "";
    this._searchboxHelpPanel.hidePopup();
  },

  

  },

  






  _performLineSearch: function(aLine) {
    
    if (this._prevSearchedLine != aLine && aLine) {
      DebuggerView.editor.setCaretPosition(aLine - 1);
    }
    
    if (this._prevSearchedToken && !aLine) {
      this._target.refresh();
    }

    
    this._prevSearchedLine = aLine;
  },

  






  _performTokenSearch: function(aToken) {
    
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
  },

  


  _onClick: function() {
    this._searchboxHelpPanel.openPopup(this._searchbox);
  },

  


  _onSearch: function() {
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

  


  _onKeyPress: function(e) {
    
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
        DebuggerView.FilteredSources[["selectNext", "selectPrev"][action]]();
      }
      this._prevSearchedFile = file;
      return;
    }

    
    if (isGlobal) {
      if (isReturnKey && (isDifferentToken || DebuggerView.GlobalSearch.hidden)) {
        DebuggerView.GlobalSearch.scheduleSearch(token, 0);
      } else {
        DebuggerView.GlobalSearch[["selectNext", "selectPrev"][action]]();
      }
      this._prevSearchedToken = token;
      return;
    }

    
    if (isFunction) {
      if (isReturnKey && (isDifferentToken || DebuggerView.FilteredFunctions.hidden)) {
        DebuggerView.FilteredFunctions.scheduleSearch(token, 0);
      } else if (!isReturnKey) {
        DebuggerView.FilteredFunctions[["selectNext", "selectPrev"][action]]();
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
        DebuggerView.Variables.scheduleSearch(token, 0);
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

  


  _onBlur: function() {
    DebuggerView.GlobalSearch.clearView();
    DebuggerView.FilteredSources.clearView();
    DebuggerView.FilteredFunctions.clearView();
    DebuggerView.Variables.scheduleSearch(null, 0);
    this._searchboxHelpPanel.hidePopup();
  },

  





  _doSearch: function(aOperator = "") {
    this._searchbox.focus();
    this._searchbox.value = ""; 
    this._searchbox.value = aOperator + DebuggerView.editor.getSelectedText();
  },

  


  _doFileSearch: function() {
    this._doSearch();
    this._searchboxHelpPanel.openPopup(this._searchbox);
  },

  


  _doGlobalSearch: function() {
    this._doSearch(SEARCH_GLOBAL_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doFunctionSearch: function() {
    this._doSearch(SEARCH_FUNCTION_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doTokenSearch: function() {
    this._doSearch(SEARCH_TOKEN_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doLineSearch: function() {
    this._doSearch(SEARCH_LINE_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doVariableSearch: function() {
    DebuggerView.Variables.scheduleSearch("", 0);
    this._doSearch(SEARCH_VARIABLE_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doVariablesFocus: function() {
    DebuggerView.showInstrumentsPane();
    DebuggerView.Variables.focusFirstVisibleItem();
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

  this._onClick = this._onClick.bind(this);
  this._onSelect = this._onSelect.bind(this);
}

FilteredSourcesView.prototype = Heritage.extend(ResultsPanelContainer.prototype, {
  


  initialize: function() {
    dumpn("Initializing the FilteredSourcesView");

    this.anchor = document.getElementById("searchbox");
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the FilteredSourcesView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("click", this._onClick, false);
    this.anchor = null;
  },

  







  scheduleSearch: function(aToken, aWait) {
    
    let maxDelay = FILE_SEARCH_ACTION_MAX_DELAY;
    let delay = aWait === undefined ? maxDelay / aToken.length : aWait;

    
    setNamedTimeout("sources-search", delay, () => this._doSearch(aToken));
  },

  





  _doSearch: function(aToken, aStore = []) {
    
    
    
    if (!aToken) {
      return;
    }

    for (let item of DebuggerView.Sources.items) {
      let lowerCaseLabel = item.label.toLowerCase();
      let lowerCaseToken = aToken.toLowerCase();
      if (lowerCaseLabel.match(lowerCaseToken)) {
        aStore.push(item);
      }

      
      
      if (aStore.length >= RESULTS_PANEL_MAX_RESULTS) {
        this._syncView(aStore);
        return;
      }
    }

    
    
    this._syncView(aStore);
  },

  





  _syncView: function(aSearchResults) {
    
    
    if (!aSearchResults.length) {
      return;
    }

    for (let item of aSearchResults) {
      
      let trimmedLabel = SourceUtils.trimUrlLength(item.label);
      let trimmedValue = SourceUtils.trimUrlLength(item.value, 0, "start");

      this.push([trimmedLabel, trimmedValue], {
        index: -1, 
        relaxed: true, 
        attachment: {
          url: item.value
        }
      });
    }

    
    this.selectedIndex = 0;
    this.hidden = false;
  },

  


  _onClick: function(e) {
    let locationItem = this.getItemForElement(e.target);
    if (locationItem) {
      this.selectedItem = locationItem;
      DebuggerView.Filtering.clearSearch();
    }
  },

  





  _onSelect: function({ detail: locationItem }) {
    if (locationItem) {
      let targetUrl = locationItem.attachment.url;
      let currentLine = DebuggerView.editor.getCaretPosition().line + 1;

      
      
      if (DebuggerView.Sources.selectedValue == targetUrl) {
        DebuggerView.setEditorLocation(targetUrl, currentLine, { noDebug: true });
      } else {
        DebuggerView.setEditorLocation(targetUrl);
      }
    }
  }
});




function FilteredFunctionsView() {
  dumpn("FilteredFunctionsView was instantiated");

  this._onClick = this._onClick.bind(this);
  this._onSelect = this._onSelect.bind(this);
}

FilteredFunctionsView.prototype = Heritage.extend(ResultsPanelContainer.prototype, {
  


  initialize: function() {
    dumpn("Initializing the FilteredFunctionsView");

    this.anchor = document.getElementById("searchbox");
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the FilteredFunctionsView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("click", this._onClick, false);
    this.anchor = null;
  },

  







  scheduleSearch: function(aToken, aWait) {
    
    let maxDelay = FUNCTION_SEARCH_ACTION_MAX_DELAY;
    let delay = aWait === undefined ? maxDelay / aToken.length : aWait;

    
    setNamedTimeout("function-search", delay, () => {
      
      let urls = DebuggerView.Sources.values;
      let sourcesFetched = DebuggerController.SourceScripts.getTextForSources(urls);
      sourcesFetched.then(aSources => this._doSearch(aToken, aSources));
    });
  },

  








  _doSearch: function(aToken, aSources, aStore = []) {
    
    

    
    
    let currentUrl = DebuggerView.Sources.selectedValue;
    let currentSource = aSources.filter(([sourceUrl]) => sourceUrl == currentUrl)[0];
    aSources.splice(aSources.indexOf(currentSource), 1);
    aSources.unshift(currentSource);

    
    
    if (!aToken) {
      aSources.splice(1);
    }

    for (let [location, contents] of aSources) {
      let parserMethods = DebuggerController.Parser.get(location, contents);
      let sourceResults = parserMethods.getNamedFunctionDefinitions(aToken);

      for (let scriptResult of sourceResults) {
        for (let parseResult of scriptResult.parseResults) {
          aStore.push({
            sourceUrl: scriptResult.sourceUrl,
            scriptOffset: scriptResult.scriptOffset,
            functionName: parseResult.functionName,
            functionLocation: parseResult.functionLocation,
            inferredName: parseResult.inferredName,
            inferredChain: parseResult.inferredChain,
            inferredLocation: parseResult.inferredLocation
          });

          
          
          if (aStore.length >= RESULTS_PANEL_MAX_RESULTS) {
            this._syncView(aStore);
            return;
          }
        }
      }
    }

    
    
    this._syncView(aStore);
  },

  





  _syncView: function(aSearchResults) {
    
    
    if (!aSearchResults.length) {
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

      this.push([trimmedLabel, trimmedValue, description], {
        index: -1, 
        relaxed: true, 
        attachment: item
      });
    }

    
    this.selectedIndex = 0;
    this.hidden = false;
  },

  


  _onClick: function(e) {
    let functionItem = this.getItemForElement(e.target);
    if (functionItem) {
      this.selectedItem = functionItem;
      DebuggerView.Filtering.clearSearch();
    }
  },

  


  _onSelect: function({ detail: functionItem }) {
    if (functionItem) {
      let sourceUrl = functionItem.attachment.sourceUrl;
      let scriptOffset = functionItem.attachment.scriptOffset;
      let actualLocation = functionItem.attachment.actualLocation;

      DebuggerView.setEditorLocation(sourceUrl, actualLocation.start.line, {
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
