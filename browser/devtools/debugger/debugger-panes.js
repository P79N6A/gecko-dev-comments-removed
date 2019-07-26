




"use strict";




function SourcesView() {
  dumpn("SourcesView was instantiated");

  this._onEditorLoad = this._onEditorLoad.bind(this);
  this._onEditorUnload = this._onEditorUnload.bind(this);
  this._onEditorSelection = this._onEditorSelection.bind(this);
  this._onEditorContextMenu = this._onEditorContextMenu.bind(this);
  this._onSourceSelect = this._onSourceSelect.bind(this);
  this._onSourceClick = this._onSourceClick.bind(this);
  this._onBreakpointRemoved = this._onBreakpointRemoved.bind(this);
  this._onBreakpointClick = this._onBreakpointClick.bind(this);
  this._onBreakpointCheckboxClick = this._onBreakpointCheckboxClick.bind(this);
  this._onConditionalPopupShowing = this._onConditionalPopupShowing.bind(this);
  this._onConditionalPopupShown = this._onConditionalPopupShown.bind(this);
  this._onConditionalPopupHiding = this._onConditionalPopupHiding.bind(this);
  this._onConditionalTextboxInput = this._onConditionalTextboxInput.bind(this);
  this._onConditionalTextboxKeyPress = this._onConditionalTextboxKeyPress.bind(this);
}

