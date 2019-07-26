




"use strict";

const SOURCE_URL_DEFAULT_MAX_LENGTH = 64; 
const SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 1048576; 
const STACK_FRAMES_SOURCE_URL_MAX_LENGTH = 15; 
const STACK_FRAMES_SOURCE_URL_TRIM_SECTION = "center";
const STACK_FRAMES_POPUP_SOURCE_URL_MAX_LENGTH = 32; 
const STACK_FRAMES_POPUP_SOURCE_URL_TRIM_SECTION = "center";
const STACK_FRAMES_SCROLL_DELAY = 100; 
const PANES_APPEARANCE_DELAY = 50; 
const BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH = 1000; 
const BREAKPOINT_CONDITIONAL_POPUP_POSITION = "before_start";
const BREAKPOINT_CONDITIONAL_POPUP_OFFSET_X = 7; 
const BREAKPOINT_CONDITIONAL_POPUP_OFFSET_Y = -3; 
const RESULTS_PANEL_POPUP_POSITION = "before_end";
const RESULTS_PANEL_MAX_RESULTS = 10;
const GLOBAL_SEARCH_EXPAND_MAX_RESULTS = 50;
const GLOBAL_SEARCH_LINE_MAX_LENGTH = 300; 
const GLOBAL_SEARCH_ACTION_MAX_DELAY = 1500; 
const FUNCTION_SEARCH_ACTION_MAX_DELAY = 400; 
const SEARCH_GLOBAL_FLAG = "!";
const SEARCH_FUNCTION_FLAG = "@";
const SEARCH_TOKEN_FLAG = "#";
const SEARCH_LINE_FLAG = ":";
const SEARCH_VARIABLE_FLAG = "*";




