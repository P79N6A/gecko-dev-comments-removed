




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
const BREAKPOINT_CONDITIONAL_POPUP_POSITION = "after_start";
const BREAKPOINT_CONDITIONAL_POPUP_OFFSET = 50; 
const FILTERED_SOURCES_POPUP_POSITION = "before_start";
const FILTERED_SOURCES_MAX_RESULTS = 10;
const GLOBAL_SEARCH_EXPAND_MAX_RESULTS = 50;
const GLOBAL_SEARCH_LINE_MAX_LENGTH = 300; 
const GLOBAL_SEARCH_ACTION_MAX_DELAY = 1500; 
const SEARCH_GLOBAL_FLAG = "!";
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
    this.ChromeGlobals.initialize();
    this.Sources.initialize();
    this.Filtering.initialize();
    this.FilteredSources.initialize();
    this.StackFrames.initialize();
    this.Breakpoints.initialize();
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
    this.ChromeGlobals.destroy();
    this.Sources.destroy();
    this.Filtering.destroy();
    this.FilteredSources.destroy();
    this.StackFrames.destroy();
    this.Breakpoints.destroy();
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

    this._togglePanesButton = document.getElementById("toggle-panes");
    this._stackframesAndBreakpoints = document.getElementById("stackframes+breakpoints");
    this._variablesAndExpressions = document.getElementById("variables+expressions");

    this._stackframesAndBreakpoints.setAttribute("width", Prefs.stackframesWidth);
    this._variablesAndExpressions.setAttribute("width", Prefs.variablesWidth);
    this.togglePanes({
      visible: Prefs.panesVisibleOnStartup,
      animated: false
    });
  },

  


  _destroyPanes: function DV__destroyPanes() {
    dumpn("Destroying the DebuggerView panes");

    Prefs.stackframesWidth = this._stackframesAndBreakpoints.getAttribute("width");
    Prefs.variablesWidth = this._variablesAndExpressions.getAttribute("width");

    this._togglePanesButton = null;
    this._stackframesAndBreakpoints = null;
    this._variablesAndExpressions = null;
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
      this._onEditorLoad();
      aCallback();
    }.bind(this));
  },

  



  _onEditorLoad: function DV__onEditorLoad() {
    dumpn("Finished loading the DebuggerView editor");

    DebuggerController.Breakpoints.initialize();
    window.dispatchEvent("Debugger:EditorLoaded", this.editor);
    this.editor.focus();
  },

  



  _destroyEditor: function DV__destroyEditor() {
    dumpn("Destroying the DebuggerView editor");

    DebuggerController.Breakpoints.destroy();
    window.dispatchEvent("Debugger:EditorUnloaded", this.editor);
    this.editor = null;
  },

  










  setEditorMode:
  function DV_setEditorMode(aUrl, aContentType = "", aTextContent = "") {
    if (!this.editor) {
      return;
    }
    dumpn("Setting the DebuggerView editor mode: " + aUrl +
          ", content type: " + aContentType);

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

  










  setEditorSource: function DV_setEditorSource(aSource, aOptions = {}) {
    if (!this.editor) {
      return;
    }

    dumpn("Setting the DebuggerView editor source: " + aSource.source.url +
          ", loaded: " + aSource.loaded +
          ", options: " + aOptions.toSource());

    
    if (!aSource.loaded) {
      this.editor.setMode(SourceEditor.MODES.TEXT);
      this.editor.setText(L10N.getStr("loadingText"));
      this.editor.resetUndo();

      
      DebuggerController.SourceScripts.getText(aSource, function(aUrl, aText) {
        this.setEditorSource(aSource, aOptions);
      }.bind(this));
    }
    
    else {
      if (this._editorSource != aSource) {
        
        if (aSource.text.length < SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
          this.setEditorMode(aSource.source.url, aSource.contentType, aSource.text);
        } else {
          this.editor.setMode(SourceEditor.MODES.TEXT);
        }
        this.editor.setText(aSource.text);
        this.editor.resetUndo();
      }
      this._editorSource = aSource;
      this.updateEditor();

      DebuggerView.Sources.selectedValue = aSource.source.url;
      DebuggerController.Breakpoints.updateEditorBreakpoints();

      
      if (aOptions.caretLine) {
        editor.setCaretPosition(aOptions.caretLine - 1);
      }
      if (aOptions.debugLine) {
        editor.setDebugLocation(aOptions.debugLine - 1);
      }
      if (aOptions.callback) {
        aOptions.callback(aSource);
      }
      
      window.dispatchEvent("Debugger:SourceShown", aSource);
    }
  },

  














  updateEditor: function DV_updateEditor(aUrl, aLine, aFlags = {}) {
    if (!this.editor) {
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
      updateLine(aLine);
    }
    
    else if (this.Sources.containsValue(aUrl) && !aFlags.noSwitch) {
      this.Sources.selectedValue = aUrl;
      updateLine(aLine);
    }
    
    else {
      updateLine(0);
    }

    
    
    function updateLine(aLine) {
      if (!aFlags.noCaret) {
        DebuggerView.editor.setCaretPosition(aLine - 1);
      }
      if (!aFlags.noDebug) {
        DebuggerView.editor.setDebugLocation(aLine - 1);
      }
    }
  },

  








  getEditorLine: function SS_getEditorLine(aLine) {
    let line = aLine || this.editor.getCaretPosition().line;
    let start = this.editor.getLineStart(line);
    let end = this.editor.getLineEnd(line);
    return this.editor.getText(start, end);
  },

  



  get panesHidden()
    this._togglePanesButton.hasAttribute("panesHidden"),

  









  togglePanes: function DV__togglePanes(aFlags = {}) {
    
    if (aFlags.visible == !this.panesHidden) {
      if (aFlags.callback) aFlags.callback();
      return;
    }

    
    function set() {
      if (aFlags.visible) {
        this._stackframesAndBreakpoints.style.marginLeft = "0";
        this._variablesAndExpressions.style.marginRight = "0";
        this._togglePanesButton.removeAttribute("panesHidden");
        this._togglePanesButton.setAttribute("tooltiptext", L10N.getStr("collapsePanes"));
      } else {
        let marginL = ~~(this._stackframesAndBreakpoints.getAttribute("width")) + 1;
        let marginR = ~~(this._variablesAndExpressions.getAttribute("width")) + 1;
        this._stackframesAndBreakpoints.style.marginLeft = -marginL + "px";
        this._variablesAndExpressions.style.marginRight = -marginR + "px";
        this._togglePanesButton.setAttribute("panesHidden", "true");
        this._togglePanesButton.setAttribute("tooltiptext", L10N.getStr("expandPanes"));
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
      this._stackframesAndBreakpoints.setAttribute("animated", "");
      this._variablesAndExpressions.setAttribute("animated", "");
    } else {
      this._stackframesAndBreakpoints.removeAttribute("animated");
      this._variablesAndExpressions.removeAttribute("animated");
    }

    if (aFlags.delayed) {
      window.setTimeout(set.bind(this), PANES_APPEARANCE_DELAY);
    } else {
      set.call(this);
    }
  },

  





  showPanesSoon: function DV__showPanesSoon(aCallback) {
    
    window.setTimeout(function() {
      DebuggerView.togglePanes({
        visible: true,
        animated: true,
        delayed: true,
        callback: aCallback
      });
    }, PANES_APPEARANCE_DELAY);
  },

  


  _handleTabNavigation: function DV__handleTabNavigation() {
    dumpn("Handling tab navigation in the DebuggerView");

    this.ChromeGlobals.empty();
    this.Sources.empty();
    this.Filtering.clearSearch();
    this.GlobalSearch.clearView();
    this.GlobalSearch.clearCache();
    this.StackFrames.empty();
    this.Breakpoints.empty();
    this.Breakpoints.unhighlightBreakpoint();
    this.Variables.empty();
    SourceUtils.clearLabelsCache();

    if (this.editor) {
      this.editor.setText("");
      this.editor.focus();
      this._editorSource = null;
    }
  },

  Toolbar: null,
  Options: null,
  ChromeGlobals: null,
  Sources: null,
  Filtering: null,
  StackFrames: null,
  Breakpoints: null,
  GlobalSearch: null,
  Variables: null,
  _editor: null,
  _editorSource: null,
  _togglePanesButton: null,
  _stackframesAndBreakpoints: null,
  _variablesAndExpressions: null,
  _isInitialized: false,
  _isDestroyed: false
};














function MenuItem(aLabel, aValue, aDescription, aAttachment) {
  this._label = aLabel + "";
  this._value = aValue + "";
  this._description = aDescription + "";
  this.attachment = aAttachment;
}

MenuItem.prototype = {
  



  get label() this._label,

  



  get value() this._value,

  



  get description() this._description,

  



  get target() this._target,

  _label: "",
  _value: "",
  _description: "",
  _target: null,
  finalize: null,
  attachment: null
};

























function MenuContainer() {}
const FIRST = 0;
const LAST = -1;

MenuContainer.prototype = {
  



  set node(aWidget) {
    this._container = aWidget;
    this._stagedItems = [];
    this._itemsByLabel = new Map();
    this._itemsByValue = new Map();
    this._itemsByElement = new Map();
  },

  



  get node() this._container,

  


































  push: function DVMC_push(aContents, aOptions = {}) {
    if (aContents instanceof Node || aContents instanceof Element) {
      aOptions.nsIDOMNode = aContents;
      aContents = [];
    }

    let [label, value, description] = aContents;
    let item = new MenuItem(label, value, description || "", aOptions.attachment);

    
    if (aOptions.staged) {
      this._stagedItems.push({ item: item, options: aOptions });
    }
    
    else if (!("index" in aOptions)) {
      return this._insertItemAt(this._findExpectedIndex(label), item, aOptions);
    }
    
    
    else {
      return this._insertItemAt(aOptions.index, item, aOptions);
    }
  },

  






  commit: function DVMC_commit(aOptions = {}) {
    let stagedItems = this._stagedItems;

    
    if (aOptions.sorted) {
      stagedItems.sort(function(a, b) a.item.label.toLowerCase() >
                                      b.item.label.toLowerCase());
    }
    
    for (let { item, options } of stagedItems) {
      this._insertItemAt(LAST, item, options);
    }
    
    this._stagedItems = [];
  },

  






  refresh: function DVMC_refresh() {
    let selectedValue = this.selectedValue;
    if (!selectedValue) {
      return false;
    }

    let entangledLabel = this.getItemByValue(selectedValue).label;

    this._container.setAttribute("label", entangledLabel);
    this._container.setAttribute("tooltiptext", selectedValue);
    return true;
  },

  





  remove: function DVMC__remove(aItem) {
    if (!aItem) {
      return;
    }
    this._container.removeChild(aItem.target);
    this._untangleItem(aItem);
  },

  


  empty: function DVMC_empty() {
    this._preferredValue = this.selectedValue;
    this._container.selectedItem = null;
    this._container.removeAllItems();
    this._container.setAttribute("label", this._emptyLabel);
    this._container.removeAttribute("tooltiptext");

    for (let [, item] of this._itemsByElement) {
      this._untangleItem(item);
    }

    this._itemsByLabel = new Map();
    this._itemsByValue = new Map();
    this._itemsByElement = new Map();
    this._stagedItems = [];
  },

  



  setUnavailable: function DVMC_setUnavailable() {
    this._container.setAttribute("label", this._unavailableLabel);
    this._container.removeAttribute("tooltiptext");
  },

  



  _emptyLabel: "",

  


  _unavailableLabel: "",

  





  toggleContents: function DVMC_toggleContents(aVisibleFlag) {
    for (let [, item] of this._itemsByElement) {
      item.target.hidden = !aVisibleFlag;
    }
  },

  








  containsLabel: function DVMC_containsLabel(aLabel) {
    return this._itemsByLabel.has(aLabel) ||
           this._stagedItems.some(function({item}) item.label == aLabel);
  },

  








  containsValue: function DVMC_containsValue(aValue) {
    return this._itemsByValue.has(aValue) ||
           this._stagedItems.some(function({item}) item.value == aValue);
  },

  










  containsTrimmedValue: function DVMC_containsTrimmedValue(aValue, aTrim) {
    let trimmedValue = aTrim(aValue);

    for (let [value] of this._itemsByValue) {
      if (aTrim(value) == trimmedValue) {
        return true;
      }
    }
    return this._stagedItems.some(function({item}) aTrim(item.value) == trimmedValue);
  },

  



  get preferredValue() this._preferredValue,

  



  get selectedIndex()
    this._container.selectedItem ?
    this.indexOfItem(this.selectedItem) : -1,

  



  get selectedItem()
    this._container.selectedItem ?
    this._itemsByElement.get(this._container.selectedItem) : null,

  



  get selectedLabel()
    this._container.selectedItem ?
    this._itemsByElement.get(this._container.selectedItem).label : null,

  



  get selectedValue()
    this._container.selectedItem ?
    this._itemsByElement.get(this._container.selectedItem).value : null,

  



  set selectedIndex(aIndex)
    this._container.selectedItem = this._container.getItemAtIndex(aIndex),

  



  set selectedItem(aItem)
    this._container.selectedItem = aItem ? aItem.target : null,

  



  set selectedLabel(aLabel) {
    let item = this._itemsByLabel.get(aLabel);
    if (item) {
      this._container.selectedItem = item.target;
    }
  },

  



  set selectedValue(aValue) {
    let item = this._itemsByValue.get(aValue);
    if (item) {
      this._container.selectedItem = item.target;
    }
  },

  







  getItemAtIndex: function DVMC_getItemAtIndex(aIndex) {
    return this.getItemForElement(this._container.getItemAtIndex(aIndex));
  },

  







  getItemByLabel: function DVMC_getItemByLabel(aLabel) {
    return this._itemsByLabel.get(aLabel);
  },

  







  getItemByValue: function DVMC_getItemByValue(aValue) {
    return this._itemsByValue.get(aValue);
  },

  







  getItemForElement: function DVMC_getItemForElement(aElement) {
    while (aElement) {
      let item = this._itemsByElement.get(aElement);
      if (item) {
        return item;
      }
      aElement = aElement.parentNode;
    }
    return null;
  },

  







  indexOfItem: function indexOfItem({target}) {
    let itemCount = this._itemsByElement.size;

    for (let i = 0; i < itemCount; i++) {
      if (this._container.getItemAtIndex(i) == target) {
        return i;
      }
    }
    return -1;
  },

  



  get labels() {
    let labels = [];
    for (let [label] of this._itemsByLabel) {
      labels.push(label);
    }
    return labels;
  },

  



  get values() {
    let values = [];
    for (let [value] of this._itemsByValue) {
      values.push(value);
    }
    return values;
  },

  



  get itemCount() this._itemsByElement.size,

  



  get visibleItems() {
    let items = [];
    for (let [element, item] of this._itemsByElement) {
      if (!element.hidden) {
        items.push(item);
      }
    }
    return items;
  },

  







  uniquenessQualifier: 1,

  







  isUnique: function DVMC_isUnique(aItem) {
    switch (this.uniquenessQualifier) {
      case 1:
        return !this._itemsByLabel.has(aItem.label) &&
               !this._itemsByValue.has(aItem.value);
      case 2:
        return !this._itemsByLabel.has(aItem.label) ||
               !this._itemsByValue.has(aItem.value);
      case 3:
        return !this._itemsByLabel.has(aItem.label);
      case 4:
        return !this._itemsByValue.has(aItem.value);
    }
    return false;
  },

  







  isEligible: function DVMC_isEligible(aItem) {
    return this.isUnique(aItem) &&
           aItem.label != "undefined" && aItem.label != "null" &&
           aItem.value != "undefined" && aItem.value != "null";
  },

  







  _findExpectedIndex: function DVMC__findExpectedIndex(aLabel) {
    let container = this._container;
    let itemCount = this.itemCount;

    for (let i = 0; i < itemCount; i++) {
      if (this.getItemAtIndex(i).label > aLabel) {
        return i;
      }
    }
    return itemCount;
  },

  













  _insertItemAt: function DVMC__insertItemAt(aIndex, aItem, aOptions) {
    if (!aOptions.relaxed && !this.isEligible(aItem)) {
      return null;
    }

    this._entangleItem(aItem, this._container.insertItemAt(aIndex,
      aOptions.nsIDOMNode || aItem.label,
      aItem.value,
      aItem.description,
      aOptions.attachment));

    
    if (aOptions.tooltip) {
      aItem._target.setAttribute("tooltiptext", aOptions.tooltip);
    }

    
    return aItem;
  },

  







  _entangleItem: function DVMC__entangleItem(aItem, aElement) {
    this._itemsByLabel.set(aItem.label, aItem);
    this._itemsByValue.set(aItem.value, aItem);
    this._itemsByElement.set(aElement, aItem);

    aItem._target = aElement;
  },

  





  _untangleItem: function DVMC__untangleItem(aItem) {
    if (aItem.finalize instanceof Function) {
      aItem.finalize(aItem);
    }

    this._itemsByLabel.delete(aItem.label);
    this._itemsByValue.delete(aItem.value);
    this._itemsByElement.delete(aItem.target);

    aItem._target = null;
  },

  







  decorateWidgetMethods: function DVMC_decorateWidgetMethods(aTarget) {
    let widget = this.node;
    let targetNode = widget[aTarget];

    widget.getAttribute = targetNode.getAttribute.bind(targetNode);
    widget.setAttribute = targetNode.setAttribute.bind(targetNode);
    widget.removeAttribute = targetNode.removeAttribute.bind(targetNode);
    widget.addEventListener = targetNode.addEventListener.bind(targetNode);
    widget.removeEventListener = targetNode.removeEventListener.bind(targetNode);
  },

  


  __iterator__: function DVMC_iterator() {
    for (let [, item] of this._itemsByElement) {
      yield item;
    }
  },

  _container: null,
  _stagedItems: null,
  _itemsByLabel: null,
  _itemsByValue: null,
  _itemsByElement: null,
  _preferredValue: null
};

















function StackList(aAssociatedNode) {
  this._parent = aAssociatedNode;

  
  this._list = document.createElement("vbox");
  this._parent.appendChild(this._list);

  
  
  MenuContainer.prototype.decorateWidgetMethods.call({ node: this }, "_parent");
}

StackList.prototype = {
  















  insertItemAt:
  function DVSL_insertItemAt(aIndex, aLabel, aValue, aDescription, aAttachment) {
    let list = this._list;
    let childNodes = list.childNodes;

    let element = document.createElement(this.itemType);
    this.itemFactory(element, aLabel, aValue, aAttachment);
    this._removeEmptyNotice();

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

    while (firstChild = list.firstChild) {
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

  



  itemType: "hbox",

  









  itemFactory: null,

  


  _appendPermaNotice: function DVSL__appendPermaNotice() {
    if (this._permaTextNode || !this._permaTextValue) {
      return;
    }

    let label = document.createElement("label");
    label.className = "empty list-item";
    label.setAttribute("value", this._permaTextValue);

    this._parent.insertBefore(label, this._list);
    this._permaTextNode = label;
  },

  


  _appendEmptyNotice: function DVSL__appendEmptyNotice() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }

    let label = document.createElement("label");
    label.className = "empty list-item";
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