SourcesView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the SourcesView");

    this.widget = new SideMenuWidget(document.getElementById("sources"));
    this.emptyText = L10N.getStr("noSourcesText");
    this.unavailableText = L10N.getStr("noMatchingSourcesText");

    this._commandset = document.getElementById("debuggerCommands");
    this._popupset = document.getElementById("debuggerPopupset");
    this._cmPopup = document.getElementById("sourceEditorContextMenu");
    this._cbPanel = document.getElementById("conditional-breakpoint-panel");
    this._cbTextbox = document.getElementById("conditional-breakpoint-panel-textbox");

    window.addEventListener("Debugger:EditorLoaded", this._onEditorLoad, false);
    window.addEventListener("Debugger:EditorUnloaded", this._onEditorUnload, false);
    this.widget.addEventListener("select", this._onSourceSelect, false);
    this.widget.addEventListener("click", this._onSourceClick, false);
    this._cbPanel.addEventListener("popupshowing", this._onConditionalPopupShowing, false);
    this._cbPanel.addEventListener("popupshown", this._onConditionalPopupShown, false);
    this._cbPanel.addEventListener("popuphiding", this._onConditionalPopupHiding, false);
    this._cbTextbox.addEventListener("input", this._onConditionalTextboxInput, false);
    this._cbTextbox.addEventListener("keypress", this._onConditionalTextboxKeyPress, false);

    this.autoFocusOnSelection = false;

    
    this.empty();
  },

  


  destroy: function() {
    dumpn("Destroying the SourcesView");

    window.removeEventListener("Debugger:EditorLoaded", this._onEditorLoad, false);
    window.removeEventListener("Debugger:EditorUnloaded", this._onEditorUnload, false);
    this.widget.removeEventListener("select", this._onSourceSelect, false);
    this.widget.removeEventListener("click", this._onSourceClick, false);
    this._cbPanel.removeEventListener("popupshowing", this._onConditionalPopupShowing, false);
    this._cbPanel.removeEventListener("popupshowing", this._onConditionalPopupShown, false);
    this._cbPanel.removeEventListener("popuphiding", this._onConditionalPopupHiding, false);
    this._cbTextbox.removeEventListener("input", this._onConditionalTextboxInput, false);
    this._cbTextbox.removeEventListener("keypress", this._onConditionalTextboxKeyPress, false);
  },

  



  set preferredSource(aSourceLocation) {
    this._preferredValue = aSourceLocation;

    
    
    if (this.containsValue(aSourceLocation)) {
      this.selectedValue = aSourceLocation;
    }
  },

  








  addSource: function(aSource, aOptions = {}) {
    let url = aSource.url;
    let label = SourceUtils.getSourceLabel(url.split(" -> ").pop());
    let group = SourceUtils.getSourceGroup(url.split(" -> ").pop());

    
    this.push([label, url, group], {
      staged: aOptions.staged, 
      attachment: {
        source: aSource
      }
    });
  },

  















  addBreakpoint: function(aOptions) {
    let { sourceLocation: url, lineNumber: line } = aOptions;

    
    
    if (this.getBreakpoint(url, line)) {
      this.enableBreakpoint(url, line, { id: aOptions.actor });
      return;
    }

    
    let sourceItem = this.getItemByValue(url);

    
    let breakpointView = this._createBreakpointView.call(this, aOptions);
    let contextMenu = this._createContextMenu.call(this, aOptions);

    
    let breakpointItem = sourceItem.append(breakpointView.container, {
      attachment: Heritage.extend(aOptions, {
        view: breakpointView,
        popup: contextMenu
      }),
      attributes: [
        ["contextmenu", contextMenu.menupopupId]
      ],
      
      
      finalize: this._onBreakpointRemoved
    });

    
    
    if (aOptions.openPopupFlag) {
      this.highlightBreakpoint(url, line, { openPopup: true });
    }
  },

  







  removeBreakpoint: function(aSourceLocation, aLineNumber) {
    
    
    let sourceItem = this.getItemByValue(aSourceLocation);
    if (!sourceItem) {
      return;
    }
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (!breakpointItem) {
      return;
    }

    sourceItem.remove(breakpointItem);
  },

  









  getBreakpoint: function(aSourceLocation, aLineNumber) {
    return this.getItemForPredicate((aItem) =>
      aItem.attachment.sourceLocation == aSourceLocation &&
      aItem.attachment.lineNumber == aLineNumber);
  },

  
















  enableBreakpoint: function(aSourceLocation, aLineNumber, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (!breakpointItem) {
      return false;
    }

    
    if (aOptions.id) {
      breakpointItem.attachment.view.container.id = "breakpoint-" + aOptions.id;
    }
    
    if (!aOptions.silent) {
      breakpointItem.attachment.view.checkbox.setAttribute("checked", "true");
    }

    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
    let breakpointLocation = { url: url, line: line };

    
    if (!DebuggerController.Breakpoints.getBreakpoint(url, line)) {
      DebuggerController.Breakpoints.addBreakpoint(breakpointLocation, aOptions.callback, {
        noPaneUpdate: true,
        noPaneHighlight: true,
        conditionalExpression: breakpointItem.attachment.conditionalExpression
      });
    }

    
    breakpointItem.attachment.disabled = false;
    return true;
  },

  















  disableBreakpoint: function(aSourceLocation, aLineNumber, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (!breakpointItem) {
      return false;
    }

    
    if (!aOptions.silent) {
      breakpointItem.attachment.view.checkbox.removeAttribute("checked");
    }

    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
    let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);

    
    if (breakpointClient) {
      DebuggerController.Breakpoints.removeBreakpoint(breakpointClient, aOptions.callback, {
        noPaneUpdate: true
      });
      
      
      breakpointItem.attachment.conditionalExpression = breakpointClient.conditionalExpression;
    }

    
    breakpointItem.attachment.disabled = true;
    return true;
  },

  











  highlightBreakpoint: function(aSourceLocation, aLineNumber, aFlags = {}) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (!breakpointItem) {
      return;
    }

    
    this._selectBreakpoint(breakpointItem);

    
    if (aFlags.updateEditor) {
      DebuggerView.updateEditor(aSourceLocation, aLineNumber, { noDebug: true });
    }

    
    
    if (aFlags.openPopup) {
      this._openConditionalPopup();
    } else {
      this._hideConditionalPopup();
    }
  },

  


  unhighlightBreakpoint: function() {
    this._unselectBreakpoint();
    this._hideConditionalPopup();
  },

  



  get selectedBreakpointItem() this._selectedBreakpoint,

  



  get selectedBreakpointClient() {
    let breakpointItem = this._selectedBreakpoint;
    if (breakpointItem) {
      let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
      return DebuggerController.Breakpoints.getBreakpoint(url, line);
    }
    return null;
  },

  





  _selectBreakpoint: function(aItem) {
    if (this._selectedBreakpoint == aItem) {
      return;
    }
    this._unselectBreakpoint();
    this._selectedBreakpoint = aItem;
    this._selectedBreakpoint.target.classList.add("selected");

    
    this.widget.ensureElementIsVisible(aItem.target);
  },

  


  _unselectBreakpoint: function() {
    if (this._selectedBreakpoint) {
      this._selectedBreakpoint.target.classList.remove("selected");
      this._selectedBreakpoint = null;
    }
  },

  


  _openConditionalPopup: function() {
    let selectedBreakpointItem = this.selectedBreakpointItem;
    let selectedBreakpointClient = this.selectedBreakpointClient;

    if (selectedBreakpointClient.conditionalExpression === undefined) {
      this._cbTextbox.value = selectedBreakpointClient.conditionalExpression = "";
    } else {
      this._cbTextbox.value = selectedBreakpointClient.conditionalExpression;
    }

    this._cbPanel.hidden = false;
    this._cbPanel.openPopup(selectedBreakpointItem.attachment.view.lineNumber,
      BREAKPOINT_CONDITIONAL_POPUP_POSITION,
      BREAKPOINT_CONDITIONAL_POPUP_OFFSET_X,
      BREAKPOINT_CONDITIONAL_POPUP_OFFSET_Y);
  },

  


  _hideConditionalPopup: function() {
    this._cbPanel.hidden = true;
    this._cbPanel.hidePopup();
  },

  












  _createBreakpointView: function(aOptions) {
    let { lineNumber, lineText } = aOptions;

    let checkbox = document.createElement("checkbox");
    checkbox.setAttribute("checked", "true");

    let lineNumberNode = document.createElement("label");
    lineNumberNode.className = "plain dbg-breakpoint-line";
    lineNumberNode.setAttribute("value", lineNumber);

    let lineTextNode = document.createElement("label");
    lineTextNode.className = "plain dbg-breakpoint-text";
    lineTextNode.setAttribute("value", lineText);
    lineTextNode.setAttribute("crop", "end");
    lineTextNode.setAttribute("flex", "1");
    lineTextNode.setAttribute("tooltiptext",
      lineText.substr(0, BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH));

    let container = document.createElement("hbox");
    container.id = "breakpoint-" + aOptions.actor;
    container.className = "dbg-breakpoint side-menu-widget-item-other";
    container.setAttribute("align", "center");
    container.setAttribute("flex", "1");

    container.addEventListener("click", this._onBreakpointClick, false);
    checkbox.addEventListener("click", this._onBreakpointCheckboxClick, false);

    container.appendChild(checkbox);
    container.appendChild(lineNumberNode);
    container.appendChild(lineTextNode);

    return {
      container: container,
      checkbox: checkbox,
      lineNumber: lineNumberNode,
      lineText: lineTextNode
    };
  },

  









  _createContextMenu: function(aOptions) {
    let commandsetId = "bp-cSet-" + aOptions.actor;
    let menupopupId = "bp-mPop-" + aOptions.actor;

    let commandset = document.createElement("commandset");
    let menupopup = document.createElement("menupopup");
    commandset.id = commandsetId;
    menupopup.id = menupopupId;

    createMenuItem.call(this, "enableSelf", true);
    createMenuItem.call(this, "disableSelf");
    createMenuItem.call(this, "deleteSelf");
    createMenuSeparator();
    createMenuItem.call(this, "setConditional");
    createMenuSeparator();
    createMenuItem.call(this, "enableOthers");
    createMenuItem.call(this, "disableOthers");
    createMenuItem.call(this, "deleteOthers");
    createMenuSeparator();
    createMenuItem.call(this, "enableAll");
    createMenuItem.call(this, "disableAll");
    createMenuSeparator();
    createMenuItem.call(this, "deleteAll");

    this._popupset.appendChild(menupopup);
    this._commandset.appendChild(commandset);

    return {
      commandsetId: commandsetId,
      menupopupId: menupopupId
    };

    








    function createMenuItem(aName, aHiddenFlag) {
      let menuitem = document.createElement("menuitem");
      let command = document.createElement("command");

      let prefix = "bp-cMenu-"; 
      let commandId = prefix + aName + "-" + aOptions.actor + "-command";
      let menuitemId = prefix + aName + "-" + aOptions.actor + "-menuitem";

      let label = L10N.getStr("breakpointMenuItem." + aName);
      let func = "_on" + aName.charAt(0).toUpperCase() + aName.slice(1);

      command.id = commandId;
      command.setAttribute("label", label);
      command.addEventListener("command", () => this[func](aOptions.actor), false);

      menuitem.id = menuitemId;
      menuitem.setAttribute("command", commandId);
      aHiddenFlag && menuitem.setAttribute("hidden", "true");

      commandset.appendChild(command);
      menupopup.appendChild(menuitem);
    }

    



    function createMenuSeparator() {
      let menuseparator = document.createElement("menuseparator");
      menupopup.appendChild(menuseparator);
    }
  },

  





  _onBreakpointRemoved: function(aItem) {
    dumpn("Finalizing breakpoint item: " + aItem);

    
    let contextMenu = aItem.attachment.popup;
    document.getElementById(contextMenu.commandsetId).remove();
    document.getElementById(contextMenu.menupopupId).remove();

    if (this._selectedBreakpoint == aItem) {
      this._selectedBreakpoint = null;
    }
  },

  


  _onEditorLoad: function({ detail: editor }) {
    editor.addEventListener("Selection", this._onEditorSelection, false);
    editor.addEventListener("ContextMenu", this._onEditorContextMenu, false);
  },

  


  _onEditorUnload: function({ detail: editor }) {
    editor.removeEventListener("Selection", this._onEditorSelection, false);
    editor.removeEventListener("ContextMenu", this._onEditorContextMenu, false);
  },

  


  _onEditorSelection: function(e) {
    let { start, end } = e.newValue;

    let sourceLocation = this.selectedValue;
    let lineStart = DebuggerView.editor.getLineAtOffset(start) + 1;
    let lineEnd = DebuggerView.editor.getLineAtOffset(end) + 1;

    if (this.getBreakpoint(sourceLocation, lineStart) && lineStart == lineEnd) {
      this.highlightBreakpoint(sourceLocation, lineStart);
    } else {
      this.unhighlightBreakpoint();
    }
  },

  


  _onEditorContextMenu: function({ x, y }) {
    let offset = DebuggerView.editor.getOffsetAtLocation(x, y);
    let line = DebuggerView.editor.getLineAtOffset(offset);
    this._editorContextMenuLineNumber = line;
  },

  


  _onSourceSelect: function({ detail: sourceItem }) {
    if (!sourceItem) {
      return;
    }
    
    let selectedSource = sourceItem.attachment.source;

    if (DebuggerView.editorSource != selectedSource) {
      DebuggerView.editorSource = selectedSource;
    }
  },

  


  _onSourceClick: function() {
    
    DebuggerView.Filtering.target = this;
  },

  


  _onBreakpointClick: function(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;

    let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);
    let conditionalExpression = (breakpointClient || {}).conditionalExpression;

    this.highlightBreakpoint(url, line, {
      updateEditor: true,
      openPopup: conditionalExpression !== undefined && e.button == 0
    });
  },

  


  _onBreakpointCheckboxClick: function(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let { sourceLocation: url, lineNumber: line, disabled } = breakpointItem.attachment;

    this[disabled ? "enableBreakpoint" : "disableBreakpoint"](url, line, {
      silent: true
    });

    
    e.preventDefault();
    e.stopPropagation();
  },

  


  _onConditionalPopupShowing: function() {
    this._conditionalPopupVisible = true;
  },

  


  _onConditionalPopupShown: function() {
    this._cbTextbox.focus();
    this._cbTextbox.select();
  },

  


  _onConditionalPopupHiding: function() {
    this._conditionalPopupVisible = false;
  },

  


  _onConditionalTextboxInput: function() {
    this.selectedBreakpointClient.conditionalExpression = this._cbTextbox.value;
  },

  


  _onConditionalTextboxKeyPress: function(e) {
    if (e.keyCode == e.DOM_VK_RETURN || e.keyCode == e.DOM_VK_ENTER) {
      this._hideConditionalPopup();
    }
  },

  


  _onCmdAddBreakpoint: function() {
    
    
    if (this._editorContextMenuLineNumber >= 0) {
      DebuggerView.editor.setCaretPosition(this._editorContextMenuLineNumber);
    }
    
    this._editorContextMenuLineNumber = -1;

    let url = DebuggerView.Sources.selectedValue;
    let line = DebuggerView.editor.getCaretPosition().line + 1;
    let breakpointItem = this.getBreakpoint(url, line);

    
    if (breakpointItem) {
      let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);
      DebuggerController.Breakpoints.removeBreakpoint(breakpointClient);
    }
    
    else {
      let breakpointLocation = { url: url, line: line };
      DebuggerController.Breakpoints.addBreakpoint(breakpointLocation);
    }
  },

  


  _onCmdAddConditionalBreakpoint: function() {
    
    
    if (this._editorContextMenuLineNumber >= 0) {
      DebuggerView.editor.setCaretPosition(this._editorContextMenuLineNumber);
    }
    
    this._editorContextMenuLineNumber = -1;

    let url =  DebuggerView.Sources.selectedValue;
    let line = DebuggerView.editor.getCaretPosition().line + 1;
    let breakpointItem = this.getBreakpoint(url, line);

    
    if (breakpointItem) {
      let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);
      this.highlightBreakpoint(url, line, { openPopup: true });
    }
    
    else {
      DebuggerController.Breakpoints.addBreakpoint({ url: url, line: line }, null, {
        conditionalExpression: "",
        openPopup: true
      });
    }
  },

  









  _onSetConditional: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let { sourceLocation: url, lineNumber: line } = targetBreakpoint.attachment;

    
    this.highlightBreakpoint(url, line, { openPopup: true });

    
    aCallback();
  },

  









  _onEnableSelf: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let { sourceLocation: url, lineNumber: line, actor } = targetBreakpoint.attachment;

    
    if (this.enableBreakpoint(url, line)) {
      let prefix = "bp-cMenu-"; 
      let enableSelfId = prefix + "enableSelf-" + actor + "-menuitem";
      let disableSelfId = prefix + "disableSelf-" + actor + "-menuitem";
      document.getElementById(enableSelfId).setAttribute("hidden", "true");
      document.getElementById(disableSelfId).removeAttribute("hidden");

      
      
      
      if (gThreadClient.state != "paused") {
        gThreadClient.addOneTimeListener("resumed", aCallback);
      } else {
        aCallback();
      }
    }
  },

  









  _onDisableSelf: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let { sourceLocation: url, lineNumber: line, actor } = targetBreakpoint.attachment;

    
    if (this.disableBreakpoint(url, line)) {
      let prefix = "bp-cMenu-"; 
      let enableSelfId = prefix + "enableSelf-" + actor + "-menuitem";
      let disableSelfId = prefix + "disableSelf-" + actor + "-menuitem";
      document.getElementById(enableSelfId).removeAttribute("hidden");
      document.getElementById(disableSelfId).setAttribute("hidden", "true");

      
      aCallback();
    }
  },

  









  _onDeleteSelf: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let { sourceLocation: url, lineNumber: line } = targetBreakpoint.attachment;

    
    this.removeBreakpoint(url, line);
    gBreakpoints.removeBreakpoint(gBreakpoints.getBreakpoint(url, line), aCallback);
  },

  









  _onEnableOthers: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);

    
    
    for (let source in this) {
      for (let otherBreakpoint in source) {
        if (otherBreakpoint != targetBreakpoint &&
            otherBreakpoint.attachment.disabled) {
          this._onEnableSelf(otherBreakpoint.attachment.actor, () =>
            this._onEnableOthers(aId, aCallback));
          return;
        }
      }
    }
    
    aCallback();
  },

  









  _onDisableOthers: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);

    
    
    for (let source in this) {
      for (let otherBreakpoint in source) {
        if (otherBreakpoint != targetBreakpoint &&
           !otherBreakpoint.attachment.disabled) {
          this._onDisableSelf(otherBreakpoint.attachment.actor, () =>
            this._onDisableOthers(aId, aCallback));
          return;
        }
      }
    }
    
    aCallback();
  },

  









  _onDeleteOthers: function(aId, aCallback = () => {}) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);

    
    
    for (let source in this) {
      for (let otherBreakpoint in source) {
        if (otherBreakpoint != targetBreakpoint) {
          this._onDeleteSelf(otherBreakpoint.attachment.actor, () =>
            this._onDeleteOthers(aId, aCallback));
          return;
        }
      }
    }
    
    aCallback();
  },

  









  _onEnableAll: function(aId) {
    this._onEnableOthers(aId, () => this._onEnableSelf(aId));
  },

  









  _onDisableAll: function(aId) {
    this._onDisableOthers(aId, () => this._onDisableSelf(aId));
  },

  









  _onDeleteAll: function(aId) {
    this._onDeleteOthers(aId, () => this._onDeleteSelf(aId));
  },

  _commandset: null,
  _popupset: null,
  _cmPopup: null,
  _cbPanel: null,
  _cbTextbox: null,
  _selectedBreakpoint: null,
  _editorContextMenuLineNumber: -1,
  _conditionalPopupVisible: false
});