let DebuggerView = {
  





  initialize: function DV_initialize(aCallback) {
    dumpn("Initializing the DebuggerView");

    this._initializeWindow();
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

    this.Variables = new VariablesView(document.getElementById("variables"));
    this.Variables.searchPlaceholder = L10N.getStr("emptyVariablesFilterText");
    this.Variables.emptyText = L10N.getStr("emptyVariablesText");
    this.Variables.onlyEnumVisible = Prefs.variablesOnlyEnumVisible;
    this.Variables.searchEnabled = Prefs.variablesSearchboxVisible;
    this.Variables.eval = DebuggerController.StackFrames.evaluate;
    this.Variables.lazyEmpty = true;

    this._initializeEditor(aCallback);
  },

  





  destroy: function DV_destroy(aCallback) {
    dumpn("Destroying the DebuggerView");

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

    this._destroyWindow();
    this._destroyPanes();
    this._destroyEditor();
    aCallback();
  },

  


  _initializeWindow: function DV__initializeWindow() {
    dumpn("Initializing the DebuggerView window");

    let isRemote = window._isRemoteDebugger;
    let isChrome = window._isChromeDebugger;

    if (isRemote || isChrome) {
      window.moveTo(Prefs.windowX, Prefs.windowY);
      window.resizeTo(Prefs.windowWidth, Prefs.windowHeight);

      if (isRemote) {
        document.title = L10N.getStr("remoteDebuggerWindowTitle");
      } else {
        document.title = L10N.getStr("chromeDebuggerWindowTitle");
      }
    }
  },

  


  _destroyWindow: function DV__destroyWindow() {
    dumpn("Destroying the DebuggerView window");

    if (window._isRemoteDebugger || window._isChromeDebugger) {
      Prefs.windowX = window.screenX;
      Prefs.windowY = window.screenY;
      Prefs.windowWidth = window.outerWidth;
      Prefs.windowHeight = window.outerHeight;
    }
  },

  


  _initializePanes: function DV__initializePanes() {
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

  


  _destroyPanes: function DV__destroyPanes() {
    dumpn("Destroying the DebuggerView panes");

    Prefs.sourcesWidth = this._sourcesPane.getAttribute("width");
    Prefs.instrumentsWidth = this._instrumentsPane.getAttribute("width");

    this._sourcesPane = null;
    this._instrumentsPane = null;
    this._instrumentsPaneToggleButton = null;
  },

  





  _initializeEditor: function DV__initializeEditor(aCallback) {
    dumpn("Initializing the DebuggerView editor");

    let placeholder = document.getElementById("editor");
    let config = {
      mode: SourceEditor.MODES.JAVASCRIPT,
      readOnly: true,
      showLineNumbers: true,
      showAnnotationRuler: true,
      showOverviewRuler: true
    };

    this.editor = new SourceEditor();
    this.editor.init(placeholder, config, function() {
      this._loadingText = L10N.getStr("loadingText");
      this._onEditorLoad();
      aCallback();
    }.bind(this));
  },

  



  _onEditorLoad: function DV__onEditorLoad() {
    dumpn("Finished loading the DebuggerView editor");

    DebuggerController.Breakpoints.initialize();
    window.dispatchEvent(document, "Debugger:EditorLoaded", this.editor);
    this.editor.focus();
  },

  



  _destroyEditor: function DV__destroyEditor() {
    dumpn("Destroying the DebuggerView editor");

    DebuggerController.Breakpoints.destroy();
    window.dispatchEvent(document, "Debugger:EditorUnloaded", this.editor);
  },

  










  setEditorMode: function DV_setEditorMode(aUrl, aContentType = "", aTextContent = "") {
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

  








  set editorSource(aSource) {
    if (!this._isInitialized || this._isDestroyed || this._editorSource == aSource) {
      return;
    }

    dumpn("Setting the DebuggerView editor source: " + aSource.url +
          ", loaded: " + aSource.loaded);

    this.editor.setMode(SourceEditor.MODES.TEXT);
    this.editor.setText(L10N.getStr("loadingText"));
    this.editor.resetUndo();
    this._editorSource = aSource;

    
    if (!aSource.loaded) {
      DebuggerController.SourceScripts.getText(aSource, set.bind(this));
    }
    
    else {
      set.call(this, aSource);
    }

    
    
    function set(aSource) {
      
      
      if (this._editorSource != aSource) {
        return;
      }

      
      if (aSource.text.length < SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
        this.setEditorMode(aSource.url, aSource.contentType, aSource.text);
      } else {
        this.editor.setMode(SourceEditor.MODES.TEXT);
      }
      this.editor.setText(aSource.text);
      this.editor.resetUndo();

      
      
      this.updateEditor();

      
      DebuggerView.Sources.selectedValue = aSource.url;
      DebuggerController.Breakpoints.updateEditorBreakpoints();

      
      window.dispatchEvent(document, "Debugger:SourceShown", aSource);
    }
  },

  





  get editorSource() this._editorSource,

  

















  updateEditor: function DV_updateEditor(aUrl, aLine, aFlags = {}) {
    if (!this._isInitialized || this._isDestroyed) {
      return;
    }
    
    
    if (!aUrl && !aLine) {
      let cachedFrames = DebuggerController.activeThread.cachedFrames;
      let currentFrame = DebuggerController.StackFrames.currentFrame;
      let frame = cachedFrames[currentFrame];
      if (frame) {
        let { url, line } = frame.where;
        this.updateEditor(url, line, { noSwitch: true });
      }
      return;
    }

    dumpn("Updating the DebuggerView editor: " + aUrl + " @ " + aLine +
          ", flags: " + aFlags.toSource());

    
    if (this.Sources.selectedValue == aUrl) {
      set(aLine);
    }
    
    else if (this.Sources.containsValue(aUrl) && !aFlags.noSwitch) {
      this.Sources.selectedValue = aUrl;
      set(aLine);
    }
    
    else {
      set(0);
    }

    
    
    function set(aLine) {
      let editor = DebuggerView.editor;

      
      if (aFlags.charOffset) {
        aLine += editor.getLineAtOffset(aFlags.charOffset);
      }
      if (aFlags.lineOffset) {
        aLine += aFlags.lineOffset;
      }
      if (!aFlags.noCaret) {
        editor.setCaretPosition(aLine - 1, aFlags.columnOffset);
      }
      if (!aFlags.noDebug) {
        editor.setDebugLocation(aLine - 1, aFlags.columnOffset);
      }
    }
  },

  








  getEditorLine: function DV_getEditorLine(aLine) {
    let line = aLine || this.editor.getCaretPosition().line;
    let start = this.editor.getLineStart(line);
    let end = this.editor.getLineEnd(line);
    return this.editor.getText(start, end);
  },

  





  getEditorSelection: function DV_getEditorSelection() {
    let selection = this.editor.getSelection();
    return this.editor.getText(selection.start, selection.end);
  },

  



  get instrumentsPaneHidden()
    this._instrumentsPaneToggleButton.hasAttribute("toggled"),

  









  toggleInstrumentsPane: function DV__toggleInstrumentsPane(aFlags = {}) {
    
    if (aFlags.visible == !this.instrumentsPaneHidden) {
      if (aFlags.callback) aFlags.callback();
      return;
    }

    
    function set() {
      if (aFlags.visible) {
        this._instrumentsPane.style.marginLeft = "0";
        this._instrumentsPane.style.marginRight = "0";
        this._instrumentsPaneToggleButton.removeAttribute("toggled");
        this._instrumentsPaneToggleButton.setAttribute("tooltiptext", this._collapsePaneString);
      } else {
        let margin = ~~(this._instrumentsPane.getAttribute("width")) + 1;
        this._instrumentsPane.style.marginLeft = -margin + "px";
        this._instrumentsPane.style.marginRight = -margin + "px";
        this._instrumentsPaneToggleButton.setAttribute("toggled", "true");
        this._instrumentsPaneToggleButton.setAttribute("tooltiptext", this._expandPaneString);
      }

      if (aFlags.animated) {
        
        
        
        window.addEventListener("transitionend", function onEvent() {
          window.removeEventListener("transitionend", onEvent, false);
          DebuggerView.updateEditor();

          
          if (aFlags.callback) aFlags.callback();
        }, false);
      } else {
        
        if (aFlags.callback) aFlags.callback();
      }
    }

    if (aFlags.animated) {
      this._instrumentsPane.setAttribute("animated", "");
    } else {
      this._instrumentsPane.removeAttribute("animated");
    }

    if (aFlags.delayed) {
      window.setTimeout(set.bind(this), PANES_APPEARANCE_DELAY);
    } else {
      set.call(this);
    }
  },

  





  showInstrumentsPane: function DV__showInstrumentsPane(aCallback) {
    DebuggerView.toggleInstrumentsPane({
      visible: true,
      animated: true,
      delayed: true,
      callback: aCallback
    });
  },

  


  _handleTabNavigation: function DV__handleTabNavigation() {
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
      this.editor.setText("");
      this.editor.focus();
      this._editorSource = null;
    }
  },

  Toolbar: null,
  Options: null,
  Filtering: null,
  FilteredSources: null,
  ChromeGlobals: null,
  StackFrames: null,
  Sources: null,
  WatchExpressions: null,
  GlobalSearch: null,
  Variables: null,
  _editor: null,
  _editorSource: null,
  _loadingText: "",
  _sourcesPane: null,
  _instrumentsPane: null,
  _instrumentsPaneToggleButton: null,
  _collapsePaneString: "",
  _expandPaneString: "",
  _isInitialized: false,
  _isDestroyed: false
};

















function ListWidget(aAssociatedNode) {
  this._parent = aAssociatedNode;

  
  this._list = document.createElement("vbox");
  this._parent.appendChild(this._list);

  
  
  ViewHelpers.delegateWidgetAttributeMethods(this, aAssociatedNode);
  ViewHelpers.delegateWidgetEventMethods(this, aAssociatedNode);
}

ListWidget.prototype = {
  



  itemType: "hbox",

  









  itemFactory: null,

  















  insertItemAt:
  function DVSL_insertItemAt(aIndex, aLabel, aValue, aDescription, aAttachment) {
    let list = this._list;
    let childNodes = list.childNodes;

    let element = document.createElement(this.itemType);
    this.itemFactory(element, aAttachment, aLabel, aValue, aDescription);
    this._removeEmptyNotice();

    element.classList.add("list-widget-item");
    return list.insertBefore(element, childNodes[aIndex]);
  },

  







  getItemAtIndex: function DVSL_getItemAtIndex(aIndex) {
    return this._list.childNodes[aIndex];
  },

  





  removeChild: function DVSL__removeChild(aChild) {
    this._list.removeChild(aChild);

    if (this._selectedItem == aChild) {
      this._selectedItem = null;
    }
    if (!this._list.hasChildNodes()) {
      this._appendEmptyNotice();
    }
  },

  


  removeAllItems: function DVSL_removeAllItems() {
    let parent = this._parent;
    let list = this._list;
    let firstChild;

    while ((firstChild = list.firstChild)) {
      list.removeChild(firstChild);
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

  


  _appendPermaNotice: function DVSL__appendPermaNotice() {
    if (this._permaTextNode || !this._permaTextValue) {
      return;
    }

    let label = document.createElement("label");
    label.className = "empty list-widget-item";
    label.setAttribute("value", this._permaTextValue);

    this._parent.insertBefore(label, this._list);
    this._permaTextNode = label;
  },

  


  _appendEmptyNotice: function DVSL__appendEmptyNotice() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }

    let label = document.createElement("label");
    label.className = "empty list-widget-item";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.appendChild(label);
    this._emptyTextNode = label;
  },

  


  _removeEmptyNotice: function DVSL__removeEmptyNotice() {
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
  this._createItemView = this._createItemView.bind(this);
}

create({ constructor: ResultsPanelContainer, proto: MenuContainer.prototype }, {
  onClick: null,
  onSelect: null,

  



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
      if (!this.node) {
        this.node = new ListWidget(this._panel);
        this.node.itemType = "vbox";
        this.node.itemFactory = this._createItemView;
        this.node.addEventListener("click", this.onClick, false);
      }
    }
    
    else {
      if (this._panel) {
        document.documentElement.removeChild(this._panel);
        this._panel = null;
      }
      if (this.node) {
        this.node.removeEventListener("click", this.onClick, false);
        this.node = null;
      }
    }
  },

  



  get anchor() this._anchor,

  



  set options(aOptions) {
    this._top = aOptions.top;
    this._left = aOptions.left;
    this._position = aOptions.position;
  },

  



  get options() ({
    top: this._top,
    left: this._left,
    position: this._position
  }),

  



  set hidden(aFlag) {
    if (aFlag) {
      this._panel.hidePopup();
    } else {
      this._panel.openPopup(this._anchor, this._position, this._left, this._top);
      this.anchor.focus();
    }
  },

  



  get hidden()
    this._panel.state == "closed" ||
    this._panel.state == "hiding",

  


  clearView: function RPC_clearView() {
    this.hidden = true;
    this.empty();
    window.dispatchEvent(document, "Debugger:ResultsPanelContainer:ViewCleared");
  },

  


  focusNext: function RPC_focusNext() {
    let nextIndex = this.selectedIndex + 1;
    if (nextIndex >= this.itemCount) {
      nextIndex = 0;
    }
    this.select(this.getItemAtIndex(nextIndex));
  },

  


  focusPrev: function RPC_focusPrev() {
    let prevIndex = this.selectedIndex - 1;
    if (prevIndex < 0) {
      prevIndex = this.itemCount - 1;
    }
    this.select(this.getItemAtIndex(prevIndex));
  },

  





  select: function RPC_select(aItem) {
    if (typeof aItem == "number") {
      this.select(this.getItemAtIndex(aItem));
      return;
    }

    
    
    this.selectedItem = aItem;

    
    
    if (this.onSelect) {
      this.onSelect({ target: aItem.target });
    }
  },

  













  _createItemView:
  function RPC__createItemView(aElementNode, aAttachment, aLabel, aValue, aDescription) {
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
  _position: RESULTS_PANEL_POPUP_POSITION,
  _left: 0,
  _top: 0
});




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

      if (!result) {
        return false;
      }
      if ((parts = input.value.split(":")).length == 2) {
        let [host, port] = parts;

        if (host.length && port.length) {
          this.remote = { host: host, port: port, auto: check.value };
          return true;
        }
      }
    }
  }
};
