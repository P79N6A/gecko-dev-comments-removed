




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
    this._resumeButton = document.getElementById("resume");
    this._stepOverButton = document.getElementById("step-over");
    this._stepInButton = document.getElementById("step-in");
    this._stepOutButton = document.getElementById("step-out");
    this._resumeOrderTooltip = new Tooltip(document);
    this._resumeOrderTooltip.defaultPosition = TOOLBAR_ORDER_POPUP_POSITION;

    let resumeKey = ShortcutUtils.prettifyShortcut(document.getElementById("resumeKey"));
    let stepOverKey = ShortcutUtils.prettifyShortcut(document.getElementById("stepOverKey"));
    let stepInKey = ShortcutUtils.prettifyShortcut(document.getElementById("stepInKey"));
    let stepOutKey = ShortcutUtils.prettifyShortcut(document.getElementById("stepOutKey"));
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
    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the ToolbarView");

    this._instrumentsPaneToggleButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.removeEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.removeEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.removeEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.removeEventListener("mousedown", this._onStepOutPressed, false);
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      resumeCommand: () => this._onResumePressed(),
      stepOverCommand: () => this._onStepOverPressed(),
      stepInCommand: () => this._onStepInPressed(),
      stepOutCommand: () => this._onStepOutPressed()
    });
  },

  






  showResumeWarning: function(aPausedUrl) {
    let label = L10N.getFormatStr("resumptionOrderPanelTitle", aPausedUrl);
    let defaultStyle = "default-tooltip-simple-text-colors";
    this._resumeOrderTooltip.setTextContent({ messages: [label], isAlertTooltip: true });
    this._resumeOrderTooltip.show(this._resumeButton);
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

  


  _onTogglePanesPressed: function() {
    DebuggerView.toggleInstrumentsPane({
      visible: DebuggerView.instrumentsPaneHidden,
      animated: true,
      delayed: true
    });
  },

  


  _onResumePressed: function() {
    if (DebuggerController.StackFrames._currentFrameDescription != FRAME_TYPE.NORMAL) {
      return;
    }

    if (DebuggerController.activeThread.paused) {
      let warn = DebuggerController._ensureResumptionOrder;
      DebuggerController.StackFrames.currentFrameDepth = -1;
      DebuggerController.activeThread.resume(warn);
    } else {
      DebuggerController.ThreadState.interruptedByResumeButton = true;
      DebuggerController.activeThread.interrupt();
    }
  },

  


  _onStepOverPressed: function() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.StackFrames.currentFrameDepth = -1;
      let warn = DebuggerController._ensureResumptionOrder;
      DebuggerController.activeThread.stepOver(warn);
    }
  },

  


  _onStepInPressed: function() {
    if (DebuggerController.StackFrames._currentFrameDescription != FRAME_TYPE.NORMAL) {
      return;
    }

    if (DebuggerController.activeThread.paused) {
      DebuggerController.StackFrames.currentFrameDepth = -1;
      let warn = DebuggerController._ensureResumptionOrder;
      DebuggerController.activeThread.stepIn(warn);
    }
  },

  


  _onStepOutPressed: function() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.StackFrames.currentFrameDepth = -1;
      let warn = DebuggerController._ensureResumptionOrder;
      DebuggerController.activeThread.stepOut(warn);
    }
  },

  _instrumentsPaneToggleButton: null,
  _resumeButton: null,
  _stepOverButton: null,
  _stepInButton: null,
  _stepOutButton: null,
  _resumeOrderTooltip: null,
  _resumeTooltip: "",
  _pauseTooltip: "",
  _stepOverTooltip: "",
  _stepInTooltip: "",
  _stepOutTooltip: ""
};