let SourceUtils = {
  _labelsCache: new Map(), 
  _groupsCache: new Map(),

  




  clearCache: function() {
    this._labelsCache.clear();
    this._groupsCache.clear();
  },

  







  getSourceLabel: function(aUrl) {
    let cachedLabel = this._labelsCache.get(aUrl);
    if (cachedLabel) {
      return cachedLabel;
    }

    let sourceLabel = this.trimUrl(aUrl);
    let unicodeLabel = NetworkHelper.convertToUnicode(unescape(sourceLabel));
    this._labelsCache.set(aUrl, unicodeLabel);
    return unicodeLabel;
  },

  








  getSourceGroup: function(aUrl) {
    let cachedGroup = this._groupsCache.get(aUrl);
    if (cachedGroup) {
      return cachedGroup;
    }

    try {
      
      var uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    } catch (e) {
      
      return "";
    }

    let { scheme, directory, fileName } = uri;
    let hostPort;
    
    if (scheme != "jar") {
      hostPort = uri.hostPort;
    }
    let lastDir = directory.split("/").reverse()[1];
    let group = [];

    
    if (scheme != "http") {
      group.push(scheme);
    }
    
    
    if (hostPort) {
      
      group.push(hostPort.split(".").slice(0, 2).join("."));
    }
    
    
    if (fileName) {
      group.push(lastDir);
    }

    let groupLabel = group.join(" ");
    let unicodeLabel = NetworkHelper.convertToUnicode(unescape(groupLabel));
    this._groupsCache.set(aUrl, unicodeLabel)
    return unicodeLabel;
  },

  












  trimUrlLength: function(aUrl, aLength, aSection) {
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

  







  trimUrlQuery: function(aUrl) {
    let length = aUrl.length;
    let q1 = aUrl.indexOf('?');
    let q2 = aUrl.indexOf('&');
    let q3 = aUrl.indexOf('#');
    let q = Math.min(q1 != -1 ? q1 : length,
                     q2 != -1 ? q2 : length,
                     q3 != -1 ? q3 : length);

    return aUrl.slice(0, q);
  },

  












  trimUrl: function(aUrl, aLabel, aSeq) {
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




function WatchExpressionsView() {
  dumpn("WatchExpressionsView was instantiated");

  this.switchExpression = this.switchExpression.bind(this);
  this.deleteExpression = this.deleteExpression.bind(this);
  this._createItemView = this._createItemView.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onClose = this._onClose.bind(this);
  this._onBlur = this._onBlur.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
}

WatchExpressionsView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the WatchExpressionsView");

    this.widget = new ListWidget(document.getElementById("expressions"));
    this.widget.permaText = L10N.getStr("addWatchExpressionText");
    this.widget.itemFactory = this._createItemView;
    this.widget.setAttribute("context", "debuggerWatchExpressionsContextMenu");
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the WatchExpressionsView");

    this.widget.removeEventListener("click", this._onClick, false);
  },

  





  addExpression: function(aExpression = "") {
    
    DebuggerView.showInstrumentsPane();

    
    let expressionItem = this.push([, aExpression], {
      index: 0, 
      relaxed: true, 
      attachment: {
        initialExpression: aExpression,
        currentExpression: ""
      }
    });

    
    expressionItem.attachment.inputNode.select();
    expressionItem.attachment.inputNode.focus();
    DebuggerView.Variables.parentNode.scrollTop = 0;
  },

  









  switchExpression: function(aVar, aExpression) {
    let expressionItem =
      [i for (i in this) if (i.attachment.currentExpression == aVar.name)][0];

    
    if (!aExpression || this.getAllStrings().indexOf(aExpression) != -1) {
      this.deleteExpression(aVar);
      return;
    }

    
    expressionItem.attachment.currentExpression = aExpression;
    expressionItem.attachment.inputNode.value = aExpression;

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  







  deleteExpression: function(aVar) {
    let expressionItem =
      [i for (i in this) if (i.attachment.currentExpression == aVar.name)][0];

    
    this.remove(expressionItem);

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  







  getString: function(aIndex) {
    return this.getItemAtIndex(aIndex).attachment.currentExpression;
  },

  





  getAllStrings: function() {
    return this.orderedItems.map((e) => e.attachment.currentExpression);
  },

  







  _createItemView: function(aElementNode, aAttachment) {
    let arrowNode = document.createElement("box");
    arrowNode.className = "dbg-expression-arrow";

    let inputNode = document.createElement("textbox");
    inputNode.className = "plain dbg-expression-input";
    inputNode.setAttribute("value", aAttachment.initialExpression);
    inputNode.setAttribute("flex", "1");

    let closeNode = document.createElement("toolbarbutton");
    closeNode.className = "plain variables-view-delete";

    closeNode.addEventListener("click", this._onClose, false);
    inputNode.addEventListener("blur", this._onBlur, false);
    inputNode.addEventListener("keypress", this._onKeyPress, false);

    aElementNode.className = "dbg-expression";
    aElementNode.appendChild(arrowNode);
    aElementNode.appendChild(inputNode);
    aElementNode.appendChild(closeNode);

    aAttachment.arrowNode = arrowNode;
    aAttachment.inputNode = inputNode;
    aAttachment.closeNode = closeNode;
  },

  


  _onCmdAddExpression: function(aText) {
    
    if (this.getAllStrings().indexOf("") == -1) {
      this.addExpression(aText || DebuggerView.editor.getSelectedText());
    }
  },

  


  _onCmdRemoveAllExpressions: function() {
    
    this.empty();

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  


  _onClick: function(e) {
    if (e.button != 0) {
      
      return;
    }
    let expressionItem = this.getItemForElement(e.target);
    if (!expressionItem) {
      
      this.addExpression();
    }
  },

  


  _onClose: function(e) {
    
    this.remove(this.getItemForElement(e.target));

    
    DebuggerController.StackFrames.syncWatchExpressions();

    
    e.preventDefault();
    e.stopPropagation();
  },

  


  _onBlur: function({ target: textbox }) {
    let expressionItem = this.getItemForElement(textbox);
    let oldExpression = expressionItem.attachment.currentExpression;
    let newExpression = textbox.value.trim();

    
    if (!newExpression) {
      this.remove(expressionItem);
    }
    
    else if (!oldExpression && this.getAllStrings().indexOf(newExpression) != -1) {
      this.remove(expressionItem);
    }
    
    else {
      expressionItem.attachment.currentExpression = newExpression;
    }

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  


  _onKeyPress: function(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
      case e.DOM_VK_ESCAPE:
        DebuggerView.editor.focus();
        return;
    }
  }
});




function GlobalSearchView() {
  dumpn("GlobalSearchView was instantiated");

  this._startSearch = this._startSearch.bind(this);
  this._performGlobalSearch = this._performGlobalSearch.bind(this);
  this._createItemView = this._createItemView.bind(this);
  this._onScroll = this._onScroll.bind(this);
  this._onHeaderClick = this._onHeaderClick.bind(this);
  this._onLineClick = this._onLineClick.bind(this);
  this._onMatchClick = this._onMatchClick.bind(this);
}

GlobalSearchView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the GlobalSearchView");

    this.widget = new ListWidget(document.getElementById("globalsearch"));
    this._splitter = document.querySelector("#globalsearch + .devtools-horizontal-splitter");

    this.widget.emptyText = L10N.getStr("noMatchingStringsText");
    this.widget.itemFactory = this._createItemView;
    this.widget.addEventListener("scroll", this._onScroll, false);
  },

  


  destroy: function() {
    dumpn("Destroying the GlobalSearchView");

    this.widget.removeEventListener("scroll", this._onScroll, false);
  },

  



  get hidden()
    this.widget.getAttribute("hidden") == "true" ||
    this._splitter.getAttribute("hidden") == "true",

  



  set hidden(aFlag) {
    this.widget.setAttribute("hidden", aFlag);
    this._splitter.setAttribute("hidden", aFlag);
  },

  


  clearView: function() {
    this.hidden = true;
    this.empty();
    window.dispatchEvent(document, "Debugger:GlobalSearch:ViewCleared");
  },

  



  selectNext: function() {
    let totalLineResults = LineResults.size();
    if (!totalLineResults) {
      return;
    }
    if (++this._currentlyFocusedMatch >= totalLineResults) {
      this._currentlyFocusedMatch = 0;
    }
    this._onMatchClick({
      target: LineResults.getElementAtIndex(this._currentlyFocusedMatch)
    });
  },

  



  selectPrev: function() {
    let totalLineResults = LineResults.size();
    if (!totalLineResults) {
      return;
    }
    if (--this._currentlyFocusedMatch < 0) {
      this._currentlyFocusedMatch = totalLineResults - 1;
    }
    this._onMatchClick({
      target: LineResults.getElementAtIndex(this._currentlyFocusedMatch)
    });
  },

  


  delayedSearch: true,

  





  scheduleSearch: function(aQuery) {
    if (!this.delayedSearch) {
      this.performSearch(aQuery);
      return;
    }
    let delay = Math.max(GLOBAL_SEARCH_ACTION_MAX_DELAY / aQuery.length, 0);

    window.clearTimeout(this._searchTimeout);
    this._searchFunction = this._startSearch.bind(this, aQuery);
    this._searchTimeout = window.setTimeout(this._searchFunction, delay);
  },

  





  performSearch: function(aQuery) {
    window.clearTimeout(this._searchTimeout);
    this._searchFunction = null;
    this._startSearch(aQuery);
  },

  





  _startSearch: function(aQuery) {
    this._searchedToken = aQuery;

    
    DebuggerController.SourceScripts
      .getTextForSources(DebuggerView.Sources.values)
      .then(this._performGlobalSearch);
  },

  



  _performGlobalSearch: function(aSources) {
    
    let token = this._searchedToken;

    
    if (!token) {
      this.clearView();
      window.dispatchEvent(document, "Debugger:GlobalSearch:TokenEmpty");
      return;
    }

    
    let lowerCaseToken = token.toLowerCase();
    let tokenLength = token.length;

    
    let globalResults = new GlobalResults();

    for (let [location, contents] of aSources) {
      
      if (!contents.toLowerCase().contains(lowerCaseToken)) {
        continue;
      }
      let lines = contents.split("\n");
      let sourceResults = new SourceResults();

      for (let i = 0, len = lines.length; i < len; i++) {
        let line = lines[i];
        let lowerCaseLine = line.toLowerCase();

        
        if (!lowerCaseLine.contains(lowerCaseToken)) {
          continue;
        }

        let lineNumber = i;
        let lineResults = new LineResults();

        lowerCaseLine.split(lowerCaseToken).reduce((prev, curr, index, { length }) => {
          let prevLength = prev.length;
          let currLength = curr.length;
          let unmatched = line.substr(prevLength, currLength);
          lineResults.add(unmatched);

          if (index != length - 1) {
            let matched = line.substr(prevLength + currLength, tokenLength);
            let range = {
              start: prevLength + currLength,
              length: matched.length
            };
            lineResults.add(matched, range, true);
            sourceResults.matchCount++;
          }
          return prev + token + curr;
        }, "");

        if (sourceResults.matchCount) {
          sourceResults.add(lineNumber, lineResults);
        }
      }
      if (sourceResults.matchCount) {
        globalResults.add(location, sourceResults);
      }
    }

    
    this.empty();

    
    if (globalResults.itemCount) {
      this.hidden = false;
      this._currentlyFocusedMatch = -1;
      this._createGlobalResultsUI(globalResults);
      window.dispatchEvent(document, "Debugger:GlobalSearch:MatchFound");
    } else {
      window.dispatchEvent(document, "Debugger:GlobalSearch:MatchNotFound");
    }
  },

  





  _createGlobalResultsUI: function(aGlobalResults) {
    let i = 0;

    for (let [location, sourceResults] in aGlobalResults) {
      if (i++ == 0) {
        this._createSourceResultsUI(location, sourceResults, true);
      } else {
        
        
        
        Services.tm.currentThread.dispatch({ run:
          this._createSourceResultsUI.bind(this, location, sourceResults) }, 0);
      }
    }
  },

  









  _createSourceResultsUI: function(aLocation, aSourceResults, aExpandFlag) {
    
    let sourceResultsItem = this.push([aLocation, aSourceResults.matchCount], {
      index: -1, 
      relaxed: true, 
      attachment: {
        sourceResults: aSourceResults,
        expandFlag: aExpandFlag
      }
    });
  },

  











  _createItemView: function(aElementNode, aAttachment, aLocation, aMatchCount) {
    let { sourceResults, expandFlag } = aAttachment;

    sourceResults.createView(aElementNode, aLocation, aMatchCount, expandFlag, {
      onHeaderClick: this._onHeaderClick,
      onLineClick: this._onLineClick,
      onMatchClick: this._onMatchClick
    });
  },

  


  _onHeaderClick: function(e) {
    let sourceResultsItem = SourceResults.getItemForElement(e.target);
    sourceResultsItem.instance.toggle(e);
  },

  


  _onLineClick: function(e) {
    let lineResultsItem = LineResults.getItemForElement(e.target);
    this._onMatchClick({ target: lineResultsItem.firstMatch });
  },

  


  _onMatchClick: function(e) {
    if (e instanceof Event) {
      e.preventDefault();
      e.stopPropagation();
    }
    let target = e.target;
    let sourceResultsItem = SourceResults.getItemForElement(target);
    let lineResultsItem = LineResults.getItemForElement(target);

    sourceResultsItem.instance.expand();
    this._currentlyFocusedMatch = LineResults.indexOfElement(target);
    this._scrollMatchIntoViewIfNeeded(target);
    this._bounceMatch(target);

    let location = sourceResultsItem.location;
    let lineNumber = lineResultsItem.lineNumber;
    DebuggerView.updateEditor(location, lineNumber + 1, { noDebug: true });

    let editor = DebuggerView.editor;
    let offset = editor.getCaretOffset();
    let { start, length } = lineResultsItem.lineData.range;
    editor.setSelection(offset + start, offset + start + length);
  },

  


  _onScroll: function(e) {
    for (let item in this) {
      this._expandResultsIfNeeded(item.target);
    }
  },

  





  _expandResultsIfNeeded: function(aTarget) {
    let sourceResultsItem = SourceResults.getItemForElement(aTarget);
    if (sourceResultsItem.instance.toggled ||
        sourceResultsItem.instance.expanded) {
      return;
    }
    let { top, height } = aTarget.getBoundingClientRect();
    let { clientHeight } = this.widget._parent;

    if (top - height <= clientHeight || this._forceExpandResults) {
      sourceResultsItem.instance.expand();
    }
  },

  





  _scrollMatchIntoViewIfNeeded: function(aMatch) {
    
    
    let boxObject = this.widget._parent.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    boxObject.ensureElementIsVisible(aMatch);
  },

  





  _bounceMatch: function(aMatch) {
    Services.tm.currentThread.dispatch({ run: function() {
      aMatch.addEventListener("transitionend", function onEvent() {
        aMatch.removeEventListener("transitionend", onEvent);
        aMatch.removeAttribute("focused");
      });
      aMatch.setAttribute("focused", "");
    }}, 0);
    aMatch.setAttribute("focusing", "");
  },

  _splitter: null,
  _currentlyFocusedMatch: -1,
  _forceExpandResults: false,
  _searchTimeout: null,
  _searchFunction: null,
  _searchedToken: ""
});





