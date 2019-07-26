




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
  this._onSourceCheck = this._onSourceCheck.bind(this);
  this._onStopBlackBoxing = this._onStopBlackBoxing.bind(this);
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

    this.widget = new SideMenuWidget(document.getElementById("sources"), {
      showCheckboxes: true,
      showArrows: true
    });

    this.emptyText = L10N.getStr("noSourcesText");
    this.unavailableText = L10N.getStr("noMatchingSourcesText");
    this._blackBoxCheckboxTooltip = L10N.getStr("blackBoxCheckboxTooltip");

    this._commandset = document.getElementById("debuggerCommands");
    this._popupset = document.getElementById("debuggerPopupset");
    this._cmPopup = document.getElementById("sourceEditorContextMenu");
    this._cbPanel = document.getElementById("conditional-breakpoint-panel");
    this._cbTextbox = document.getElementById("conditional-breakpoint-panel-textbox");
    this._editorDeck = document.getElementById("editor-deck");
    this._stopBlackBoxButton = document.getElementById("black-boxed-message-button");

    window.addEventListener("Debugger:EditorLoaded", this._onEditorLoad, false);
    window.addEventListener("Debugger:EditorUnloaded", this._onEditorUnload, false);
    this.widget.addEventListener("select", this._onSourceSelect, false);
    this.widget.addEventListener("click", this._onSourceClick, false);
    this.widget.addEventListener("check", this._onSourceCheck, false);
    this._stopBlackBoxButton.addEventListener("click", this._onStopBlackBoxing, false);
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
    this.widget.removeEventListener("check", this._onSourceCheck, false);
    this._stopBlackBoxButton.removeEventListener("click", this._onStopBlackBoxing, false);
    this._cbPanel.removeEventListener("popupshowing", this._onConditionalPopupShowing, false);
    this._cbPanel.removeEventListener("popupshowing", this._onConditionalPopupShown, false);
    this._cbPanel.removeEventListener("popuphiding", this._onConditionalPopupHiding, false);
    this._cbTextbox.removeEventListener("input", this._onConditionalTextboxInput, false);
    this._cbTextbox.removeEventListener("keypress", this._onConditionalTextboxKeyPress, false);
  },

  



  set preferredSource(aUrl) {
    this._preferredValue = aUrl;

    
    
    if (this.containsValue(aUrl)) {
      this.selectedValue = aUrl;
    }
  },

  








  addSource: function(aSource, aOptions = {}) {
    let url = aSource.url;
    let label = SourceUtils.getSourceLabel(url.split(" -> ").pop());
    let group = SourceUtils.getSourceGroup(url.split(" -> ").pop());

    
    this.push([label, url, group], {
      staged: aOptions.staged, 
      attachment: {
        checkboxState: !aSource.isBlackBoxed,
        checkboxTooltip: this._blackBoxCheckboxTooltip,
        source: aSource
      }
    });
  },

  











  addBreakpoint: function(aBreakpointData, aOptions = {}) {
    let { location, actor } = aBreakpointData;

    
    
    if (this.getBreakpoint(location)) {
      this.enableBreakpoint(location, { id: actor });
      return;
    }

    
    let sourceItem = this.getItemByValue(location.url);

    
    let breakpointArgs = Heritage.extend(aBreakpointData, aOptions);
    let breakpointView = this._createBreakpointView.call(this, breakpointArgs);
    let contextMenu = this._createContextMenu.call(this, breakpointArgs);

    
    sourceItem.append(breakpointView.container, {
      attachment: Heritage.extend(breakpointArgs, {
        url: location.url,
        line: location.line,
        view: breakpointView,
        popup: contextMenu
      }),
      attributes: [
        ["contextmenu", contextMenu.menupopupId]
      ],
      
      
      finalize: this._onBreakpointRemoved
    });

    
    if (aOptions.openPopup || !aOptions.noEditorUpdate) {
      this.highlightBreakpoint(location, aOptions);
    }
  },

  






  removeBreakpoint: function(aLocation) {
    
    
    let sourceItem = this.getItemByValue(aLocation.url);
    if (!sourceItem) {
      return;
    }
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return;
    }

    
    sourceItem.remove(breakpointItem);
  },

  







  getBreakpoint: function(aLocation) {
    return this.getItemForPredicate(aItem =>
      aItem.attachment.url == aLocation.url &&
      aItem.attachment.line == aLocation.line);
  },

  









  getOtherBreakpoints: function(aId, aStore = []) {
    for (let source in this) {
      for (let breakpointItem in source) {
        if (breakpointItem.attachment.actor != aId) {
          aStore.push(breakpointItem);
        }
      }
    }
    return aStore;
  },

  














  enableBreakpoint: function(aLocation, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return promise.reject(new Error("No breakpoint found."));
    }

    
    let attachment = breakpointItem.attachment;
    attachment.disabled = false;

    
    let prefix = "bp-cMenu-"; 
    let enableSelfId = prefix + "enableSelf-" + attachment.actor + "-menuitem";
    let disableSelfId = prefix + "disableSelf-" + attachment.actor + "-menuitem";
    document.getElementById(enableSelfId).setAttribute("hidden", "true");
    document.getElementById(disableSelfId).removeAttribute("hidden");

    
    if (aOptions.id) {
      attachment.view.container.id = "breakpoint-" + aOptions.id;
    }
    
    if (!aOptions.silent) {
      attachment.view.checkbox.setAttribute("checked", "true");
    }

    return DebuggerController.Breakpoints.addBreakpoint(aLocation, {
      
      
      noPaneUpdate: true
    });
  },

  













  disableBreakpoint: function(aLocation, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return promise.reject(new Error("No breakpoint found."));
    }

    
    let attachment = breakpointItem.attachment;
    attachment.disabled = true;

    
    let prefix = "bp-cMenu-"; 
    let enableSelfId = prefix + "enableSelf-" + attachment.actor + "-menuitem";
    let disableSelfId = prefix + "disableSelf-" + attachment.actor + "-menuitem";
    document.getElementById(enableSelfId).removeAttribute("hidden");
    document.getElementById(disableSelfId).setAttribute("hidden", "true");

    
    if (!aOptions.silent) {
      attachment.view.checkbox.removeAttribute("checked");
    }

    return DebuggerController.Breakpoints.removeBreakpoint(aLocation, {
      
      
      noPaneUpdate: true
    });
  },

  









  highlightBreakpoint: function(aLocation, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return;
    }

    
    this._selectBreakpoint(breakpointItem);

    
    if (!aOptions.noEditorUpdate) {
      DebuggerView.setEditorLocation(aLocation.url, aLocation.line, { noDebug: true });
    }

    
    
    if (aOptions.openPopup) {
      this._openConditionalPopup();
    } else {
      this._hideConditionalPopup();
    }
  },

  


  unhighlightBreakpoint: function() {
    this._unselectBreakpoint();
    this._hideConditionalPopup();
  },

  





  _selectBreakpoint: function(aItem) {
    if (this._selectedBreakpointItem == aItem) {
      return;
    }
    this._unselectBreakpoint();
    this._selectedBreakpointItem = aItem;
    this._selectedBreakpointItem.target.classList.add("selected");

    
    this.widget.ensureElementIsVisible(aItem.target);
  },

  


  _unselectBreakpoint: function() {
    if (!this._selectedBreakpointItem) {
      return;
    }
    this._selectedBreakpointItem.target.classList.remove("selected");
    this._selectedBreakpointItem = null;
  },

  


  _openConditionalPopup: function() {
    let breakpointItem = this._selectedBreakpointItem;
    let attachment = breakpointItem.attachment;

    
    
    let breakpointPromise = DebuggerController.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      breakpointPromise.then(aBreakpointClient => {
        let isConditionalBreakpoint = "conditionalExpression" in aBreakpointClient;
        let conditionalExpression = aBreakpointClient.conditionalExpression;
        doOpen.call(this, isConditionalBreakpoint ? conditionalExpression : "")
      });
    } else {
      doOpen.call(this, "")
    }

    function doOpen(aConditionalExpression) {
      
      
      this._cbTextbox.value = aConditionalExpression;

      
      
      this._cbPanel.hidden = false;
      this._cbPanel.openPopup(breakpointItem.attachment.view.lineNumber,
        BREAKPOINT_CONDITIONAL_POPUP_POSITION,
        BREAKPOINT_CONDITIONAL_POPUP_OFFSET_X,
        BREAKPOINT_CONDITIONAL_POPUP_OFFSET_Y);
    }
  },

  


  _hideConditionalPopup: function() {
    this._cbPanel.hidden = true;
    this._cbPanel.hidePopup();
  },

  











  _createBreakpointView: function(aOptions) {
    let checkbox = document.createElement("checkbox");
    checkbox.setAttribute("checked", "true");
    checkbox.className = "dbg-breakpoint-checkbox";

    let lineNumberNode = document.createElement("label");
    lineNumberNode.className = "plain dbg-breakpoint-line";
    lineNumberNode.setAttribute("value", aOptions.location.line);

    let lineTextNode = document.createElement("label");
    lineTextNode.className = "plain dbg-breakpoint-text";
    lineTextNode.setAttribute("value", aOptions.text);
    lineTextNode.setAttribute("crop", "end");
    lineTextNode.setAttribute("flex", "1");

    let tooltip = aOptions.text.substr(0, BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH);
    lineTextNode.setAttribute("tooltiptext", tooltip);

    let container = document.createElement("hbox");
    container.id = "breakpoint-" + aOptions.actor;
    container.className = "dbg-breakpoint side-menu-widget-item-other";
    container.classList.add("devtools-monospace");
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

    
    if (this._selectedBreakpointItem == aItem) {
      this._selectedBreakpointItem = null;
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

    let url = this.selectedValue;
    let lineStart = DebuggerView.editor.getLineAtOffset(start) + 1;
    let lineEnd = DebuggerView.editor.getLineAtOffset(end) + 1;
    let location = { url: url, line: lineStart };

    if (this.getBreakpoint(location) && lineStart == lineEnd) {
      this.highlightBreakpoint(location, { noEditorUpdate: true });
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
    
    DebuggerView.setEditorLocation(sourceItem.value);
    this.maybeShowBlackBoxMessage();
  },

  



  maybeShowBlackBoxMessage: function() {
    let sourceForm = this.selectedItem.attachment.source;
    let sourceClient = DebuggerController.activeThread.source(sourceForm);
    this._editorDeck.selectedIndex = sourceClient.isBlackBoxed ? 1 : 0;
  },

  


  _onSourceClick: function() {
    
    DebuggerView.Filtering.target = this;
  },

  


  _onSourceCheck: function({ detail: { checked }, target }) {
    let item = this.getItemForElement(target);
    DebuggerController.SourceScripts.blackBox(item.attachment.source, !checked);
  },

  


  _onStopBlackBoxing: function() {
    let sourceForm = this.selectedItem.attachment.source;
    DebuggerController.SourceScripts.blackBox(sourceForm, false);
  },

  


  _onBreakpointClick: function(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let attachment = breakpointItem.attachment;

    
    let breakpointPromise = DebuggerController.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      breakpointPromise.then(aBreakpointClient => {
        doHighlight.call(this, "conditionalExpression" in aBreakpointClient);
      });
    } else {
      doHighlight.call(this, false);
    }

    function doHighlight(aConditionalBreakpointFlag) {
      
      this.highlightBreakpoint(attachment, {
        
        
        
        openPopup: aConditionalBreakpointFlag && e.button == 0
      });
    }
  },

  


  _onBreakpointCheckboxClick: function(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let attachment = breakpointItem.attachment;

    
    this[attachment.disabled ? "enableBreakpoint" : "disableBreakpoint"](attachment, {
      
      
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
    let breakpointItem = this._selectedBreakpointItem;
    let attachment = breakpointItem.attachment;

    
    
    let breakpointPromise = DebuggerController.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      breakpointPromise.then(aBreakpointClient => {
        aBreakpointClient.conditionalExpression = this._cbTextbox.value;
      });
    }
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
    let location = { url: url, line: line };
    let breakpointItem = this.getBreakpoint(location);

    
    if (breakpointItem) {
      DebuggerController.Breakpoints.removeBreakpoint(location);
    }
    
    else {
      DebuggerController.Breakpoints.addBreakpoint(location);
    }
  },

  


  _onCmdAddConditionalBreakpoint: function() {
    
    
    if (this._editorContextMenuLineNumber >= 0) {
      DebuggerView.editor.setCaretPosition(this._editorContextMenuLineNumber);
    }
    
    this._editorContextMenuLineNumber = -1;

    let url =  DebuggerView.Sources.selectedValue;
    let line = DebuggerView.editor.getCaretPosition().line + 1;
    let location = { url: url, line: line };
    let breakpointItem = this.getBreakpoint(location);

    
    if (breakpointItem) {
      this.highlightBreakpoint(location, { openPopup: true });
    }
    
    else {
      DebuggerController.Breakpoints.addBreakpoint(location, { openPopup: true });
    }
  },

  





  _onSetConditional: function(aId) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let attachment = targetBreakpoint.attachment;

    
    this.highlightBreakpoint(attachment, { openPopup: true });
  },

  





  _onEnableSelf: function(aId) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let attachment = targetBreakpoint.attachment;

    
    this.enableBreakpoint(attachment);
  },

  





  _onDisableSelf: function(aId) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let attachment = targetBreakpoint.attachment;

    
    this.disableBreakpoint(attachment);
  },

  





  _onDeleteSelf: function(aId) {
    let targetBreakpoint = this.getItemForPredicate(aItem => aItem.attachment.actor == aId);
    let attachment = targetBreakpoint.attachment;

    
    this.removeBreakpoint(attachment);
    DebuggerController.Breakpoints.removeBreakpoint(attachment);
  },

  





  _onEnableOthers: function(aId) {
    let enableOthers = (aCallback) => {
      let other = this.getOtherBreakpoints(aId);
      let outstanding = other.map(e => this.enableBreakpoint(e.attachment));
      promise.all(outstanding).then(aCallback);
    }

    
    
    
    if (gThreadClient.state == "paused") {
      enableOthers();
    } else {
      gThreadClient.interrupt(() => enableOthers(() => gThreadClient.resume()));
    }
  },

  





  _onDisableOthers: function(aId) {
    let other = this.getOtherBreakpoints(aId);
    other.forEach(e => this._onDisableSelf(e.attachment.actor));
  },

  





  _onDeleteOthers: function(aId) {
    let other = this.getOtherBreakpoints(aId);
    other.forEach(e => this._onDeleteSelf(e.attachment.actor));
  },

  


  _onEnableAll: function() {
    this._onEnableOthers(null);
  },

  


  _onDisableAll: function() {
    this._onDisableOthers(null);
  },

  


  _onDeleteAll: function() {
    this._onDeleteOthers(null);
  },

  _commandset: null,
  _popupset: null,
  _cmPopup: null,
  _cbPanel: null,
  _cbTextbox: null,
  _selectedBreakpointItem: null,
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
    return this.items.map(e => e.attachment.currentExpression);
  },

  







  _createItemView: function(aElementNode, aAttachment) {
    let arrowNode = document.createElement("box");
    arrowNode.className = "dbg-expression-arrow";

    let inputNode = document.createElement("textbox");
    inputNode.className = "plain dbg-expression-input devtools-monospace";
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

  this._createItemView = this._createItemView.bind(this);
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
  },

  


  destroy: function() {
    dumpn("Destroying the GlobalSearchView");
  },

  



  set hidden(aFlag) {
    this.widget.setAttribute("hidden", aFlag);
    this._splitter.setAttribute("hidden", aFlag);
  },

  



  get hidden()
    this.widget.getAttribute("hidden") == "true" ||
    this._splitter.getAttribute("hidden") == "true",

  


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

  







  scheduleSearch: function(aToken, aWait) {
    
    let maxDelay = GLOBAL_SEARCH_ACTION_MAX_DELAY;
    let delay = aWait === undefined ? maxDelay / aToken.length : aWait;

    
    setNamedTimeout("global-search", delay, () => {
      
      let urls = DebuggerView.Sources.values;
      let sourcesFetched = DebuggerController.SourceScripts.getTextForSources(urls);
      sourcesFetched.then(aSources => this._doSearch(aToken, aSources));
    });
  },

  








  _doSearch: function(aToken, aSources) {
    
    if (!aToken) {
      this.clearView();
      window.dispatchEvent(document, "Debugger:GlobalSearch:TokenEmpty");
      return;
    }

    
    let lowerCaseToken = aToken.toLowerCase();
    let tokenLength = aToken.length;

    
    let globalResults = new GlobalResults();

    
    for (let [url, text] of aSources) {
      
      if (!text.toLowerCase().contains(lowerCaseToken)) {
        continue;
      }
      
      let sourceResults = new SourceResults(url, globalResults);

      
      text.split("\n").forEach((aString, aLine) => {
        
        let lowerCaseLine = aString.toLowerCase();

        
        if (!lowerCaseLine.contains(lowerCaseToken)) {
          return;
        }
        
        let lineResults = new LineResults(aLine, sourceResults);

        
        lowerCaseLine.split(lowerCaseToken).reduce((aPrev, aCurr, aIndex, aArray) => {
          let prevLength = aPrev.length;
          let currLength = aCurr.length;

          
          let unmatched = aString.substr(prevLength, currLength);
          lineResults.add(unmatched);

          
          
          if (aIndex != aArray.length - 1) {
            let matched = aString.substr(prevLength + currLength, tokenLength);
            let range = { start: prevLength + currLength, length: matched.length };
            lineResults.add(matched, range, true);
          }

          
          return aPrev + aToken + aCurr;
        }, "");

        if (lineResults.matchCount) {
          sourceResults.add(lineResults);
        }
      });

      if (sourceResults.matchCount) {
        globalResults.add(sourceResults);
      }
    }

    
    if (globalResults.matchCount) {
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

    for (let sourceResults in aGlobalResults) {
      if (i++ == 0) {
        this._createSourceResultsUI(sourceResults);
      } else {
        
        
        
        Services.tm.currentThread.dispatch({ run:
          this._createSourceResultsUI.bind(this, sourceResults)
        }, 0);
      }
    }
  },

  





  _createSourceResultsUI: function(aSourceResults) {
    
    this.push([], {
      index: -1, 
      relaxed: true, 
      attachment: {
        sourceResults: aSourceResults
      }
    });
  },

  







  _createItemView: function(aElementNode, aAttachment) {
    aAttachment.sourceResults.createView(aElementNode, {
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

    let url = sourceResultsItem.instance.url;
    let line = lineResultsItem.instance.line;
    DebuggerView.setEditorLocation(url, line + 1, { noDebug: true });

    let editor = DebuggerView.editor;
    let offset = editor.getCaretOffset();
    let { start, length } = lineResultsItem.lineData.range;
    editor.setSelection(offset + start, offset + start + length);
  },

  





  _scrollMatchIntoViewIfNeeded: function(aMatch) {
    
    
    let boxObject = this.widget._parent.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    boxObject.ensureElementIsVisible(aMatch);
  },

  





  _bounceMatch: function(aMatch) {
    Services.tm.currentThread.dispatch({ run: () => {
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
  _forceExpandResults: false
});





function GlobalResults() {
  this._store = [];
  SourceResults._itemsByElement = new Map();
  LineResults._itemsByElement = new Map();
}

GlobalResults.prototype = {
  





  add: function(aSourceResults) {
    this._store.push(aSourceResults);
  },

  


  get matchCount() this._store.length
};










function SourceResults(aUrl, aGlobalResults) {
  this.url = aUrl;
  this._globalResults = aGlobalResults;
  this._store = [];
}

SourceResults.prototype = {
  





  add: function(aLineResults) {
    this._store.push(aLineResults);
  },

  


  get matchCount() this._store.length,

  


  expand: function() {
    this._resultsContainer.removeAttribute("hidden");
    this._arrow.setAttribute("open", "");
  },

  


  collapse: function() {
    this._resultsContainer.setAttribute("hidden", "true");
    this._arrow.removeAttribute("open");
  },

  


  toggle: function(e) {
    this.expanded ^= 1;
  },

  



  get expanded()
    this._resultsContainer.getAttribute("hidden") != "true" &&
    this._arrow.hasAttribute("open"),

  



  set expanded(aFlag) this[aFlag ? "expand" : "collapse"](),

  



  get target() this._target,

  









  createView: function(aElementNode, aCallbacks) {
    this._target = aElementNode;

    let arrow = this._arrow = document.createElement("box");
    arrow.className = "arrow";

    let locationNode = document.createElement("label");
    locationNode.className = "plain dbg-results-header-location";
    locationNode.setAttribute("value", this.url);

    let matchCountNode = document.createElement("label");
    matchCountNode.className = "plain dbg-results-header-match-count";
    matchCountNode.setAttribute("value", "(" + this.matchCount + ")");

    let resultsHeader = this._resultsHeader = document.createElement("hbox");
    resultsHeader.className = "dbg-results-header";
    resultsHeader.setAttribute("align", "center")
    resultsHeader.appendChild(arrow);
    resultsHeader.appendChild(locationNode);
    resultsHeader.appendChild(matchCountNode);
    resultsHeader.addEventListener("click", aCallbacks.onHeaderClick, false);

    let resultsContainer = this._resultsContainer = document.createElement("vbox");
    resultsContainer.className = "dbg-results-container";
    resultsContainer.setAttribute("hidden", "true");

    
    
    
    for (let lineResults of this._store) {
      lineResults.createView(resultsContainer, aCallbacks);
    }
    if (this.matchCount < GLOBAL_SEARCH_EXPAND_MAX_RESULTS) {
      this.expand();
    }

    let resultsBox = document.createElement("vbox");
    resultsBox.setAttribute("flex", "1");
    resultsBox.appendChild(resultsHeader);
    resultsBox.appendChild(resultsContainer);

    aElementNode.id = "source-results-" + this.url;
    aElementNode.className = "dbg-source-results";
    aElementNode.appendChild(resultsBox);

    SourceResults._itemsByElement.set(aElementNode, { instance: this });
  },

  url: "",
  _globalResults: null,
  _store: null,
  _target: null,
  _arrow: null,
  _resultsHeader: null,
  _resultsContainer: null
};










function LineResults(aLine, aSourceResults) {
  this.line = aLine;
  this._sourceResults = aSourceResults;
  this._store = [];
  this._matchCount = 0;
}

LineResults.prototype = {
  









  add: function(aString, aRange, aMatchFlag) {
    this._store.push({ string: aString, range: aRange, match: !!aMatchFlag });
    this._matchCount += aMatchFlag ? 1 : 0;
  },

  


  get matchCount() this._matchCount,

  



  get target() this._target,

  









  createView: function(aElementNode, aCallbacks) {
    this._target = aElementNode;

    let lineNumberNode = document.createElement("label");
    lineNumberNode.className = "plain dbg-results-line-number";
    lineNumberNode.classList.add("devtools-monospace");
    lineNumberNode.setAttribute("value", this.line + 1);

    let lineContentsNode = document.createElement("hbox");
    lineContentsNode.className = "light list-widget-item dbg-results-line-contents";
    lineContentsNode.classList.add("devtools-monospace");
    lineContentsNode.setAttribute("flex", "1");

    let lineString = "";
    let lineLength = 0;
    let firstMatch = null;

    for (let lineChunk of this._store) {
      let { string, range, match } = lineChunk;
      lineString = string.substr(0, GLOBAL_SEARCH_LINE_MAX_LENGTH - lineLength);
      lineLength += string.length;

      let lineChunkNode = document.createElement("label");
      lineChunkNode.className = "plain dbg-results-line-contents-string";
      lineChunkNode.setAttribute("value", lineString);
      lineChunkNode.setAttribute("match", match);
      lineContentsNode.appendChild(lineChunkNode);

      if (match) {
        this._entangleMatch(lineChunkNode, lineChunk);
        lineChunkNode.addEventListener("click", aCallbacks.onMatchClick, false);
        firstMatch = firstMatch || lineChunkNode;
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

    aElementNode.appendChild(searchResult);
  },

  




  _entangleMatch: function(aNode, aMatchChunk) {
    LineResults._itemsByElement.set(aNode, {
      instance: this,
      lineData: aMatchChunk
    });
  },

  




  _entangleLine: function(aNode, aFirstMatch) {
    LineResults._itemsByElement.set(aNode, {
      instance: this,
      firstMatch: aFirstMatch,
      ignored: true
    });
  },

  


  _ellipsis: (function() {
    let label = document.createElement("label");
    label.className = "plain dbg-results-line-contents-string";
    label.setAttribute("value", L10N.ellipsis);
    return label;
  })(),

  line: 0,
  _sourceResults: null,
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
    if (!item.ignored && !aIndex--) {
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
    if (!item.ignored) {
      count++;
    }
  }
  return -1;
};







SourceResults.size =
LineResults.size = function() {
  let count = 0;
  for (let [, item] of this._itemsByElement) {
    if (!item.ignored) {
      count++;
    }
  }
  return count;
};




DebuggerView.Sources = new SourcesView();
DebuggerView.WatchExpressions = new WatchExpressionsView();
DebuggerView.GlobalSearch = new GlobalSearchView();
