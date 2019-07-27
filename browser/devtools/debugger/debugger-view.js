




"use strict";

const SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 1048576; 
const SOURCE_URL_DEFAULT_MAX_LENGTH = 64; 
const STACK_FRAMES_SOURCE_URL_MAX_LENGTH = 15; 
const STACK_FRAMES_SOURCE_URL_TRIM_SECTION = "center";
const STACK_FRAMES_SCROLL_DELAY = 100; 
const BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH = 1000; 
const BREAKPOINT_CONDITIONAL_POPUP_POSITION = "before_start";
const BREAKPOINT_CONDITIONAL_POPUP_OFFSET_X = 7; 
const BREAKPOINT_CONDITIONAL_POPUP_OFFSET_Y = -3; 
const RESULTS_PANEL_POPUP_POSITION = "before_end";
const RESULTS_PANEL_MAX_RESULTS = 10;
const FILE_SEARCH_ACTION_MAX_DELAY = 300; 
const GLOBAL_SEARCH_EXPAND_MAX_RESULTS = 50;
const GLOBAL_SEARCH_LINE_MAX_LENGTH = 300; 
const GLOBAL_SEARCH_ACTION_MAX_DELAY = 1500; 
const FUNCTION_SEARCH_ACTION_MAX_DELAY = 400; 
const SEARCH_GLOBAL_FLAG = "!";
const SEARCH_FUNCTION_FLAG = "@";
const SEARCH_TOKEN_FLAG = "#";
const SEARCH_LINE_FLAG = ":";
const SEARCH_VARIABLE_FLAG = "*";
const SEARCH_AUTOFILL = [SEARCH_GLOBAL_FLAG, SEARCH_FUNCTION_FLAG, SEARCH_TOKEN_FLAG];
const EDITOR_VARIABLE_HOVER_DELAY = 750; 
const EDITOR_VARIABLE_POPUP_POSITION = "topcenter bottomleft";
const TOOLBAR_ORDER_POPUP_POSITION = "topcenter bottomleft";




