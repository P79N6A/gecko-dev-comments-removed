




"use strict";




function SourcesView() {
  dumpn("SourcesView was instantiated");

  this._breakpointsCache = new Map(); 
  this._onBreakpointRemoved = this._onBreakpointRemoved.bind(this);
  this._onEditorLoad = this._onEditorLoad.bind(this);
  this._onEditorUnload = this._onEditorUnload.bind(this);
  this._onEditorSelection = this._onEditorSelection.bind(this);
  this._onEditorContextMenu = this._onEditorContextMenu.bind(this);
  this._onSourceMouseDown = this._onSourceMouseDown.bind(this);
  this._onSourceSelect = this._onSourceSelect.bind(this);
  this._onSourceClick = this._onSourceClick.bind(this);
  this._onBreakpointClick = this._onBreakpointClick.bind(this);
  this._onBreakpointCheckboxClick = this._onBreakpointCheckboxClick.bind(this);
  this._onConditionalPopupShowing = this._onConditionalPopupShowing.bind(this);
  this._onConditionalPopupShown = this._onConditionalPopupShown.bind(this);
  this._onConditionalPopupHiding = this._onConditionalPopupHiding.bind(this);
  this._onConditionalTextboxInput = this._onConditionalTextboxInput.bind(this);
  this._onConditionalTextboxKeyPress = this._onConditionalTextboxKeyPress.bind(this);
}

