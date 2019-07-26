




"use strict";

const SOURCE_URL_MAX_LENGTH = 64; 
const SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 1048576; 
const PANES_APPEARANCE_DELAY = 50; 
const BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH = 1000; 
const GLOBAL_SEARCH_LINE_MAX_LENGTH = 300; 
const GLOBAL_SEARCH_ACTION_DELAY = 150; 
const SEARCH_GLOBAL_FLAG = "!";
const SEARCH_LINE_FLAG = ":";
const SEARCH_TOKEN_FLAG = "#";




let DebuggerView = {
  





  initialize: function DV_initialize(aCallback) {
    dumpn("Initializing the DebuggerView");
    this.Toolbar.initialize();
    this.Options.initialize();
    this.ChromeGlobals.initialize();
    this.Sources.initialize();
    this.Filtering.initialize();
    this.StackFrames.initialize();
    this.Breakpoints.initialize();
    this.GlobalSearch.initialize();

    this.Variables = new VariablesView(document.getElementById("variables"));
    this.Variables.emptyText = L10N.getStr("emptyVariablesText");
    this.Variables.nonEnumVisible = Prefs.nonEnumVisible;
    this.Variables.eval = DebuggerController.StackFrames.evaluate;
    this.Variables.lazyEmpty = true;

    this._initializePanes();
    this._initializeEditor(aCallback)
    this._isInitialized = true;
  },

  





  destroy: function DV_destroy(aCallback) {
    dumpn("Destroying the DebuggerView");
    this.Toolbar.destroy();
    this.Options.destroy();
    this.ChromeGlobals.destroy();
    this.Sources.destroy();
    this.Filtering.destroy();
    this.StackFrames.destroy();
    this.Breakpoints.destroy();
    this.GlobalSearch.destroy();

    this._destroyPanes();
    this._destroyEditor();
    aCallback();
  },

  


  _initializePanes: function DV__initializePanes() {
    dumpn("Initializing the DebuggerView panes");

    this._togglePanesButton = document.getElementById("toggle-panes");
    this._stackframesAndBreakpoints = document.getElementById("stackframes+breakpoints");
    this._variables = document.getElementById("variables");

    this._stackframesAndBreakpoints.setAttribute("width", Prefs.stackframesWidth);
    this._variables.setAttribute("width", Prefs.variablesWidth);
    this.togglePanes({ visible: false, animated: false, silent: true });
  },

  


  _destroyPanes: function DV__initializePanes() {
    dumpn("Destroying the DebuggerView panes");

    Prefs.stackframesWidth = this._stackframesAndBreakpoints.getAttribute("width");
    Prefs.variablesWidth = this._variables.getAttribute("width");

    this._togglePanesButton = null;
    this._stackframesAndBreakpoints = null;
    this._variables = null;
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
    this.editor.focus();
  },

  



  _destroyEditor: function DV__destroyEditor() {
    dumpn("Destroying the DebuggerView editor");

    DebuggerController.Breakpoints.destroy();
    this.editor = null;
  },

  








  setEditorMode: function DV_setEditorMode(aUrl, aContentType) {
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
    } else {
      
      if (/\.jsm?$/.test(SourceUtils.trimUrlQuery(aUrl))) {
        this.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
      } else {
        this.editor.setMode(SourceEditor.MODES.HTML);
      }
    }
  },

  










  setEditorSource: function DV_setEditorSource(aSource, aOptions = {}) {
    if (!this.editor) {
      return;
    }
    dumpn("Setting the DebuggerView editor source: " + aSource.url +
          ", loaded: " + aSource.loaded +
          ", options: " + aOptions.toSource());

    
    if (!aSource.loaded) {
      this.editor.setMode(SourceEditor.MODES.TEXT);
      this.editor.setText(L10N.getStr("loadingText"));
      this.editor.resetUndo();

      
      DebuggerController.SourceScripts.getText(aSource, function(aUrl, aText) {
        aSource.loaded = true;
        aSource.text = aText;
        this.setEditorSource(aSource, aOptions);
      }.bind(this));
    }
    
    else {
      if (aSource.text.length < SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
        this.setEditorMode(aSource.url, aSource.contentType);
      }
      this.editor.setText(aSource.text);
      this.editor.resetUndo();
      this.updateEditor();

      DebuggerView.Sources.selectedValue = aSource.url;
      DebuggerController.Breakpoints.updateEditorBreakpoints();

      
      if (aOptions.targetLine) {
        editor.setCaretPosition(aOptions.targetLine - 1);
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
    this.stackframesAndBreakpointsHidden && this.variablesHidden,

  



  get stackframesAndBreakpointsHidden()
    !!this._togglePanesButton.getAttribute("stackframesAndBreakpointsHidden"),

  



  get variablesHidden()
    !!this._togglePanesButton.getAttribute("variablesHidden"),

  








  togglePanes: function DV__togglePanes(aFlags = {}) {
    this._toggleStackframesAndBreakpointsPane(aFlags);
    this._toggleVariablesPane(aFlags);
  },

  



  showPanesIfPreffered: function DV_showPanesIfPreffered() {
    let self = this;

    
    window.setTimeout(function() {
      let target;

      if (Prefs.stackframesPaneVisible && self.stackframesAndBreakpointsHidden) {
        self._toggleStackframesAndBreakpointsPane({
          visible: true,
          animated: true,
          silent: true
        });
        target = self._stackframesAndBreakpoints;
      }
      if (Prefs.variablesPaneVisible && self.variablesHidden) {
        self._toggleVariablesPane({
          visible: true,
          animated: true,
          silent: true
        });
        target = self._variables;
      }
      
      
      
      if (target) {
        target.addEventListener("transitionend", function onEvent() {
          target.removeEventListener("transitionend", onEvent, false);
          self.updateEditor();
        }, false);
      }
    }, PANES_APPEARANCE_DELAY);
  },

  



  _toggleStackframesAndBreakpointsPane:
  function DV__toggleStackframesAndBreakpointsPane(aFlags) {
    if (aFlags.animated) {
      this._stackframesAndBreakpoints.setAttribute("animated", "");
    } else {
      this._stackframesAndBreakpoints.removeAttribute("animated");
    }
    if (aFlags.visible) {
      this._stackframesAndBreakpoints.style.marginLeft = "0";
      this._togglePanesButton.removeAttribute("stackframesAndBreakpointsHidden");
      this._togglePanesButton.setAttribute("tooltiptext", L10N.getStr("collapsePanes"));
    } else {
      let margin = parseInt(this._stackframesAndBreakpoints.getAttribute("width")) + 1;
      this._stackframesAndBreakpoints.style.marginLeft = -margin + "px";
      this._togglePanesButton.setAttribute("stackframesAndBreakpointsHidden", "true");
      this._togglePanesButton.setAttribute("tooltiptext", L10N.getStr("expandPanes"));
    }
    if (!aFlags.silent) {
      Prefs.stackframesPaneVisible = !!aFlags.visible;
    }
  },

  



  _toggleVariablesPane:
  function DV__toggleVariablesPane(aFlags) {
    if (aFlags.animated) {
      this._variables.setAttribute("animated", "");
    } else {
      this._variables.removeAttribute("animated");
    }
    if (aFlags.visible) {
      this._variables.style.marginRight = "0";
      this._togglePanesButton.removeAttribute("variablesHidden");
      this._togglePanesButton.setAttribute("tooltiptext", L10N.getStr("collapsePanes"));
    } else {
      let margin = parseInt(this._variables.getAttribute("width")) + 1;
      this._variables.style.marginRight = -margin + "px";
      this._togglePanesButton.setAttribute("variablesHidden", "true");
      this._togglePanesButton.setAttribute("tooltiptext", L10N.getStr("expandPanes"));
    }
    if (!aFlags.silent) {
      Prefs.variablesPaneVisible = !!aFlags.visible;
    }
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
    this.Variables.empty();
    SourceUtils.clearLabelsCache();

    if (this.editor) {
      this.editor.setText("");
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
  _togglePanesButton: null,
  _stackframesAndBreakpoints: null,
  _variables: null,
  _isInitialized: false,
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

































function MenuContainer(aContainerNode) {
  this._container = aContainerNode;
  this._stagedItems = [];
  this._itemsByLabel = new Map();
  this._itemsByValue = new Map();
  this._itemsByElement = new Map();
}

MenuContainer.prototype = {
  































  push: function DVMC_push(aLabel, aValue, aOptions = {}) {
    let item = new MenuItem(
      aLabel, aValue, aOptions.description, aOptions.attachment);

    
    if (!aOptions.forced) {
      this._stagedItems.push(item);
    }
    
    else if (!aOptions.unsorted) {
      return this._insertItemAt(this._findExpectedIndex(aLabel), item, aOptions);
    }
    
    else {
      return this._appendItem(item, aOptions);
    }
  },

  






  commit: function DVMC_commit(aOptions = {}) {
    let stagedItems = this._stagedItems;

    
    if (!aOptions.unsorted) {
      stagedItems.sort(function(a, b) a.label.toLowerCase() > b.label.toLowerCase());
    }
    
    for (let item of stagedItems) {
      this._appendItem(item, aOptions);
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
    this._container.removeChild(aItem.target);
    this._untangleItem(aItem);
  },

  


  empty: function DVMC_empty() {
    this._preferredValue = this.selectedValue;
    this._container.selectedIndex = -1;
    this._container.setAttribute("label", this._emptyLabel);
    this._container.removeAttribute("tooltiptext");
    this._container.removeAllItems();

    for (let [_, item] of this._itemsByElement) {
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

  








  containsLabel: function DVMC_containsLabel(aLabel) {
    return this._itemsByLabel.has(aLabel) ||
           this._stagedItems.some(function(o) o.label == aLabel);
  },

  








  containsValue: function DVMC_containsValue(aValue) {
    return this._itemsByValue.has(aValue) ||
           this._stagedItems.some(function(o) o.value == aValue);
  },

  










  containsTrimmedValue:
  function DVMC_containsTrimmedValue(aValue,
                                     aTrim = SourceUtils.trimUrlQuery) {
    let trimmedValue = aTrim(aValue);

    for (let [value] of this._itemsByValue) {
      if (aTrim(value) == trimmedValue) {
        return true;
      }
    }
    return this._stagedItems.some(function(o) aTrim(o.value) == trimmedValue);
  },

  



  get preferredValue() this._preferredValue,

  



  get selectedIndex() this._container.selectedIndex,

  



  get selectedItem()
    this._container.selectedItem ?
    this._itemsByElement.get(this._container.selectedItem) : null,

  



  get selectedLabel()
    this._container.selectedItem ?
    this._itemsByElement.get(this._container.selectedItem).label : null,

  



  get selectedValue()
    this._container.selectedItem ?
    this._itemsByElement.get(this._container.selectedItem).value : null,

  



  set selectedIndex(aIndex) this._container.selectedIndex = aIndex,

  



  set selectedItem(aItem) this._container.selectedItem = aItem.target,

  



  set selectedLabel(aLabel) {
    let item = this._itemsByLabel.get(aValue);
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

  







  getItemByLabel: function DVMC_getItemByLabel(aLabel) {
    return this._itemsByLabel.get(aLabel);
  },

  







  getItemByValue: function DVMC_getItemByValue(aValue) {
    return this._itemsByValue.get(aValue);
  },

  







  getItemForElement:
  function DVMC_getItemForElement(aElement) {
    while (aElement) {
      let item = this._itemsByElement.get(aElement);
      if (item) {
        return item;
      }
      aElement = aElement.parentNode;
    }
    return null;
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

  



  get visibleItems() {
    let count = 0;
    for (let [element] of this._itemsByElement) {
      count += element.hidden ? 0 : 1;
    }
    return count;
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
    let itemCount = container.itemCount;

    for (let i = 0; i < itemCount; i++) {
      if (this.getItemForElement(container.getItemAtIndex(i)).label > aLabel) {
        return i;
      }
    }
    return itemCount;
  },

  










  _appendItem:
  function DVMC__appendItem(aItem, aOptions = {}) {
    if (!aOptions.relaxed && !this.isEligible(aItem)) {
      return null;
    }

    return this._entangleItem(aItem, this._container.appendItem(
      aItem.label, aItem.value, "", aOptions.attachment));
  },

  












  _insertItemAt:
  function DVMC__insertItemAt(aIndex, aItem, aOptions) {
    if (!aOptions.relaxed && !this.isEligible(aItem)) {
      return null;
    }

    return this._entangleItem(aItem, this._container.insertItemAt(
      aIndex, aItem.label, aItem.value, "", aOptions.attachment));
  },

  









  _entangleItem: function DVMC__entangleItem(aItem, aElement) {
    this._itemsByLabel.set(aItem.label, aItem);
    this._itemsByValue.set(aItem.value, aItem);
    this._itemsByElement.set(aElement, aItem);

    aItem._target = aElement;
    return aItem;
  },

  







  _untangleItem: function DVMC__untangleItem(aItem) {
    if (aItem.finalize instanceof Function) {
      aItem.finalize(aItem);
    }

    this._itemsByLabel.delete(aItem.label);
    this._itemsByValue.delete(aItem.value);
    this._itemsByElement.delete(aItem.target);

    aItem._target = null;
    return aItem;
  },

  


  __iterator__: function DVMC_iterator() {
    for (let [_, item] of this._itemsByElement) {
      yield item;
    }
  },

  _container: null,
  _stagedItems: null,
  _itemsByLabel: null,
  _itemsByValue: null,
  _itemsByElement: null,
  _preferredValue: null,
  _emptyLabel: "",
  _unavailableLabel: ""
};















function StackList(aAssociatedNode) {
  this._parent = aAssociatedNode;
  this._appendEmptyNotice();

  
  this._list = document.createElement("vbox");
  this._parent.appendChild(this._list);
}

StackList.prototype = {
  













  appendItem:
  function DVSL_appendItem(aLabel, aValue, aDescription, aAttachment) {
    return this.insertItemAt(
      Number.MAX_VALUE, aLabel, aValue, aDescription, aAttachment);
  },

  















  insertItemAt:
  function DVSL_insertItemAt(aIndex, aLabel, aValue, aDescription, aAttachment) {
    let list = this._list;
    let childNodes = list.childNodes;

    let element = document.createElement(this.itemType);
    this._createItemView(element, aLabel, aValue, aAttachment);
    this._removeEmptyNotice();

    return list.insertBefore(element, childNodes[aIndex]);
  },

  







  getItemAtIndex: function DVSL_getItemAtIndex(aIndex) {
    return this._list.childNodes[aIndex];
  },

  





  removeChild: function DVSL__removeChild(aChild) {
    this._list.removeChild(aChild);

    if (!this.itemCount) {
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
    this._selectedIndex = -1;
    this._appendEmptyNotice();
  },

  



  get itemCount() this._list.childNodes.length,

  



  get selectedIndex() this._selectedIndex,

  




  set selectedIndex(aIndex) this.selectedItem = this._list.childNodes[aIndex],

  



  get selectedItem() this._selectedItem,

  



  set selectedItem(aChild) {
    let childNodes = this._list.childNodes;

    if (!aChild) {
      this._selectedItem = null;
      this._selectedIndex = -1;
    }
    for (let node of childNodes) {
      if (node == aChild) {
        node.classList.add("selected");
        this._selectedIndex = Array.indexOf(childNodes, node);
        this._selectedItem = node;
      } else {
        node.classList.remove("selected");
      }
    }
  },

  







  getAttribute: function DVSL_setAttribute(aName) {
    return this._parent.getAttribute(aName);
  },

  







  setAttribute: function DVSL_setAttribute(aName, aValue) {
    this._parent.setAttribute(aName, aValue);
  },

  





  removeAttribute: function DVSL_removeAttribute(aName) {
    this._parent.removeAttribute(aName);
  },

  









  addEventListener:
  function DVSL_addEventListener(aName, aCallback, aBubbleFlag) {
    this._parent.addEventListener(aName, aCallback, aBubbleFlag);
  },

  









  removeEventListener:
  function DVSL_removeEventListener(aName, aCallback, aBubbleFlag) {
    this._parent.removeEventListener(aName, aCallback, aBubbleFlag);
  },

  



  set emptyText(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
  },

  



  itemType: "hbox",

  



  set itemFactory(aCallback) this._createItemView = aCallback,

  









  _createItemView: function DVSL__createItemView(aElementNode, aLabel, aValue) {
    let labelNode = document.createElement("label");
    let valueNode = document.createElement("label");
    let spacer = document.createElement("spacer");

    labelNode.setAttribute("value", aLabel);
    valueNode.setAttribute("value", aValue);
    spacer.setAttribute("flex", "1");

    aElementNode.appendChild(labelNode);
    aElementNode.appendChild(spacer);
    aElementNode.appendChild(valueNode);

    aElementNode.labelNode = labelNode;
    aElementNode.valueNode = valueNode;
  },

  


  _appendEmptyNotice: function DVSL__appendEmptyNotice() {
    if (this._emptyTextNode) {
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
  _selectedIndex: -1,
  _selectedItem: null,
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