function OptionsView() {
  dumpn("OptionsView was instantiated");

  this._toggleAutoPrettyPrint = this._toggleAutoPrettyPrint.bind(this);
  this._togglePauseOnExceptions = this._togglePauseOnExceptions.bind(this);
  this._toggleIgnoreCaughtExceptions = this._toggleIgnoreCaughtExceptions.bind(this);
  this._toggleShowPanesOnStartup = this._toggleShowPanesOnStartup.bind(this);
  this._toggleShowVariablesOnlyEnum = this._toggleShowVariablesOnlyEnum.bind(this);
  this._toggleShowVariablesFilterBox = this._toggleShowVariablesFilterBox.bind(this);
  this._toggleShowOriginalSource = this._toggleShowOriginalSource.bind(this);
  this._toggleAutoBlackBox = this._toggleAutoBlackBox.bind(this);
}

OptionsView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the OptionsView");

    this._button = document.getElementById("debugger-options");
    this._autoPrettyPrint = document.getElementById("auto-pretty-print");
    this._pauseOnExceptionsItem = document.getElementById("pause-on-exceptions");
    this._ignoreCaughtExceptionsItem = document.getElementById("ignore-caught-exceptions");
    this._showPanesOnStartupItem = document.getElementById("show-panes-on-startup");
    this._showVariablesOnlyEnumItem = document.getElementById("show-vars-only-enum");
    this._showVariablesFilterBoxItem = document.getElementById("show-vars-filter-box");
    this._showOriginalSourceItem = document.getElementById("show-original-source");
    this._autoBlackBoxItem = document.getElementById("auto-black-box");

    this._autoPrettyPrint.setAttribute("checked", Prefs.autoPrettyPrint);
    this._pauseOnExceptionsItem.setAttribute("checked", Prefs.pauseOnExceptions);
    this._ignoreCaughtExceptionsItem.setAttribute("checked", Prefs.ignoreCaughtExceptions);
    this._showPanesOnStartupItem.setAttribute("checked", Prefs.panesVisibleOnStartup);
    this._showVariablesOnlyEnumItem.setAttribute("checked", Prefs.variablesOnlyEnumVisible);
    this._showVariablesFilterBoxItem.setAttribute("checked", Prefs.variablesSearchboxVisible);
    this._showOriginalSourceItem.setAttribute("checked", Prefs.sourceMapsEnabled);
    this._autoBlackBoxItem.setAttribute("checked", Prefs.autoBlackBox);

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the OptionsView");
    
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      toggleAutoPrettyPrint: () => this._toggleAutoPrettyPrint(),
      togglePauseOnExceptions: () => this._togglePauseOnExceptions(),
      toggleIgnoreCaughtExceptions: () => this._toggleIgnoreCaughtExceptions(),
      toggleShowPanesOnStartup: () => this._toggleShowPanesOnStartup(),
      toggleShowOnlyEnum: () => this._toggleShowVariablesOnlyEnum(),
      toggleShowVariablesFilterBox: () => this._toggleShowVariablesFilterBox(),
      toggleShowOriginalSource: () => this._toggleShowOriginalSource(),
      toggleAutoBlackBox: () =>  this._toggleAutoBlackBox()
    });
  },

  


  _onPopupShowing: function() {
    this._button.setAttribute("open", "true");
    window.emit(EVENTS.OPTIONS_POPUP_SHOWING);
  },

  


  _onPopupHiding: function() {
    this._button.removeAttribute("open");
  },

  


  _onPopupHidden: function() {
    window.emit(EVENTS.OPTIONS_POPUP_HIDDEN);
  },

  


  _toggleAutoPrettyPrint: function(){
    Prefs.autoPrettyPrint =
      this._autoPrettyPrint.getAttribute("checked") == "true";
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

    
    window.once(EVENTS.OPTIONS_POPUP_HIDDEN, () => {
      
      window.setTimeout(() => {
        DebuggerController.reconfigureThread({
          useSourceMaps: pref,
          autoBlackBox: Prefs.autoBlackBox
        });
      }, POPUP_HIDDEN_DELAY);
    });
  },

  



  _toggleAutoBlackBox: function() {
    let pref = Prefs.autoBlackBox =
      this._autoBlackBoxItem.getAttribute("checked") == "true";

    
    window.once(EVENTS.OPTIONS_POPUP_HIDDEN, () => {
      
      window.setTimeout(() => {
        DebuggerController.reconfigureThread({
          useSourceMaps: Prefs.sourceMapsEnabled,
          autoBlackBox: pref
        });
      }, POPUP_HIDDEN_DELAY);
    });
  },

  _button: null,
  _pauseOnExceptionsItem: null,
  _showPanesOnStartupItem: null,
  _showVariablesOnlyEnumItem: null,
  _showVariablesFilterBoxItem: null,
  _showOriginalSourceItem: null,
  _autoBlackBoxItem: null
};




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

    this.widget = new BreadcrumbsWidget(document.getElementById("stackframes"));
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("scroll", this._onScroll, true);
    window.addEventListener("resize", this._onScroll, true);

    this.autoFocusOnFirstItem = false;
    this.autoFocusOnSelection = false;

    
    this._mirror = DebuggerView.StackFramesClassicList;
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

    
    this.push([frameView], {
      index: 0, 
      attachment: {
        title: aTitle,
        url: aUrl,
        line: aLine,
        depth: aDepth
      },
      
      
      finalize: this._onStackframeRemoved
    });

    
    this._mirror.addFrame(aTitle, aUrl, aLine, aDepth);
  },

  



  set selectedDepth(aDepth) {
    this.selectedItem = aItem => aItem.attachment.depth == aDepth;
  },

  






  get selectedDepth() {
    return this.selectedItem.attachment.depth;
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

  





  _onStackframeRemoved: function(aItem) {
    dumpn("Finalizing stackframe item: " + aItem.stringify());

    
    let depth = aItem.attachment.depth;
    this._mirror.remove(this._mirror.getItemForAttachment(e => e.depth == depth));

    
    this._prevBlackBoxedUrl = null;
  },

  


  _onSelect: function(e) {
    let stackframeItem = this.selectedItem;
    if (stackframeItem) {
      
      let depth = stackframeItem.attachment.depth;

      
      this.suppressSelectionEvents = true;
      this._mirror.selectedItem = e => e.attachment.depth == depth;
      this.suppressSelectionEvents = false;

      DebuggerController.StackFrames.selectFrame(depth);
    }
  },

  


  _onScroll: function() {
    
    if (!this.dirty) {
      return;
    }
    
    setNamedTimeout("stack-scroll", STACK_FRAMES_SCROLL_DELAY, this._afterScroll);
  },

  


  _afterScroll: function() {
    let scrollPosition = this.widget.getAttribute("scrollPosition");
    let scrollWidth = this.widget.getAttribute("scrollWidth");

    
    
    if (scrollPosition - scrollWidth / 10 < 1) {
      this.ensureIndexIsVisible(CALL_STACK_PAGE_SIZE - 1);
      this.dirty = false;

      
      DebuggerController.StackFrames.addMoreFrames();
    }
  },

  _mirror: null,
  _prevBlackBoxedUrl: null
});