create({ constructor: SourcesView, proto: MenuContainer.prototype }, {
  


  initialize: function DVS_initialize() {
    dumpn("Initializing the SourcesView");

    this.node = new SideMenuWidget(document.getElementById("sources"));
    this.emptyText = L10N.getStr("noSourcesText");
    this.unavailableText = L10N.getStr("noMatchingSourcesText");

    this._commandset = document.getElementById("debuggerCommands");
    this._popupset = document.getElementById("debuggerPopupset");
    this._cmPopup = document.getElementById("sourceEditorContextMenu");
    this._cbPanel = document.getElementById("conditional-breakpoint-panel");
    this._cbTextbox = document.getElementById("conditional-breakpoint-panel-textbox");

    window.addEventListener("Debugger:EditorLoaded", this._onEditorLoad, false);
    window.addEventListener("Debugger:EditorUnloaded", this._onEditorUnload, false);
    this.node.addEventListener("mousedown", this._onSourceMouseDown, false);
    this.node.addEventListener("select", this._onSourceSelect, false);
    this.node.addEventListener("click", this._onSourceClick, false);
    this._cbPanel.addEventListener("popupshowing", this._onConditionalPopupShowing, false);
    this._cbPanel.addEventListener("popupshown", this._onConditionalPopupShown, false);
    this._cbPanel.addEventListener("popuphiding", this._onConditionalPopupHiding, false);
    this._cbTextbox.addEventListener("input", this._onConditionalTextboxInput, false);
    this._cbTextbox.addEventListener("keypress", this._onConditionalTextboxKeyPress, false);

    
    this.empty();
  },

  


  destroy: function DVS_destroy() {
    dumpn("Destroying the SourcesView");

    window.removeEventListener("Debugger:EditorLoaded", this._onEditorLoad, false);
    window.removeEventListener("Debugger:EditorUnloaded", this._onEditorUnload, false);
    this.node.removeEventListener("mousedown", this._onSourceMouseDown, false);
    this.node.removeEventListener("select", this._onSourceSelect, false);
    this.node.removeEventListener("click", this._onSourceClick, false);
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

  








  addSource: function DVS_addSource(aSource, aOptions = {}) {
    let url = aSource.url;
    let label = SourceUtils.getSourceLabel(url.split(" -> ").pop());
    let group = SourceUtils.getSourceGroup(url.split(" -> ").pop());

    
    let sourceItem = this.push([label, url, group], {
      staged: aOptions.staged, 
      attachment: {
        source: aSource
      }
    });
  },

  















  addBreakpoint: function DVS_addBreakpoint(aOptions) {
    let { sourceLocation: url, lineNumber: line } = aOptions;

    
    
    if (this.getBreakpoint(url, line)) {
      this.enableBreakpoint(url, line, { id: aOptions.actor });
      return;
    }

    
    let sourceItem = this.getItemByValue(url);

    
    let breakpointView = this._createBreakpointView.call(this, aOptions);
    let contextMenu = this._createContextMenu.call(this, aOptions);

    
    let breakpointItem = sourceItem.append(breakpointView.container, {
      attachment: Object.create(aOptions, {
        view: { value: breakpointView },
        popup: { value: contextMenu }
      }),
      attributes: [
        ["contextmenu", contextMenu.menupopupId]
      ],
      
      
      finalize: this._onBreakpointRemoved
    });

    this._breakpointsCache.set(this._getBreakpointKey(url, line), breakpointItem);

    
    
    if (aOptions.openPopupFlag) {
      this.highlightBreakpoint(url, line, { openPopup: true });
    }
  },

  







  removeBreakpoint: function DVS_removeBreakpoint(aSourceLocation, aLineNumber) {
    
    
    let sourceItem = this.getItemByValue(aSourceLocation);
    if (!sourceItem) {
      return;
    }
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (!breakpointItem) {
      return;
    }

    sourceItem.remove(breakpointItem);

    if (this._selectedBreakpoint == breakpointItem) {
      this._selectedBreakpoint = null;
    }
  },

  









  getBreakpoint: function DVS_getBreakpoint(aSourceLocation, aLineNumber) {
    let breakpointKey = this._getBreakpointKey(aSourceLocation, aLineNumber);
    return this._breakpointsCache.get(breakpointKey);
  },

  
















  enableBreakpoint:
  function DVS_enableBreakpoint(aSourceLocation, aLineNumber, aOptions = {}) {
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
    DebuggerController.Breakpoints.addBreakpoint(breakpointLocation, aOptions.callback, {
      noPaneUpdate: true,
      noPaneHighlight: true,
      conditionalExpression: breakpointItem.attachment.conditionalExpression
    });

    
    breakpointItem.attachment.disabled = false;
    return true;
  },

  















  disableBreakpoint:
  function DVS_disableBreakpoint(aSourceLocation, aLineNumber, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (!breakpointItem) {
      return false;
    }

    
    if (!aOptions.silent) {
      breakpointItem.attachment.view.checkbox.removeAttribute("checked");
    }

    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
    let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);
    DebuggerController.Breakpoints.removeBreakpoint(breakpointClient, aOptions.callback, {
      noPaneUpdate: true
    });

    
    breakpointItem.attachment.conditionalExpression = breakpointClient.conditionalExpression;

    
    breakpointItem.attachment.disabled = true;
    return true;
  },

  











  highlightBreakpoint:
  function DVS_highlightBreakpoint(aSourceLocation, aLineNumber, aFlags = {}) {
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

  


  unhighlightBreakpoint: function DVS_unhighlightBreakpoint() {
    this._unselectBreakpoint();
    this._hideConditionalPopup();
  },

  



  get selectedBreakpoint() this._selectedBreakpoint,

  



  get selectedClient() {
    let breakpointItem = this._selectedBreakpoint;
    if (breakpointItem) {
      let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
      return DebuggerController.Breakpoints.getBreakpoint(url, line);
    }
    return null;
  },

  





  _selectBreakpoint: function DVS__selectBreakpoint(aItem) {
    if (this._selectedBreakpoint == aItem) {
      return;
    }
    this._unselectBreakpoint();
    this._selectedBreakpoint = aItem;
    this._selectedBreakpoint.markSelected();

    
    this.node.ensureElementIsVisible(aItem.target);
  },

  


  _unselectBreakpoint: function DVS__unselectBreakpoint() {
    if (this._selectedBreakpoint) {
      this._selectedBreakpoint.markDeselected();
      this._selectedBreakpoint = null;
    }
  },

  


  _openConditionalPopup: function DVS__openConditionalPopup() {
    let selectedBreakpoint = this.selectedBreakpoint;
    let selectedClient = this.selectedClient;

    if (selectedClient.conditionalExpression === undefined) {
      this._cbTextbox.value = selectedClient.conditionalExpression = "";
    } else {
      this._cbTextbox.value = selectedClient.conditionalExpression;
    }

    this._cbPanel.hidden = false;
    this._cbPanel.openPopup(this.selectedBreakpoint.attachment.view.lineNumber,
      BREAKPOINT_CONDITIONAL_POPUP_POSITION,
      BREAKPOINT_CONDITIONAL_POPUP_OFFSET_X,
      BREAKPOINT_CONDITIONAL_POPUP_OFFSET_Y);
  },

  


  _hideConditionalPopup: function DVS__hideConditionalPopup() {
    this._cbPanel.hidden = true;
    this._cbPanel.hidePopup();
  },

  












  _createBreakpointView: function DVS_createBreakpointView(aOptions) {
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

  









  _createContextMenu: function DVS__createContextMenu(aOptions) {
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
      let func = this["_on" + aName.charAt(0).toUpperCase() + aName.slice(1)];

      command.id = commandId;
      command.setAttribute("label", label);
      command.addEventListener("command", func.bind(this, aOptions), false);

      menuitem.id = menuitemId;
      menuitem.setAttribute("command", commandId);
      menuitem.setAttribute("command", commandId);
      menuitem.setAttribute("hidden", aHiddenFlag);

      commandset.appendChild(command);
      menupopup.appendChild(menuitem);
    }

    



    function createMenuSeparator() {
      let menuseparator = document.createElement("menuseparator");
      menupopup.appendChild(menuseparator);
    }
  },

  





  _destroyContextMenu: function DVS__destroyContextMenu(aContextMenu) {
    dumpn("Destroying context menu: " +
      aContextMenu.commandsetId + " & " + aContextMenu.menupopupId);

    let commandset = document.getElementById(aContextMenu.commandsetId);
    let menupopup = document.getElementById(aContextMenu.menupopupId);
    commandset.parentNode.removeChild(commandset);
    menupopup.parentNode.removeChild(menupopup);
  },

  





  _onBreakpointRemoved: function DVS__onBreakpointRemoved(aItem) {
    dumpn("Finalizing breakpoint item: " + aItem);

    let { sourceLocation: url, lineNumber: line, popup } = aItem.attachment;
    this._destroyContextMenu(popup);
    this._breakpointsCache.delete(this._getBreakpointKey(url, line));
  },

  


  _onEditorLoad: function DVS__onEditorLoad({ detail: editor }) {
    editor.addEventListener("Selection", this._onEditorSelection, false);
    editor.addEventListener("ContextMenu", this._onEditorContextMenu, false);
  },

  


  _onEditorUnload: function DVS__onEditorUnload({ detail: editor }) {
    editor.removeEventListener("Selection", this._onEditorSelection, false);
    editor.removeEventListener("ContextMenu", this._onEditorContextMenu, false);
  },

  


  _onEditorSelection: function DVS__onEditorSelection(e) {
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

  


  _onEditorContextMenu: function DVS__onEditorContextMenu({ x, y }) {
    let offset = DebuggerView.editor.getOffsetAtLocation(x, y);
    let line = DebuggerView.editor.getLineAtOffset(offset);
    this._editorContextMenuLineNumber = line;
  },

  


  _onSourceMouseDown: function DVS__onSourceMouseDown(e) {
    let item = this.getItemForElement(e.target);
    if (item) {
      
      this.selectedItem = item;
    }
  },

  


  _onSourceSelect: function DVS__onSourceSelect() {
    if (!this.refresh()) {
      return;
    }

    let selectedSource = this.selectedItem.attachment.source;
    if (DebuggerView.editorSource != selectedSource) {
      DebuggerView.editorSource = selectedSource;
    }
  },

  


  _onSourceClick: function DVS__onSourceClick() {
    
    DebuggerView.Filtering.target = this;
  },

  


  _onBreakpointClick: function DVS__onBreakpointClick(e) {
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

  


  _onBreakpointCheckboxClick: function DVS__onBreakpointCheckboxClick(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let { sourceLocation: url, lineNumber: line, disabled } = breakpointItem.attachment;

    this[disabled ? "enableBreakpoint" : "disableBreakpoint"](url, line, {
      silent: true
    });

    
    e.preventDefault();
    e.stopPropagation();
  },

  


  _onConditionalPopupShowing: function DVS__onConditionalPopupShowing() {
    this._conditionalPopupVisible = true;
  },

  


  _onConditionalPopupShown: function DVS__onConditionalPopupShown() {
    this._cbTextbox.focus();
    this._cbTextbox.select();
  },

  


  _onConditionalPopupHiding: function DVS__onConditionalPopupHiding() {
    this._conditionalPopupVisible = false;
  },

  


  _onConditionalTextboxInput: function DVS__onConditionalTextboxInput() {
    this.selectedClient.conditionalExpression = this._cbTextbox.value;
  },

  


  _onConditionalTextboxKeyPress: function DVS__onConditionalTextboxKeyPress(e) {
    if (e.keyCode == e.DOM_VK_RETURN || e.keyCode == e.DOM_VK_ENTER) {
      this._hideConditionalPopup();
    }
  },

  


  _onCmdAddBreakpoint: function BP__onCmdAddBreakpoint() {
    
    
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

  


  _onCmdAddConditionalBreakpoint: function BP__onCmdAddConditionalBreakpoint() {
    
    
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

  





  _onSetConditional: function DVS__onSetConditional(aDetails) {
    let { sourceLocation: url, lineNumber: line, actor } = aDetails;
    let breakpointItem = this.getBreakpoint(url, line);
    this.highlightBreakpoint(url, line, { openPopup: true });
  },

  





  _onEnableSelf: function DVS__onEnableSelf(aDetails) {
    let { sourceLocation: url, lineNumber: line, actor } = aDetails;

    if (this.enableBreakpoint(url, line)) {
      let prefix = "bp-cMenu-"; 
      let enableSelfId = prefix + "enableSelf-" + actor + "-menuitem";
      let disableSelfId = prefix + "disableSelf-" + actor + "-menuitem";
      document.getElementById(enableSelfId).setAttribute("hidden", "true");
      document.getElementById(disableSelfId).removeAttribute("hidden");
    }
  },

  





  _onDisableSelf: function DVS__onDisableSelf(aDetails) {
    let { sourceLocation: url, lineNumber: line, actor } = aDetails;

    if (this.disableBreakpoint(url, line)) {
      let prefix = "bp-cMenu-"; 
      let enableSelfId = prefix + "enableSelf-" + actor + "-menuitem";
      let disableSelfId = prefix + "disableSelf-" + actor + "-menuitem";
      document.getElementById(enableSelfId).removeAttribute("hidden");
      document.getElementById(disableSelfId).setAttribute("hidden", "true");
    }
  },

  





  _onDeleteSelf: function DVS__onDeleteSelf(aDetails) {
    let { sourceLocation: url, lineNumber: line } = aDetails;
    let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);

    this.removeBreakpoint(url, line);
    DebuggerController.Breakpoints.removeBreakpoint(breakpointClient);
  },

  





  _onEnableOthers: function DVS__onEnableOthers(aDetails) {
    for (let [, item] of this._breakpointsCache) {
      if (item.attachment.actor != aDetails.actor) {
        this._onEnableSelf(item.attachment);
      }
    }
  },

  





  _onDisableOthers: function DVS__onDisableOthers(aDetails) {
    for (let [, item] of this._breakpointsCache) {
      if (item.attachment.actor != aDetails.actor) {
        this._onDisableSelf(item.attachment);
      }
    }
  },

  





  _onDeleteOthers: function DVS__onDeleteOthers(aDetails) {
    for (let [, item] of this._breakpointsCache) {
      if (item.attachment.actor != aDetails.actor) {
        this._onDeleteSelf(item.attachment);
      }
    }
  },

  





  _onEnableAll: function DVS__onEnableAll(aDetails) {
    this._onEnableOthers(aDetails);
    this._onEnableSelf(aDetails);
  },

  





  _onDisableAll: function DVS__onDisableAll(aDetails) {
    this._onDisableOthers(aDetails);
    this._onDisableSelf(aDetails);
  },

  





  _onDeleteAll: function DVS__onDeleteAll(aDetails) {
    this._onDeleteOthers(aDetails);
    this._onDeleteSelf(aDetails);
  },

  









  _getBreakpointKey: function DVS__getBreakpointKey(aSourceLocation, aLineNumber) {
    return [aSourceLocation, aLineNumber].join();
  },

  _breakpointsCache: null,
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

  




  clearCache: function SU_clearCache() {
    this._labelsCache = new Map();
    this._groupsCache = new Map();
  },

  







  getSourceLabel: function SU_getSourceLabel(aUrl) {
    let cachedLabel = this._labelsCache.get(aUrl);
    if (cachedLabel) {
      return cachedLabel;
    }

    let sourceLabel = this.trimUrl(aUrl);
    let unicodeLabel = this.convertToUnicode(window.unescape(sourceLabel));
    this._labelsCache.set(aUrl, unicodeLabel);
    return unicodeLabel;
  },

  








  getSourceGroup: function SU_getSourceGroup(aUrl) {
    let cachedGroup = this._groupsCache.get(aUrl);
    if (cachedGroup) {
      return cachedGroup;
    }

    try {
      
      var uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    } catch (e) {
      
      return "";
    }

    let { scheme, hostPort, directory, fileName } = uri;
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
    let unicodeLabel = this.convertToUnicode(window.unescape(groupLabel));
    this._groupsCache.set(aUrl, unicodeLabel)
    return unicodeLabel;
  },

  












  trimUrlLength: function SU_trimUrlLength(aUrl, aLength, aSection) {
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

  







  trimUrlQuery: function SU_trimUrlQuery(aUrl) {
    let length = aUrl.length;
    let q1 = aUrl.indexOf('?');
    let q2 = aUrl.indexOf('&');
    let q3 = aUrl.indexOf('#');
    let q = Math.min(q1 != -1 ? q1 : length,
                     q2 != -1 ? q2 : length,
                     q3 != -1 ? q3 : length);

    return aUrl.slice(0, q);
  },

  












  trimUrl: function SU_trimUrl(aUrl, aLabel, aSeq) {
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
  },

  









  convertToUnicode: function SU_convertToUnicode(aString, aCharset) {
    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
        .createInstance(Ci.nsIScriptableUnicodeConverter);

    try {
      if (aCharset) {
        converter.charset = aCharset;
      }
      return converter.ConvertToUnicode(aString);
    } catch(e) {
      return aString;
    }
  }
};




function WatchExpressionsView() {
  dumpn("WatchExpressionsView was instantiated");

  this._cache = []; 
  this.switchExpression = this.switchExpression.bind(this);
  this.deleteExpression = this.deleteExpression.bind(this);
  this._createItemView = this._createItemView.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onClose = this._onClose.bind(this);
  this._onBlur = this._onBlur.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
}

create({ constructor: WatchExpressionsView, proto: MenuContainer.prototype }, {
  


  initialize: function DVWE_initialize() {
    dumpn("Initializing the WatchExpressionsView");

    this.node = new ListWidget(document.getElementById("expressions"));
    this._variables = document.getElementById("variables");

    this.node.permaText = L10N.getStr("addWatchExpressionText");
    this.node.itemFactory = this._createItemView;
    this.node.setAttribute("context", "debuggerWatchExpressionsContextMenu");
    this.node.addEventListener("click", this._onClick, false);
  },

  


  destroy: function DVWE_destroy() {
    dumpn("Destroying the WatchExpressionsView");

    this.node.removeEventListener("click", this._onClick, false);
  },

  





  addExpression: function DVWE_addExpression(aExpression = "") {
    
    DebuggerView.showInstrumentsPane();

    
    let expressionItem = this.push([, aExpression], {
      index: 0, 
      relaxed: true, 
      attachment: {
        initialExpression: aExpression,
        currentExpression: "",
        id: this._generateId()
      }
    });

    
    expressionItem.attachment.inputNode.select();
    expressionItem.attachment.inputNode.focus();
    this._variables.scrollTop = 0;

    this._cache.splice(0, 0, expressionItem);
  },

  





  removeExpressionAt: function DVWE_removeExpressionAt(aIndex) {
    this.remove(this._cache[aIndex]);
    this._cache.splice(aIndex, 1);
  },

  









  switchExpression: function DVWE_switchExpression(aVar, aExpression) {
    let expressionItem =
      [i for (i of this._cache) if (i.attachment.currentExpression == aVar.name)][0];

    
    if (!aExpression || this.getExpressions().indexOf(aExpression) != -1) {
      this.deleteExpression(aVar);
      return;
    }

    
    expressionItem.attachment.currentExpression = aExpression;
    expressionItem.attachment.inputNode.value = aExpression;

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  







  deleteExpression: function DVWE_deleteExpression(aVar) {
    let expressionItem =
      [i for (i of this._cache) if (i.attachment.currentExpression == aVar.name)][0];

    
    this.removeExpressionAt(this._cache.indexOf(expressionItem));

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  







  getExpression: function DVWE_getExpression(aIndex) {
    return this._cache[aIndex].attachment.currentExpression;
  },

  





  getExpressions: function DVWE_getExpressions() {
    return [item.attachment.currentExpression for (item of this._cache)];
  },

  







  _createItemView: function DVWE__createItemView(aElementNode, aAttachment) {
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

    aElementNode.id = "expression-" + aAttachment.id;
    aElementNode.className = "dbg-expression title";

    aElementNode.appendChild(arrowNode);
    aElementNode.appendChild(inputNode);
    aElementNode.appendChild(closeNode);

    aAttachment.arrowNode = arrowNode;
    aAttachment.inputNode = inputNode;
    aAttachment.closeNode = closeNode;
  },

  


  _onCmdAddExpression: function BP__onCmdAddExpression(aText) {
    
    if (this.getExpressions().indexOf("") == -1) {
      this.addExpression(aText || DebuggerView.editor.getSelectedText());
    }
  },

  


  _onCmdRemoveAllExpressions: function BP__onCmdRemoveAllExpressions() {
    
    this.empty();
    this._cache.length = 0;

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  


  _onClick: function DVWE__onClick(e) {
    if (e.button != 0) {
      
      return;
    }
    let expressionItem = this.getItemForElement(e.target);
    if (!expressionItem) {
      
      this.addExpression();
    }
  },

  


  _onClose: function DVWE__onClose(e) {
    let expressionItem = this.getItemForElement(e.target);
    this.removeExpressionAt(this._cache.indexOf(expressionItem));

    
    DebuggerController.StackFrames.syncWatchExpressions();

    
    e.preventDefault();
    e.stopPropagation();
  },

  


  _onBlur: function DVWE__onBlur({ target: textbox }) {
    let expressionItem = this.getItemForElement(textbox);
    let oldExpression = expressionItem.attachment.currentExpression;
    let newExpression = textbox.value.trim();

    
    if (!newExpression) {
      this.removeExpressionAt(this._cache.indexOf(expressionItem));
    }
    
    else if (!oldExpression && this.getExpressions().indexOf(newExpression) != -1) {
      this.removeExpressionAt(this._cache.indexOf(expressionItem));
    }
    
    else {
      expressionItem.attachment.currentExpression = newExpression;
    }

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  


  _onKeyPress: function DVWE__onKeyPress(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
      case e.DOM_VK_ESCAPE:
        DebuggerView.editor.focus();
        return;
    }
  },

  



  _generateId: (function() {
    let count = 0;
    return function DVWE__generateId() {
      return (++count) + "";
    };
  })(),

  _variables: null,
  _cache: null
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

create({ constructor: GlobalSearchView, proto: MenuContainer.prototype }, {
  


  initialize: function DVGS_initialize() {
    dumpn("Initializing the GlobalSearchView");

    this.node = new ListWidget(document.getElementById("globalsearch"));
    this._splitter = document.querySelector("#globalsearch + .devtools-horizontal-splitter");

    this.node.emptyText = L10N.getStr("noMatchingStringsText");
    this.node.itemFactory = this._createItemView;
    this.node.addEventListener("scroll", this._onScroll, false);
  },

  


  destroy: function DVGS_destroy() {
    dumpn("Destroying the GlobalSearchView");

    this.node.removeEventListener("scroll", this._onScroll, false);
  },

  



  get hidden()
    this.node.getAttribute("hidden") == "true" ||
    this._splitter.getAttribute("hidden") == "true",

  



  set hidden(aFlag) {
    this.node.setAttribute("hidden", aFlag);
    this._splitter.setAttribute("hidden", aFlag);
  },

  


  clearView: function DVGS_clearView() {
    this.hidden = true;
    this.empty();
    window.dispatchEvent(document, "Debugger:GlobalSearch:ViewCleared");
  },

  


  focusNextMatch: function DVGS_focusNextMatch() {
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

  


  focusPrevMatch: function DVGS_focusPrevMatch() {
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

  





  scheduleSearch: function DVGS_scheduleSearch(aQuery) {
    if (!this.delayedSearch) {
      this.performSearch(aQuery);
      return;
    }
    let delay = Math.max(GLOBAL_SEARCH_ACTION_MAX_DELAY / aQuery.length, 0);

    window.clearTimeout(this._searchTimeout);
    this._searchFunction = this._startSearch.bind(this, aQuery);
    this._searchTimeout = window.setTimeout(this._searchFunction, delay);
  },

  





  performSearch: function DVGS_performSearch(aQuery) {
    window.clearTimeout(this._searchTimeout);
    this._searchFunction = null;
    this._startSearch(aQuery);
  },

  





  _startSearch: function DVGS__startSearch(aQuery) {
    this._searchedToken = aQuery;

    DebuggerController.SourceScripts.fetchSources(DebuggerView.Sources.values, {
      onFinished: this._performGlobalSearch
    });
  },

  



  _performGlobalSearch: function DVGS__performGlobalSearch() {
    
    let token = this._searchedToken;

    
    if (!token) {
      this.clearView();
      window.dispatchEvent(document, "Debugger:GlobalSearch:TokenEmpty");
      return;
    }

    
    let lowerCaseToken = token.toLowerCase();
    let tokenLength = token.length;

    
    let globalResults = new GlobalResults();
    let sourcesCache = DebuggerController.SourceScripts.getCache();

    for (let [location, contents] of sourcesCache) {
      
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

        lowerCaseLine.split(lowerCaseToken).reduce(function(prev, curr, index, {length}) {
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

  





  _createGlobalResultsUI: function DVGS__createGlobalResultsUI(aGlobalResults) {
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

  









  _createSourceResultsUI:
  function DVGS__createSourceResultsUI(aLocation, aSourceResults, aExpandFlag) {
    
    let sourceResultsItem = this.push([aLocation, aSourceResults.matchCount], {
      index: -1, 
      relaxed: true, 
      attachment: {
        sourceResults: aSourceResults,
        expandFlag: aExpandFlag
      }
    });
  },

  











  _createItemView:
  function DVGS__createItemView(aElementNode, aAttachment, aLocation, aMatchCount) {
    let { sourceResults, expandFlag } = aAttachment;

    sourceResults.createView(aElementNode, aLocation, aMatchCount, expandFlag, {
      onHeaderClick: this._onHeaderClick,
      onLineClick: this._onLineClick,
      onMatchClick: this._onMatchClick
    });
  },

  


  _onHeaderClick: function DVGS__onHeaderClick(e) {
    let sourceResultsItem = SourceResults.getItemForElement(e.target);
    sourceResultsItem.instance.toggle(e);
  },

  


  _onLineClick: function DVGLS__onLineClick(e) {
    let lineResultsItem = LineResults.getItemForElement(e.target);
    this._onMatchClick({ target: lineResultsItem.firstMatch });
  },

  


  _onMatchClick: function DVGLS__onMatchClick(e) {
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

  


  _onScroll: function DVGS__onScroll(e) {
    for (let item in this) {
      this._expandResultsIfNeeded(item.target);
    }
  },

  





  _expandResultsIfNeeded: function DVGS__expandResultsIfNeeded(aTarget) {
    let sourceResultsItem = SourceResults.getItemForElement(aTarget);
    if (sourceResultsItem.instance.toggled ||
        sourceResultsItem.instance.expanded) {
      return;
    }
    let { top, height } = aTarget.getBoundingClientRect();
    let { clientHeight } = this.node._parent;

    if (top - height <= clientHeight || this._forceExpandResults) {
      sourceResultsItem.instance.expand();
    }
  },

  





  _scrollMatchIntoViewIfNeeded:  function DVGS__scrollMatchIntoViewIfNeeded(aMatch) {
    let boxObject = this.node._parent.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    boxObject.ensureElementIsVisible(aMatch);
  },

  





  _bounceMatch: function DVGS__bounceMatch(aMatch) {
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
  







  add: function GR_add(aLocation, aSourceResults) {
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
  







  add: function SR_add(aLineNumber, aLineResults) {
    this._store.set(aLineNumber, aLineResults);
  },

  


  matchCount: -1,

  


  expand: function SR_expand() {
    this._target.resultsContainer.removeAttribute("hidden")
    this._target.arrow.setAttribute("open", "");
  },

  


  collapse: function SR_collapse() {
    this._target.resultsContainer.setAttribute("hidden", "true");
    this._target.arrow.removeAttribute("open");
  },

  


  toggle: function SR_toggle(e) {
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

  















  createView:
  function SR_createView(aElementNode, aLocation, aMatchCount, aExpandFlag, aCallbacks) {
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
  









  add: function LC_add(aString, aRange, aMatchFlag) {
    this._store.push({
      string: aString,
      range: aRange,
      match: !!aMatchFlag
    });
  },

  



  get target() this._target,

  











  createView: function LR_createView(aContainer, aLineNumber, aCallbacks) {
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

  





  _entangleMatch: function LR__entangleMatch(aLineNumber, aNode, aMatchChunk) {
    LineResults._itemsByElement.set(aNode, {
      lineNumber: aLineNumber,
      lineData: aMatchChunk
    });
  },

  




  _entangleLine: function LR__entangleLine(aNode, aFirstMatch) {
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
LineResults.prototype.__iterator__ = function DVGS_iterator() {
  for (let item of this._store) {
    yield item;
  }
};









SourceResults.getItemForElement =
LineResults.getItemForElement = function DVGS_getItemForElement(aElement) {
  return MenuContainer.prototype.getItemForElement.call(this, aElement);
};









SourceResults.getElementAtIndex =
LineResults.getElementAtIndex = function DVGS_getElementAtIndex(aIndex) {
  for (let [element, item] of this._itemsByElement) {
    if (!item.nonenumerable && !aIndex--) {
      return element;
    }
  }
  return null;
};









SourceResults.indexOfElement =
LineResults.indexOfElement = function DVGS_indexOFElement(aElement) {
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
LineResults.size = function DVGS_size() {
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