let DebuggerView = {
  





  initialize: function() {
    if (this._startup) {
      return this._startup;
    }

    let deferred = promise.defer();
    this._startup = deferred.promise;

    this._initializePanes();
    this.Toolbar.initialize();
    this.Options.initialize();
    this.Filtering.initialize();
    this.StackFrames.initialize();
    this.StackFramesClassicList.initialize();
    this.Sources.initialize();
    this.VariableBubble.initialize();
    this.Tracer.initialize();
    this.WatchExpressions.initialize();
    this.EventListeners.initialize();
    this.GlobalSearch.initialize();
    this._initializeVariablesView();
    this._initializeEditor(deferred.resolve);

    document.title = L10N.getStr("DebuggerWindowTitle");

    return deferred.promise;
  },

  





  destroy: function() {
    if (this._shutdown) {
      return this._shutdown;
    }

    let deferred = promise.defer();
    this._shutdown = deferred.promise;

    this.Toolbar.destroy();
    this.Options.destroy();
    this.Filtering.destroy();
    this.StackFrames.destroy();
    this.StackFramesClassicList.destroy();
    this.Sources.destroy();
    this.VariableBubble.destroy();
    this.Tracer.destroy();
    this.WatchExpressions.destroy();
    this.EventListeners.destroy();
    this.GlobalSearch.destroy();
    this._destroyPanes();
    this._destroyEditor(deferred.resolve);

    return deferred.promise;
  },

  


  _initializePanes: function() {
    dumpn("Initializing the DebuggerView panes");

    this._body = document.getElementById("body");
    this._editorDeck = document.getElementById("editor-deck");
    this._sourcesPane = document.getElementById("sources-pane");
    this._instrumentsPane = document.getElementById("instruments-pane");
    this._instrumentsPaneToggleButton = document.getElementById("instruments-pane-toggle");

    this.showEditor = this.showEditor.bind(this);
    this.showBlackBoxMessage = this.showBlackBoxMessage.bind(this);
    this.showProgressBar = this.showProgressBar.bind(this);
    this.maybeShowBlackBoxMessage = this.maybeShowBlackBoxMessage.bind(this);

    this._onTabSelect = this._onInstrumentsPaneTabSelect.bind(this);
    this._instrumentsPane.tabpanels.addEventListener("select", this._onTabSelect);

    this._collapsePaneString = L10N.getStr("collapsePanes");
    this._expandPaneString = L10N.getStr("expandPanes");

    this._sourcesPane.setAttribute("width", Prefs.sourcesWidth);
    this._instrumentsPane.setAttribute("width", Prefs.instrumentsWidth);
    this.toggleInstrumentsPane({ visible: Prefs.panesVisibleOnStartup });

    
    if (gHostType == "side") {
      this.handleHostChanged(gHostType);
    }
  },

  


  _destroyPanes: function() {
    dumpn("Destroying the DebuggerView panes");

    if (gHostType != "side") {
      Prefs.sourcesWidth = this._sourcesPane.getAttribute("width");
      Prefs.instrumentsWidth = this._instrumentsPane.getAttribute("width");
    }

    this._sourcesPane = null;
    this._instrumentsPane = null;
    this._instrumentsPaneToggleButton = null;
  },

  


  _initializeVariablesView: function() {
    this.Variables = new VariablesView(document.getElementById("variables"), {
      searchPlaceholder: L10N.getStr("emptyVariablesFilterText"),
      emptyText: L10N.getStr("emptyVariablesText"),
      onlyEnumVisible: Prefs.variablesOnlyEnumVisible,
      searchEnabled: Prefs.variablesSearchboxVisible,
      eval: (variable, value) => {
        let string = variable.evaluationMacro(variable, value);
        DebuggerController.StackFrames.evaluate(string);
      },
      lazyEmpty: true
    });

    
    
    this.Variables.toolbox = DebuggerController._toolbox;

    
    VariablesViewController.attach(this.Variables, {
      getEnvironmentClient: aObject => gThreadClient.environment(aObject),
      getObjectClient: aObject => {
        return aObject instanceof DebuggerController.Tracer.WrappedObject
          ? DebuggerController.Tracer.syncGripClient(aObject.object)
          : gThreadClient.pauseGrip(aObject)
      }
    });

    
    this.Variables.on("fetched", (aEvent, aType) => {
      switch (aType) {
        case "scopes":
          window.emit(EVENTS.FETCHED_SCOPES);
          break;
        case "variables":
          window.emit(EVENTS.FETCHED_VARIABLES);
          break;
        case "properties":
          window.emit(EVENTS.FETCHED_PROPERTIES);
          break;
      }
    });
  },

  





  _initializeEditor: function(aCallback) {
    dumpn("Initializing the DebuggerView editor");

    let extraKeys = {};
    bindKey("_doTokenSearch", "tokenSearchKey");
    bindKey("_doGlobalSearch", "globalSearchKey", { alt: true });
    bindKey("_doFunctionSearch", "functionSearchKey");
    extraKeys[Editor.keyFor("jumpToLine")] = false;
    extraKeys["Esc"] = false;

    function bindKey(func, key, modifiers = {}) {
      key = document.getElementById(key).getAttribute("key");
      let shortcut = Editor.accel(key, modifiers);
      extraKeys[shortcut] = () => DebuggerView.Filtering[func]();
    }

    let gutters = ["breakpoints"];
    if (Services.prefs.getBoolPref("devtools.debugger.tracer")) {
      gutters.unshift("hit-counts");
    }

    this.editor = new Editor({
      mode: Editor.modes.text,
      readOnly: true,
      lineNumbers: true,
      showAnnotationRuler: true,
      gutters: gutters,
      extraKeys: extraKeys,
      contextMenu: "sourceEditorContextMenu",
      enableCodeFolding: false
    });

    this.editor.appendTo(document.getElementById("editor")).then(() => {
      this.editor.extend(DebuggerEditor);
      this._loadingText = L10N.getStr("loadingText");
      this._onEditorLoad(aCallback);
    });

    this.editor.on("gutterClick", (ev, line, button) => {
      
      
      if (button == 2) {
        this.clickedLine = line;
      }
      else {
        if (this.editor.hasBreakpoint(line)) {
          this.editor.removeBreakpoint(line);
        } else {
          this.editor.addBreakpoint(line);
        }
      }
    });
  },

  






  _onEditorLoad: function(aCallback) {
    dumpn("Finished loading the DebuggerView editor");

    DebuggerController.Breakpoints.initialize().then(() => {
      window.emit(EVENTS.EDITOR_LOADED, this.editor);
      aCallback();
    });
  },

  






  _destroyEditor: function(aCallback) {
    dumpn("Destroying the DebuggerView editor");

    DebuggerController.Breakpoints.destroy().then(() => {
      window.emit(EVENTS.EDITOR_UNLOADED, this.editor);
      this.editor.destroy();
      this.editor = null;
      aCallback();
    });
  },

  


  showEditor: function() {
    this._editorDeck.selectedIndex = 0;
  },

  


  showBlackBoxMessage: function() {
    this._editorDeck.selectedIndex = 1;
  },

  


  showProgressBar: function() {
    this._editorDeck.selectedIndex = 2;
  },

  



  maybeShowBlackBoxMessage: function() {
    let { source } = DebuggerView.Sources.selectedItem.attachment;
    if (gThreadClient.source(source).isBlackBoxed) {
      this.showBlackBoxMessage();
    } else {
      this.showEditor();
    }
  },

  






  _setEditorText: function(aTextContent = "") {
    this.editor.setMode(Editor.modes.text);
    this.editor.setText(aTextContent);
    this.editor.clearDebugLocation();
    this.editor.clearHistory();
  },

  










  _setEditorMode: function(aUrl, aContentType = "", aTextContent = "") {
    
    
    if (aTextContent.length >= SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
      return void this.editor.setMode(Editor.modes.text);
    }

    
    if (SourceUtils.isJavaScript(aUrl, aContentType)) {
      return void this.editor.setMode(Editor.modes.js);
    }

    
    
    if (aTextContent.match(/^\s*</)) {
      return void this.editor.setMode(Editor.modes.html);
    }

    
    this.editor.setMode(Editor.modes.text);
  },

  













  _setEditorSource: function(aSource, aFlags={}) {
    
    if (this._editorSource.actor == aSource.actor && !aFlags.force) {
      return this._editorSource.promise;
    }
    let transportType = gClient.localTransport ? "_LOCAL" : "_REMOTE";
    let histogramId = "DEVTOOLS_DEBUGGER_DISPLAY_SOURCE" + transportType + "_MS";
    let histogram = Services.telemetry.getHistogramById(histogramId);
    let startTime = Date.now();

    let deferred = promise.defer();

    this._setEditorText(L10N.getStr("loadingText"));
    this._editorSource = { actor: aSource.actor, promise: deferred.promise };

    DebuggerController.SourceScripts.getText(aSource).then(([, aText, aContentType]) => {
      
      
      if (this._editorSource.actor != aSource.actor) {
        return;
      }

      this._setEditorText(aText);
      this._setEditorMode(aSource.url, aContentType, aText);

      
      
      DebuggerView.Sources.selectedValue = aSource.actor;
      DebuggerController.Breakpoints.updateEditorBreakpoints();
      DebuggerController.HitCounts.updateEditorHitCounts();

      histogram.add(Date.now() - startTime);

      
      window.emit(EVENTS.SOURCE_SHOWN, aSource);
      deferred.resolve([aSource, aText, aContentType]);
    },
    ([, aError]) => {
      let msg = L10N.getStr("errorLoadingText") + DevToolsUtils.safeErrorString(aError);
      this._setEditorText(msg);
      Cu.reportError(msg);
      dumpn(msg);

      
      window.emit(EVENTS.SOURCE_ERROR_SHOWN, aSource);
      deferred.reject([aSource, aError]);
    });

    return deferred.promise;
  },

  




















  setEditorLocation: function(aActor, aLine = 0, aFlags = {}) {
    
    if (!this.Sources.containsValue(aActor)) {
      return promise.reject(new Error("Unknown source for the specified URL."));
    }

    
    
    if (!aLine) {
      let cachedFrames = DebuggerController.activeThread.cachedFrames;
      let currentDepth = DebuggerController.StackFrames.currentFrameDepth;
      let frame = cachedFrames[currentDepth];
      if (frame && frame.source.actor == aActor) {
        aLine = frame.where.line;
      }
    }

    let sourceItem = this.Sources.getItemByValue(aActor);
    let sourceForm = sourceItem.attachment.source;

    this._editorLoc = { actor: sourceForm.actor };

    
    
    return this._setEditorSource(sourceForm, aFlags).then(([,, aContentType]) => {
      if (this._editorLoc.actor !== sourceForm.actor) {
        return;
      }

      
      sourceForm.contentType = aContentType;
      
      
      if (aLine < 1) {
        window.emit(EVENTS.EDITOR_LOCATION_SET);
        return;
      }
      if (aFlags.charOffset) {
        aLine += this.editor.getPosition(aFlags.charOffset).line;
      }
      if (aFlags.lineOffset) {
        aLine += aFlags.lineOffset;
      }
      if (!aFlags.noCaret) {
        let location = { line: aLine -1, ch: aFlags.columnOffset || 0 };
        this.editor.setCursor(location, aFlags.align);
      }
      if (!aFlags.noDebug) {
        this.editor.setDebugLocation(aLine - 1);
      }
      window.emit(EVENTS.EDITOR_LOCATION_SET);
    }).then(null, console.error);
  },

  



  get instrumentsPaneHidden()
    this._instrumentsPane.hasAttribute("pane-collapsed"),

  



  get instrumentsPaneTab()
    this._instrumentsPane.selectedTab.id,

  











  toggleInstrumentsPane: function(aFlags, aTabIndex) {
    let pane = this._instrumentsPane;
    let button = this._instrumentsPaneToggleButton;

    ViewHelpers.togglePane(aFlags, pane);

    if (aFlags.visible) {
      button.removeAttribute("pane-collapsed");
      button.setAttribute("tooltiptext", this._collapsePaneString);
    } else {
      button.setAttribute("pane-collapsed", "");
      button.setAttribute("tooltiptext", this._expandPaneString);
    }

    if (aTabIndex !== undefined) {
      pane.selectedIndex = aTabIndex;
    }
  },

  





  showInstrumentsPane: function(aCallback) {
    DebuggerView.toggleInstrumentsPane({
      visible: true,
      animated: true,
      delayed: true,
      callback: aCallback
    }, 0);
  },

  


  _onInstrumentsPaneTabSelect: function() {
    if (this._instrumentsPane.selectedTab.id == "events-tab") {
      DebuggerController.Breakpoints.DOM.scheduleEventListenersFetch();
    }
  },

  





  handleHostChanged: function(aType) {
    let newLayout = "";

    if (aType == "side") {
      newLayout = "vertical";
      this._enterVerticalLayout();
    } else {
      newLayout = "horizontal";
      this._enterHorizontalLayout();
    }

    this._hostType = aType;
    this._body.setAttribute("layout", newLayout);
    window.emit(EVENTS.LAYOUT_CHANGED, newLayout);
  },

  


  _enterVerticalLayout: function() {
    let normContainer = document.getElementById("debugger-widgets");
    let vertContainer = document.getElementById("vertical-layout-panes-container");

    
    let splitter = document.getElementById("sources-and-instruments-splitter");
    vertContainer.insertBefore(this._sourcesPane, splitter);
    vertContainer.appendChild(this._instrumentsPane);

    
    
    vertContainer.setAttribute("height",
      vertContainer.getBoundingClientRect().height);
  },

  


  _enterHorizontalLayout: function() {
    let normContainer = document.getElementById("debugger-widgets");
    let vertContainer = document.getElementById("vertical-layout-panes-container");

    
    
    let splitter = document.getElementById("sources-and-editor-splitter");
    normContainer.insertBefore(this._sourcesPane, splitter);
    normContainer.appendChild(this._instrumentsPane);

    
    
    this._sourcesPane.setAttribute("width", Prefs.sourcesWidth);
    this._instrumentsPane.setAttribute("width", Prefs.instrumentsWidth);
  },

  


  handleTabNavigation: function() {
    dumpn("Handling tab navigation in the DebuggerView");

    this.Filtering.clearSearch();
    this.GlobalSearch.clearView();
    this.StackFrames.empty();
    this.Sources.empty();
    this.Variables.empty();
    this.EventListeners.empty();

    if (this.editor) {
      this.editor.setMode(Editor.modes.text);
      this.editor.setText("");
      this.editor.clearHistory();
      this._editorSource = {};
    }

    this.Sources.emptyText = L10N.getStr("loadingSourcesText");
  },

  _startup: null,
  _shutdown: null,
  Toolbar: null,
  Options: null,
  Filtering: null,
  GlobalSearch: null,
  StackFrames: null,
  Sources: null,
  Tracer: null,
  Variables: null,
  VariableBubble: null,
  WatchExpressions: null,
  EventListeners: null,
  editor: null,
  _editorSource: {},
  _loadingText: "",
  _body: null,
  _editorDeck: null,
  _sourcesPane: null,
  _instrumentsPane: null,
  _instrumentsPaneToggleButton: null,
  _collapsePaneString: "",
  _expandPaneString: ""
};