function StackFramesClassicListView() {
  dumpn("StackFramesClassicListView was instantiated");

  this._onSelect = this._onSelect.bind(this);
}

StackFramesClassicListView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the StackFramesClassicListView");

    this.widget = new SideMenuWidget(document.getElementById("callstack-list"));
    this.widget.addEventListener("select", this._onSelect, false);

    this.emptyText = L10N.getStr("noStackFramesText");
    this.autoFocusOnFirstItem = false;
    this.autoFocusOnSelection = false;

    
    this._mirror = DebuggerView.StackFrames;
  },

  


  destroy: function() {
    dumpn("Destroying the StackFramesClassicListView");

    this.widget.removeEventListener("select", this._onSelect, false);
  },

  











  addFrame: function(aTitle, aUrl, aLine, aDepth) {
    
    let frameView = this._createFrameView.apply(this, arguments);

    
    this.push([frameView], {
      attachment: {
        depth: aDepth
      }
    });
  },

  













  _createFrameView: function(aTitle, aUrl, aLine, aDepth) {
    let container = document.createElement("hbox");
    container.id = "classic-stackframe-" + aDepth;
    container.className = "dbg-classic-stackframe";
    container.setAttribute("flex", "1");

    let frameTitleNode = document.createElement("label");
    frameTitleNode.className = "plain dbg-classic-stackframe-title";
    frameTitleNode.setAttribute("value", aTitle);
    frameTitleNode.setAttribute("crop", "center");

    let frameDetailsNode = document.createElement("hbox");
    frameDetailsNode.className = "plain dbg-classic-stackframe-details";

    let frameUrlNode = document.createElement("label");
    frameUrlNode.className = "plain dbg-classic-stackframe-details-url";
    frameUrlNode.setAttribute("value", SourceUtils.getSourceLabel(aUrl));
    frameUrlNode.setAttribute("crop", "center");
    frameDetailsNode.appendChild(frameUrlNode);

    let frameDetailsSeparator = document.createElement("label");
    frameDetailsSeparator.className = "plain dbg-classic-stackframe-details-sep";
    frameDetailsSeparator.setAttribute("value", SEARCH_LINE_FLAG);
    frameDetailsNode.appendChild(frameDetailsSeparator);

    let frameLineNode = document.createElement("label");
    frameLineNode.className = "plain dbg-classic-stackframe-details-line";
    frameLineNode.setAttribute("value", aLine);
    frameDetailsNode.appendChild(frameLineNode);

    container.appendChild(frameTitleNode);
    container.appendChild(frameDetailsNode);

    return container;
  },

  


  _onSelect: function(e) {
    let stackframeItem = this.selectedItem;
    if (stackframeItem) {
      
      
      let depth = stackframeItem.attachment.depth;
      this._mirror.selectedItem = e => e.attachment.depth == depth;
    }
  },

  _mirror: null
});