function GlobalResults() {
  this._store = new Map();
  SourceResults._itemsByElement = new Map();
  LineResults._itemsByElement = new Map();
}

GlobalResults.prototype = {
  







  add: function(aLocation, aSourceResults) {
    this._store.set(aLocation, aSourceResults);
  },

  


  get itemCount() this._store.size,

  _store: null
};





function SourceResults() {
  this._store = new Map();
  this.matchCount = 0;
}

SourceResults.prototype = {
  







  add: function(aLineNumber, aLineResults) {
    this._store.set(aLineNumber, aLineResults);
  },

  


  matchCount: -1,

  


  expand: function() {
    this._target.resultsContainer.removeAttribute("hidden")
    this._target.arrow.setAttribute("open", "");
  },

  


  collapse: function() {
    this._target.resultsContainer.setAttribute("hidden", "true");
    this._target.arrow.removeAttribute("open");
  },

  


  toggle: function(e) {
    if (e instanceof Event) {
      this._userToggled = true;
    }
    this.expanded ^= 1;
  },

  


  alwaysExpand: true,

  



  get expanded()
    this._target.resultsContainer.getAttribute("hidden") != "true" &&
    this._target.arrow.hasAttribute("open"),

  



  set expanded(aFlag) this[aFlag ? "expand" : "collapse"](),

  



  get toggled() this._userToggled,

  



  get target() this._target,

  















  createView: function(aElementNode, aLocation, aMatchCount, aExpandFlag, aCallbacks) {
    this._target = aElementNode;

    let arrow = document.createElement("box");
    arrow.className = "arrow";

    let locationNode = document.createElement("label");
    locationNode.className = "plain dbg-results-header-location";
    locationNode.setAttribute("value", SourceUtils.trimUrlLength(aLocation));

    let matchCountNode = document.createElement("label");
    matchCountNode.className = "plain dbg-results-header-match-count";
    matchCountNode.setAttribute("value", "(" + aMatchCount + ")");

    let resultsHeader = document.createElement("hbox");
    resultsHeader.className = "dbg-results-header";
    resultsHeader.setAttribute("align", "center")
    resultsHeader.appendChild(arrow);
    resultsHeader.appendChild(locationNode);
    resultsHeader.appendChild(matchCountNode);
    resultsHeader.addEventListener("click", aCallbacks.onHeaderClick, false);

    let resultsContainer = document.createElement("vbox");
    resultsContainer.className = "dbg-results-container";
    resultsContainer.setAttribute("hidden", "true");

    for (let [lineNumber, lineResults] of this._store) {
      lineResults.createView(resultsContainer, lineNumber, aCallbacks)
    }

    aElementNode.arrow = arrow;
    aElementNode.resultsHeader = resultsHeader;
    aElementNode.resultsContainer = resultsContainer;

    if ((aExpandFlag || this.alwaysExpand) &&
         aMatchCount < GLOBAL_SEARCH_EXPAND_MAX_RESULTS) {
      this.expand();
    }

    let resultsBox = document.createElement("vbox");
    resultsBox.setAttribute("flex", "1");
    resultsBox.appendChild(resultsHeader);
    resultsBox.appendChild(resultsContainer);

    aElementNode.id = "source-results-" + aLocation;
    aElementNode.className = "dbg-source-results";
    aElementNode.appendChild(resultsBox);

    SourceResults._itemsByElement.set(aElementNode, {
      location: aLocation,
      matchCount: aMatchCount,
      autoExpand: aExpandFlag,
      instance: this
    });
  },

  _store: null,
  _target: null,
  _userToggled: false
};





