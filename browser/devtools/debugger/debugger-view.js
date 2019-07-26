




"use strict";

const SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 1048576; 
const SOURCE_URL_DEFAULT_MAX_LENGTH = 64; 
const STACK_FRAMES_SOURCE_URL_MAX_LENGTH = 15; 
const STACK_FRAMES_SOURCE_URL_TRIM_SECTION = "center";
const STACK_FRAMES_POPUP_SOURCE_URL_MAX_LENGTH = 32; 
const STACK_FRAMES_POPUP_SOURCE_URL_TRIM_SECTION = "center";
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
const DEFAULT_EDITOR_CONFIG = {
  mode: SourceEditor.MODES.TEXT,
  readOnly: true,
  showLineNumbers: true,
  showAnnotationRuler: true,
  showOverviewRuler: true
};




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
    this.Sources.initialize();
    this.WatchExpressions.initialize();
    this.GlobalSearch.initialize();
    this._initializeVariablesView();
    this._initializeEditor(deferred.resolve);

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
    this.Sources.destroy();
    this.WatchExpressions.destroy();
    this.GlobalSearch.destroy();
    this._destroyPanes();
    this._destroyEditor(deferred.resolve);

    return deferred.promise;
  },

  


  _initializePanes: function() {
    dumpn("Initializing the DebuggerView panes");

    this._sourcesPane = document.getElementById("sources-pane");
    this._instrumentsPane = document.getElementById("instruments-pane");
    this._instrumentsPaneToggleButton = document.getElementById("instruments-pane-toggle");

    this._collapsePaneString = L10N.getStr("collapsePanes");
    this._expandPaneString = L10N.getStr("expandPanes");

    this._sourcesPane.setAttribute("width", Prefs.sourcesWidth);
    this._instrumentsPane.setAttribute("width", Prefs.instrumentsWidth);
    this.toggleInstrumentsPane({ visible: Prefs.panesVisibleOnStartup });
  },

  


  _destroyPanes: function() {
    dumpn("Destroying the DebuggerView panes");

    Prefs.sourcesWidth = this._sourcesPane.getAttribute("width");
    Prefs.instrumentsWidth = this._instrumentsPane.getAttribute("width");

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
      getObjectClient: aObject => gThreadClient.pauseGrip(aObject)
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

    this.editor = new SourceEditor();
    this.editor.init(document.getElementById("editor"), DEFAULT_EDITOR_CONFIG, () => {
      this._loadingText = L10N.getStr("loadingText");
      this._onEditorLoad(aCallback);
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

  






  _setEditorText: function(aTextContent = "") {
    this.editor.setMode(SourceEditor.MODES.TEXT);
    this.editor.setText(aTextContent);
    this.editor.resetUndo();
  },

  










  _setEditorMode: function(aUrl, aContentType = "", aTextContent = "") {
    
    if (aTextContent.length >= SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
      this.editor.setMode(SourceEditor.MODES.TEXT);
      return;
    }

    if (aContentType) {
      if (/javascript/.test(aContentType)) {
        this.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
      } else {
        this.editor.setMode(SourceEditor.MODES.HTML);
      }
    } else if (aTextContent.match(/^\s*</)) {
      
      
      this.editor.setMode(SourceEditor.MODES.HTML);
    } else {
      
      if (/\.jsm?$/.test(SourceUtils.trimUrlQuery(aUrl))) {
        this.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
      } else {
        this.editor.setMode(SourceEditor.MODES.TEXT);
      }
    }
  },

  














  _setEditorSource: function(aSource, aFlags={}) {
    
    if (this._editorSource.url == aSource.url && !aFlags.force) {
      return this._editorSource.promise;
    }

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
        aLine += this.editor.getLineAtOffset(aFlags.charOffset);
      }
      if (aFlags.lineOffset) {
        aLine += aFlags.lineOffset;
      }
      if (!aFlags.noCaret) {
        this.editor.setCaretPosition(aLine - 1, aFlags.columnOffset);
      }
      if (!aFlags.noDebug) {
        this.editor.setDebugLocation(aLine - 1, aFlags.columnOffset);
      }
    });
  },

  








  getEditorLineText: function(aLine) {
    let line = aLine || this.editor.getCaretPosition().line;
    let start = this.editor.getLineStart(line);
    let end = this.editor.getLineEnd(line);
    return this.editor.getText(start, end);
  },

  



  get instrumentsPaneHidden()
    this._instrumentsPane.hasAttribute("pane-collapsed"),

  









  toggleInstrumentsPane: function(aFlags) {
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
  },

  





  showInstrumentsPane: function(aCallback) {
    DebuggerView.toggleInstrumentsPane({
      visible: true,
      animated: true,
      delayed: true,
      callback: aCallback
    });
  },

  


  _handleTabNavigation: function() {
    dumpn("Handling tab navigation in the DebuggerView");

    this.Filtering.clearSearch();
    this.FilteredSources.clearView();
    this.FilteredFunctions.clearView();
    this.GlobalSearch.clearView();
    this.ChromeGlobals.empty();
    this.StackFrames.empty();
    this.Sources.empty();
    this.Variables.empty();

    if (this.editor) {
      this.editor.setMode(SourceEditor.MODES.TEXT);
      this.editor.setText("");
      this.editor.resetUndo();
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
  Variables: null,
  WatchExpressions: null,
  editor: null,
  _editorSource: {},
  _loadingText: "",
  _sourcesPane: null,
  _instrumentsPane: null,
  _instrumentsPaneToggleButton: null,
  _collapsePaneString: "",
  _expandPaneString: "",
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