function ResultsPanelContainer() {
}

ResultsPanelContainer.prototype = Heritage.extend(WidgetMethods, {
  



  set anchor(aNode) {
    this._anchor = aNode;

    
    
    if (aNode) {
      if (!this._panel) {
        this._panel = document.createElement("panel");
        this._panel.id = "results-panel";
        this._panel.setAttribute("level", "top");
        this._panel.setAttribute("noautofocus", "true");
        this._panel.setAttribute("consumeoutsideclicks", "false");
        document.documentElement.appendChild(this._panel);
      }
      if (!this.widget) {
        this.widget = new SimpleListWidget(this._panel);
        this.autoFocusOnFirstItem = false;
        this.autoFocusOnSelection = false;
        this.maintainSelectionVisible = false;
      }
    }
    
    else {
      this._panel.remove();
      this._panel = null;
      this.widget = null;
    }
  },

  



  get anchor() {
    return this._anchor;
  },

  



  set hidden(aFlag) {
    if (aFlag) {
      this._panel.hidden = true;
      this._panel.hidePopup();
    } else {
      this._panel.hidden = false;
      this._panel.openPopup(this._anchor, this.position, this.left, this.top);
    }
  },

  



  get hidden()
    this._panel.state == "closed" ||
    this._panel.state == "hiding",

  


  clearView: function() {
    this.hidden = true;
    this.empty();
  },

  



  selectNext: function() {
    let nextIndex = this.selectedIndex + 1;
    if (nextIndex >= this.itemCount) {
      nextIndex = 0;
    }
    this.selectedItem = this.getItemAtIndex(nextIndex);
  },

  



  selectPrev: function() {
    let prevIndex = this.selectedIndex - 1;
    if (prevIndex < 0) {
      prevIndex = this.itemCount - 1;
    }
    this.selectedItem = this.getItemAtIndex(prevIndex);
  },

  









  _createItemView: function(aLabel, aBelowLabel, aBeforeLabel) {
    let container = document.createElement("vbox");
    container.className = "results-panel-item";

    let firstRowLabels = document.createElement("hbox");
    let secondRowLabels = document.createElement("hbox");

    if (aBeforeLabel) {
      let beforeLabelNode = document.createElement("label");
      beforeLabelNode.className = "plain results-panel-item-label-before";
      beforeLabelNode.setAttribute("value", aBeforeLabel);
      firstRowLabels.appendChild(beforeLabelNode);
    }

    let labelNode = document.createElement("label");
    labelNode.className = "plain results-panel-item-label";
    labelNode.setAttribute("value", aLabel);
    firstRowLabels.appendChild(labelNode);

    if (aBelowLabel) {
      let belowLabelNode = document.createElement("label");
      belowLabelNode.className = "plain results-panel-item-label-below";
      belowLabelNode.setAttribute("value", aBelowLabel);
      secondRowLabels.appendChild(belowLabelNode);
    }

    container.appendChild(firstRowLabels);
    container.appendChild(secondRowLabels);

    return container;
  },

  _anchor: null,
  _panel: null,
  position: RESULTS_PANEL_POPUP_POSITION,
  left: 0,
  top: 0
});