function LineResults() {
  this._store = [];
}

LineResults.prototype = {
  









  add: function(aString, aRange, aMatchFlag) {
    this._store.push({
      string: aString,
      range: aRange,
      match: !!aMatchFlag
    });
  },

  



  get target() this._target,

  











  createView: function(aContainer, aLineNumber, aCallbacks) {
    this._target = aContainer;

    let lineNumberNode = document.createElement("label");
    let lineContentsNode = document.createElement("hbox");
    let lineString = "";
    let lineLength = 0;
    let firstMatch = null;

    lineNumberNode.className = "plain dbg-results-line-number";
    lineNumberNode.setAttribute("value", aLineNumber + 1);
    lineContentsNode.className = "light list-widget-item dbg-results-line-contents";
    lineContentsNode.setAttribute("flex", "1");

    for (let chunk of this._store) {
      let { string, range, match } = chunk;
      lineString = string.substr(0, GLOBAL_SEARCH_LINE_MAX_LENGTH - lineLength);
      lineLength += string.length;

      let label = document.createElement("label");
      label.className = "plain dbg-results-line-contents-string";
      label.setAttribute("value", lineString);
      label.setAttribute("match", match);
      lineContentsNode.appendChild(label);

      if (match) {
        this._entangleMatch(aLineNumber, label, chunk);
        label.addEventListener("click", aCallbacks.onMatchClick, false);
        firstMatch = firstMatch || label;
      }
      if (lineLength >= GLOBAL_SEARCH_LINE_MAX_LENGTH) {
        lineContentsNode.appendChild(this._ellipsis.cloneNode());
        break;
      }
    }

    this._entangleLine(lineContentsNode, firstMatch);
    lineContentsNode.addEventListener("click", aCallbacks.onLineClick, false);

    let searchResult = document.createElement("hbox");
    searchResult.className = "dbg-search-result";
    searchResult.appendChild(lineNumberNode);
    searchResult.appendChild(lineContentsNode);
    aContainer.appendChild(searchResult);
  },

  





  _entangleMatch: function(aLineNumber, aNode, aMatchChunk) {
    LineResults._itemsByElement.set(aNode, {
      lineNumber: aLineNumber,
      lineData: aMatchChunk
    });
  },

  




  _entangleLine: function(aNode, aFirstMatch) {
    LineResults._itemsByElement.set(aNode, {
      firstMatch: aFirstMatch,
      nonenumerable: true
    });
  },

  


  _ellipsis: (function() {
    let label = document.createElement("label");
    label.className = "plain dbg-results-line-contents-string";
    label.setAttribute("value", L10N.ellipsis);
    return label;
  })(),

  _store: null,
  _target: null
};




GlobalResults.prototype.__iterator__ =
SourceResults.prototype.__iterator__ =
LineResults.prototype.__iterator__ = function() {
  for (let item of this._store) {
    yield item;
  }
};









SourceResults.getItemForElement =
LineResults.getItemForElement = function(aElement) {
  return WidgetMethods.getItemForElement.call(this, aElement);
};









SourceResults.getElementAtIndex =
LineResults.getElementAtIndex = function(aIndex) {
  for (let [element, item] of this._itemsByElement) {
    if (!item.nonenumerable && !aIndex--) {
      return element;
    }
  }
  return null;
};









SourceResults.indexOfElement =
LineResults.indexOfElement = function(aElement) {
  let count = 0;
  for (let [element, item] of this._itemsByElement) {
    if (element == aElement) {
      return count;
    }
    if (!item.nonenumerable) {
      count++;
    }
  }
  return -1;
};







SourceResults.size =
LineResults.size = function() {
  let count = 0;
  for (let [, item] of this._itemsByElement) {
    if (!item.nonenumerable) {
      count++;
    }
  }
  return count;
};




DebuggerView.Sources = new SourcesView();
DebuggerView.WatchExpressions = new WatchExpressionsView();
DebuggerView.GlobalSearch = new GlobalSearchView();
