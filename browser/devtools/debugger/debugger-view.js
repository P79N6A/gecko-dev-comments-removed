




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
const EDITOR_VARIABLE_HOVER_DELAY = 350; 
const EDITOR_VARIABLE_POPUP_OFFSET_X = 5; 
const EDITOR_VARIABLE_POPUP_OFFSET_Y = 0; 
const EDITOR_VARIABLE_POPUP_POSITION = "before_start";




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
    this.FilteredSources.initialize();
    this.FilteredFunctions.initialize();
    this.ChromeGlobals.initialize();
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
    this.FilteredSources.destroy();
    this.FilteredFunctions.destroy();
    this.ChromeGlobals.destroy();
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
      eval: DebuggerController.StackFrames.evaluate,
      lazyEmpty: true
    });

    
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

    function bindKey(func, key, modifiers = {}) {
      let key = document.getElementById(key).getAttribute("key");
      let shortcut = Editor.accel(key, modifiers);
      extraKeys[shortcut] = () => DebuggerView.Filtering[func]();
    }

    this.editor = new Editor({
      mode: Editor.modes.text,
      readOnly: true,
      lineNumbers: true,
      showAnnotationRuler: true,
      gutters: [ "breakpoints" ],
      extraKeys: extraKeys,
      contextMenu: "sourceEditorContextMenu"
    });

    this.editor.appendTo(document.getElementById("editor")).then(() => {
      this.editor.extend(DebuggerEditor);
      this._loadingText = L10N.getStr("loadingText");
      this._onEditorLoad(aCallback);
    });

    this.editor.on("gutterClick", (ev, line) => {
      if (this.editor.hasBreakpoint(line)) {
        this.editor.removeBreakpoint(line);
      } else {
        this.editor.addBreakpoint(line);
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
    
    if (this._editorSource.url == aSource.url && !aFlags.force) {
      return this._editorSource.promise;
    }
    let transportType = gClient.localTransport ? "_LOCAL" : "_REMOTE";
    let histogramId = "DEVTOOLS_DEBUGGER_DISPLAY_SOURCE" + transportType + "_MS";
    let histogram = Services.telemetry.getHistogramById(histogramId);
    let startTime = Date.now();

    let deferred = promise.defer();

    this._setEditorText(L10N.getStr("loadingText"));
    this._editorSource = { url: aSource.url, promise: deferred.promise };

    DebuggerController.SourceScripts.getText(aSource).then(([, aText]) => {
      
      
      if (this._editorSource.url != aSource.url) {
        return;
      }

      this._setEditorText(aText);
      this._setEditorMode(aSource.url, aSource.contentType, aText);

      
      DebuggerView.Sources.selectedValue = aSource.url;
      DebuggerController.Breakpoints.updateEditorBreakpoints();

      histogram.add(Date.now() - startTime);

      
      window.emit(EVENTS.SOURCE_SHOWN, aSource);
      deferred.resolve([aSource, aText]);
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

  





















  setEditorLocation: function(aUrl, aLine = 0, aFlags = {}) {
    
    if (!this.Sources.containsValue(aUrl)) {
      return promise.reject(new Error("Unknown source for the specified URL."));
    }

    
    
    if (!aLine) {
      let cachedFrames = DebuggerController.activeThread.cachedFrames;
      let currentDepth = DebuggerController.StackFrames.currentFrameDepth;
      let frame = cachedFrames[currentDepth];
      if (frame && frame.where.url == aUrl) {
        aLine = frame.where.line;
      }
    }

    let sourceItem = this.Sources.getItemByValue(aUrl);
    let sourceForm = sourceItem.attachment.source;

    
    
    return this._setEditorSource(sourceForm, aFlags).then(() => {
      
      
      if (aLine < 1) {
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
    this.FilteredSources.clearView();
    this.FilteredFunctions.clearView();
    this.GlobalSearch.clearView();
    this.ChromeGlobals.empty();
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
  },

  _startup: null,
  _shutdown: null,
  Toolbar: null,
  Options: null,
  Filtering: null,
  FilteredSources: null,
  FilteredFunctions: null,
  GlobalSearch: null,
  ChromeGlobals: null,
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











function ListWidget(aNode) {
  this._parent = aNode;

  
  this._list = document.createElement("vbox");
  this._parent.appendChild(this._list);

  
  
  ViewHelpers.delegateWidgetAttributeMethods(this, aNode);
  ViewHelpers.delegateWidgetEventMethods(this, aNode);
}

ListWidget.prototype = {
  



  itemType: "hbox",

  









  itemFactory: null,

  















  insertItemAt: function(aIndex, aLabel, aValue, aDescription, aAttachment) {
    let list = this._list;
    let childNodes = list.childNodes;

    let element = document.createElement(this.itemType);
    this.itemFactory(element, aAttachment, aLabel, aValue, aDescription);
    this._removeEmptyNotice();

    element.classList.add("list-widget-item");
    return list.insertBefore(element, childNodes[aIndex]);
  },

  







  getItemAtIndex: function(aIndex) {
    return this._list.childNodes[aIndex];
  },

  





  removeChild: function(aChild) {
    this._list.removeChild(aChild);

    if (this._selectedItem == aChild) {
      this._selectedItem = null;
    }
    if (!this._list.hasChildNodes()) {
      this._appendEmptyNotice();
    }
  },

  


  removeAllItems: function() {
    let parent = this._parent;
    let list = this._list;

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }
    parent.scrollTop = 0;
    parent.scrollLeft = 0;

    this._selectedItem = null;
    this._appendEmptyNotice();
  },

  



  get selectedItem() this._selectedItem,

  



  set selectedItem(aChild) {
    let childNodes = this._list.childNodes;

    if (!aChild) {
      this._selectedItem = null;
    }
    for (let node of childNodes) {
      if (node == aChild) {
        node.classList.add("selected");
        this._selectedItem = node;
      } else {
        node.classList.remove("selected");
      }
    }
  },

  



  set permaText(aValue) {
    if (this._permaTextNode) {
      this._permaTextNode.setAttribute("value", aValue);
    }
    this._permaTextValue = aValue;
    this._appendPermaNotice();
  },

  



  set emptyText(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
    this._appendEmptyNotice();
  },

  


  _appendPermaNotice: function() {
    if (this._permaTextNode || !this._permaTextValue) {
      return;
    }

    let label = document.createElement("label");
    label.className = "empty list-widget-item";
    label.setAttribute("value", this._permaTextValue);

    this._parent.insertBefore(label, this._list);
    this._permaTextNode = label;
  },

  


  _appendEmptyNotice: function() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }

    let label = document.createElement("label");
    label.className = "empty list-widget-item";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.appendChild(label);
    this._emptyTextNode = label;
  },

  


  _removeEmptyNotice: function() {
    if (!this._emptyTextNode) {
      return;
    }

    this._parent.removeChild(this._emptyTextNode);
    this._emptyTextNode = null;
  },

  _parent: null,
  _list: null,
  _selectedItem: null,
  _permaTextNode: null,
  _permaTextValue: "",
  _emptyTextNode: null,
  _emptyTextValue: ""
};





function ResultsPanelContainer() {
}

ResultsPanelContainer.prototype = Heritage.extend(WidgetMethods, {
  



  set anchor(aNode) {
    this._anchor = aNode;

    
    
    if (aNode) {
      if (!this._panel) {
        this._panel = document.createElement("panel");
        this._panel.className = "results-panel";
        this._panel.setAttribute("level", "top");
        this._panel.setAttribute("noautofocus", "true");
        this._panel.setAttribute("consumeoutsideclicks", "false");
        document.documentElement.appendChild(this._panel);
      }
      if (!this.widget) {
        this.widget = new ListWidget(this._panel);
        this.widget.itemType = "vbox";
        this.widget.itemFactory = this._createItemView;
      }
    }
    
    else {
      this._panel.remove();
      this._panel = null;
      this.widget = null;
    }
  },

  



  get anchor() this._anchor,

  



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

  













  _createItemView: function(aElementNode, aAttachment, aLabel, aValue, aDescription) {
    let labelsGroup = document.createElement("hbox");

    if (aDescription) {
      let preLabelNode = document.createElement("label");
      preLabelNode.className = "plain results-panel-item-pre";
      preLabelNode.setAttribute("value", aDescription);
      labelsGroup.appendChild(preLabelNode);
    }
    if (aLabel) {
      let labelNode = document.createElement("label");
      labelNode.className = "plain results-panel-item-name";
      labelNode.setAttribute("value", aLabel);
      labelsGroup.appendChild(labelNode);
    }

    let valueNode = document.createElement("label");
    valueNode.className = "plain results-panel-item-details";
    valueNode.setAttribute("value", aValue);

    aElementNode.className = "light results-panel-item";
    aElementNode.appendChild(labelsGroup);
    aElementNode.appendChild(valueNode);
  },

  _anchor: null,
  _panel: null,
  position: RESULTS_PANEL_POPUP_POSITION,
  left: 0,
  top: 0
});