function FilterView() {
  dumpn("FilterView was instantiated");

  this._onClick = this._onClick.bind(this);
  this._onInput = this._onInput.bind(this);
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

    this._fileSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("fileSearchKey"));
    this._globalSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("globalSearchKey"));
    this._filteredFunctionsKey = ShortcutUtils.prettifyShortcut(document.getElementById("functionSearchKey"));
    this._tokenSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("tokenSearchKey"));
    this._lineSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("lineSearchKey"));
    this._variableSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("variableSearchKey"));

    this._searchbox.addEventListener("click", this._onClick, false);
    this._searchbox.addEventListener("select", this._onInput, false);
    this._searchbox.addEventListener("input", this._onInput, false);
    this._searchbox.addEventListener("keypress", this._onKeyPress, false);
    this._searchbox.addEventListener("blur", this._onBlur, false);

    let placeholder = L10N.getFormatStr("emptySearchText", this._fileSearchKey);
    this._searchbox.setAttribute("placeholder", placeholder);

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

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the FilterView");

    this._searchbox.removeEventListener("click", this._onClick, false);
    this._searchbox.removeEventListener("select", this._onInput, false);
    this._searchbox.removeEventListener("input", this._onInput, false);
    this._searchbox.removeEventListener("keypress", this._onKeyPress, false);
    this._searchbox.removeEventListener("blur", this._onBlur, false);
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      fileSearchCommand: () => this._doFileSearch(),
      globalSearchCommand: () => this._doGlobalSearch(),
      functionSearchCommand: () => this._doFunctionSearch(),
      tokenSearchCommand: () => this._doTokenSearch(),
      lineSearchCommand: () => this._doLineSearch(),
      variableSearchCommand: () => this._doVariableSearch(),
      variablesFocusCommand: () => this._doVariablesFocus()
    });
  },

  



  get searchData() {
    let operator = "", args = [];

    let rawValue = this._searchbox.value;
    let rawLength = rawValue.length;
    let globalFlagIndex = rawValue.indexOf(SEARCH_GLOBAL_FLAG);
    let functionFlagIndex = rawValue.indexOf(SEARCH_FUNCTION_FLAG);
    let variableFlagIndex = rawValue.indexOf(SEARCH_VARIABLE_FLAG);
    let tokenFlagIndex = rawValue.lastIndexOf(SEARCH_TOKEN_FLAG);
    let lineFlagIndex = rawValue.lastIndexOf(SEARCH_LINE_FLAG);

    
    if (globalFlagIndex != 0 && functionFlagIndex != 0 && variableFlagIndex != 0) {
      
      if (tokenFlagIndex != -1) {
        operator = SEARCH_TOKEN_FLAG;
        args.push(rawValue.slice(0, tokenFlagIndex)); 
        args.push(rawValue.substr(tokenFlagIndex + 1, rawLength)); 
      } else if (lineFlagIndex != -1) {
        operator = SEARCH_LINE_FLAG;
        args.push(rawValue.slice(0, lineFlagIndex)); 
        args.push(+rawValue.substr(lineFlagIndex + 1, rawLength) || 0); 
      } else {
        args.push(rawValue);
      }
    }
    
    else if (globalFlagIndex == 0) {
      operator = SEARCH_GLOBAL_FLAG;
      args.push(rawValue.slice(1));
    }
    
    else if (functionFlagIndex == 0) {
      operator = SEARCH_FUNCTION_FLAG;
      args.push(rawValue.slice(1));
    }
    
    else if (variableFlagIndex == 0) {
      operator = SEARCH_VARIABLE_FLAG;
      args.push(rawValue.slice(1));
    }

    return [operator, args];
  },

  



  get searchOperator() this.searchData[0],

  



  get searchArguments() this.searchData[1],

  


  clearSearch: function() {
    this._searchbox.value = "";
    this.clearViews();
  },

  


  clearViews: function() {
    DebuggerView.GlobalSearch.clearView();
    DebuggerView.FilteredSources.clearView();
    DebuggerView.FilteredFunctions.clearView();
    this._searchboxHelpPanel.hidePopup();
  },

  






  _performLineSearch: function(aLine) {
    
    if (aLine) {
      DebuggerView.editor.setCursor({ line: aLine - 1, ch: 0 }, "center");
    }
  },

  






  _performTokenSearch: function(aToken) {
    
    if (!aToken) {
      return;
    }
    DebuggerView.editor.find(aToken);
  },

  


  _onClick: function() {
    
    
    if (!this._searchbox.value) {
      this._searchboxHelpPanel.openPopup(this._searchbox);
    }
  },

  


  _onInput: function() {
    this.clearViews();

    
    if (!this._searchbox.value) {
      return;
    }

    
    switch (this.searchOperator) {
      case SEARCH_GLOBAL_FLAG:
        
        DebuggerView.GlobalSearch.scheduleSearch(this.searchArguments[0]);
        break;
      case SEARCH_FUNCTION_FLAG:
      
        DebuggerView.FilteredFunctions.scheduleSearch(this.searchArguments[0]);
        break;
      case SEARCH_VARIABLE_FLAG:
        
        DebuggerView.Variables.scheduleSearch(this.searchArguments[0]);
        break;
      case SEARCH_TOKEN_FLAG:
        
        DebuggerView.FilteredSources.scheduleSearch(this.searchArguments[0]);
        this._performTokenSearch(this.searchArguments[1]);
        break;
      case SEARCH_LINE_FLAG:
        
        DebuggerView.FilteredSources.scheduleSearch(this.searchArguments[0]);
        this._performLineSearch(this.searchArguments[1]);
        break;
      default:
        
        DebuggerView.FilteredSources.scheduleSearch(this.searchArguments[0]);
        break;
    }
  },

  


  _onKeyPress: function(e) {
    
    e.char = String.fromCharCode(e.charCode);

    
    let [operator, args] = this.searchData;
    let isGlobalSearch = operator == SEARCH_GLOBAL_FLAG;
    let isFunctionSearch = operator == SEARCH_FUNCTION_FLAG;
    let isVariableSearch = operator == SEARCH_VARIABLE_FLAG;
    let isTokenSearch = operator == SEARCH_TOKEN_FLAG;
    let isLineSearch = operator == SEARCH_LINE_FLAG;
    let isFileOnlySearch = !operator && args.length == 1;

    
    let actionToPerform;

    
    if ((e.char == "g" && e.metaKey) || e.char == "n" && e.ctrlKey) {
      actionToPerform = "selectNext";
    }
    
    else if ((e.char == "G" && e.metaKey) || e.char == "p" && e.ctrlKey) {
      actionToPerform = "selectPrev";
    }
    
    
    else switch (e.keyCode) {
      case e.DOM_VK_RETURN:
        var isReturnKey = true;
        
        actionToPerform = e.shiftKey ? "selectPrev" : "selectNext";
        break;
      case e.DOM_VK_DOWN:
        actionToPerform = "selectNext";
        break;
      case e.DOM_VK_UP:
        actionToPerform = "selectPrev";
        break;
    }

    
    
    if (!actionToPerform || (!operator && !args.length)) {
      DebuggerView.editor.dropSelection();
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    
    
    if (isGlobalSearch) {
      let targetView = DebuggerView.GlobalSearch;
      if (!isReturnKey) {
        targetView[actionToPerform]();
      } else if (targetView.hidden) {
        targetView.scheduleSearch(args[0], 0);
      }
      return;
    }

    
    
    if (isFunctionSearch) {
      let targetView = DebuggerView.FilteredFunctions;
      if (!isReturnKey) {
        targetView[actionToPerform]();
      } else if (targetView.hidden) {
        targetView.scheduleSearch(args[0], 0);
      } else {
        if (!targetView.selectedItem) {
          targetView.selectedIndex = 0;
        }
        this.clearSearch();
      }
      return;
    }

    
    if (isVariableSearch) {
      let targetView = DebuggerView.Variables;
      if (isReturnKey) {
        targetView.scheduleSearch(args[0], 0);
      }
      return;
    }

    
    
    if (isFileOnlySearch) {
      let targetView = DebuggerView.FilteredSources;
      if (!isReturnKey) {
        targetView[actionToPerform]();
      } else if (targetView.hidden) {
        targetView.scheduleSearch(args[0], 0);
      } else {
        if (!targetView.selectedItem) {
          targetView.selectedIndex = 0;
        }
        this.clearSearch();
      }
      return;
    }

    
    if (isTokenSearch) {
      let methods = { selectNext: "findNext", selectPrev: "findPrev" };
      DebuggerView.editor[methods[actionToPerform]]();
      return;
    }

    
    if (isLineSearch) {
      let [, line] = args;
      let amounts = { selectNext: 1, selectPrev: -1 };

      
      line += !isReturnKey ? amounts[actionToPerform] : 0;
      let lineCount = DebuggerView.editor.lineCount();
      let lineTarget = line < 1 ? 1 : line > lineCount ? lineCount : line;
      this._doSearch(SEARCH_LINE_FLAG, lineTarget);
      return;
    }
  },

  


  _onBlur: function() {
    this.clearViews();
  },

  





  _doSearch: function(aOperator = "", aText = "") {
    this._searchbox.focus();
    this._searchbox.value = ""; 

    if (aText) {
      this._searchbox.value = aOperator + aText;
      return;
    }
    if (DebuggerView.editor.somethingSelected()) {
      this._searchbox.value = aOperator + DebuggerView.editor.getSelection();
      return;
    }
    if (SEARCH_AUTOFILL.indexOf(aOperator) != -1) {
      let cursor = DebuggerView.editor.getCursor();
      let content = DebuggerView.editor.getText();
      let location = DebuggerView.Sources.selectedItem.attachment.source.url;
      let source = DebuggerController.Parser.get(content, location);
      let identifier = source.getIdentifierAt({ line: cursor.line+1, column: cursor.ch });

      if (identifier && identifier.name) {
        this._searchbox.value = aOperator + identifier.name;
        this._searchbox.select();
        this._searchbox.selectionStart += aOperator.length;
        return;
      }
    }
    this._searchbox.value = aOperator;
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
      let lowerCaseLabel = item.attachment.label.toLowerCase();
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
      window.emit(EVENTS.FILE_SEARCH_MATCH_NOT_FOUND);
      return;
    }

    for (let item of aSearchResults) {
      let url = item.attachment.source.url;

      if (url) {
        
        let itemView = this._createItemView(
          SourceUtils.trimUrlLength(item.attachment.label),
          SourceUtils.trimUrlLength(url, 0, "start")
        );

        
        this.push([itemView], {
          index: -1, 
          attachment: {
            url: url
          }
        });
      }
    }

    
    
    if (this._autoSelectFirstItem || DebuggerView.Filtering.searchOperator) {
      this.selectedIndex = 0;
    }
    this.hidden = false;

    
    window.emit(EVENTS.FILE_SEARCH_MATCH_FOUND);
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
      let actor = DebuggerView.Sources.getActorForLocation({ url: locationItem.attachment.url });
      DebuggerView.setEditorLocation(actor, undefined, {
        noCaret: true,
        noDebug: true
      });
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
      
      let actors = DebuggerView.Sources.values;
      let sourcesFetched = DebuggerController.SourceScripts.getTextForSources(actors);
      sourcesFetched.then(aSources => this._doSearch(aToken, aSources));
    });
  },

  








  _doSearch: function(aToken, aSources, aStore = []) {
    
    

    
    
    let currentActor = DebuggerView.Sources.selectedValue;
    let currentSource = aSources.filter(([actor]) => actor == currentActor)[0];
    aSources.splice(aSources.indexOf(currentSource), 1);
    aSources.unshift(currentSource);

    
    
    if (!aToken) {
      aSources.splice(1);
    }

    for (let [actor, contents] of aSources) {
      let item = DebuggerView.Sources.getItemByValue(actor);
      let url = item.attachment.source.url;
      if (!url) {
        continue;
      }

      let parsedSource = DebuggerController.Parser.get(contents, url);
      let sourceResults = parsedSource.getNamedFunctionDefinitions(aToken);

      for (let scriptResult of sourceResults) {
        for (let parseResult of scriptResult) {
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
      window.emit(EVENTS.FUNCTION_SEARCH_MATCH_NOT_FOUND);
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

      
      let itemView = this._createItemView(
        SourceUtils.trimUrlLength(item.displayedName + "()"),
        SourceUtils.trimUrlLength(item.sourceUrl, 0, "start"),
        (item.inferredChain || []).join(".")
      );

      
      this.push([itemView], {
        index: -1, 
        attachment: item
      });
    }

    
    
    if (this._autoSelectFirstItem) {
      this.selectedIndex = 0;
    }
    this.hidden = false;

    
    window.emit(EVENTS.FUNCTION_SEARCH_MATCH_FOUND);
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
      let actor = DebuggerView.Sources.getActorForLocation({ url: sourceUrl });
      let scriptOffset = functionItem.attachment.scriptOffset;
      let actualLocation = functionItem.attachment.actualLocation;

      DebuggerView.setEditorLocation(actor, actualLocation.start.line, {
        charOffset: scriptOffset,
        columnOffset: actualLocation.start.column,
        align: "center",
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
DebuggerView.StackFrames = new StackFramesView();
DebuggerView.StackFramesClassicList = new StackFramesClassicListView();
