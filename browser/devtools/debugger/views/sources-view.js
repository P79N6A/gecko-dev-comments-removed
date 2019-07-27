


"use strict";



const KNOWN_SOURCE_GROUPS = {
  "Add-on SDK": "resource://gre/modules/commonjs/",
};

KNOWN_SOURCE_GROUPS[L10N.getStr("evalGroupLabel")] = "eval";




function SourcesView(DebuggerController, DebuggerView) {
  dumpn("SourcesView was instantiated");

  this.Breakpoints = DebuggerController.Breakpoints;
  this.SourceScripts = DebuggerController.SourceScripts;
  this.DebuggerView = DebuggerView;

  this.togglePrettyPrint = this.togglePrettyPrint.bind(this);
  this.toggleBlackBoxing = this.toggleBlackBoxing.bind(this);
  this.toggleBreakpoints = this.toggleBreakpoints.bind(this);

  this._onEditorLoad = this._onEditorLoad.bind(this);
  this._onEditorUnload = this._onEditorUnload.bind(this);
  this._onEditorCursorActivity = this._onEditorCursorActivity.bind(this);
  this._onSourceSelect = this._onSourceSelect.bind(this);
  this._onStopBlackBoxing = this._onStopBlackBoxing.bind(this);
  this._onBreakpointRemoved = this._onBreakpointRemoved.bind(this);
  this._onBreakpointClick = this._onBreakpointClick.bind(this);
  this._onBreakpointCheckboxClick = this._onBreakpointCheckboxClick.bind(this);
  this._onConditionalPopupShowing = this._onConditionalPopupShowing.bind(this);
  this._onConditionalPopupShown = this._onConditionalPopupShown.bind(this);
  this._onConditionalPopupHiding = this._onConditionalPopupHiding.bind(this);
  this._onConditionalTextboxKeyPress = this._onConditionalTextboxKeyPress.bind(this);
  this._onCopyUrlCommand = this._onCopyUrlCommand.bind(this);
  this._onNewTabCommand = this._onNewTabCommand.bind(this);
}

SourcesView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the SourcesView");

    this.widget = new SideMenuWidget(document.getElementById("sources"), {
      contextMenu: document.getElementById("debuggerSourcesContextMenu"),
      showArrows: true
    });

    this.emptyText = L10N.getStr("noSourcesText");
    this._blackBoxCheckboxTooltip = L10N.getStr("blackBoxCheckboxTooltip");

    this._commandset = document.getElementById("debuggerCommands");
    this._popupset = document.getElementById("debuggerPopupset");
    this._cmPopup = document.getElementById("sourceEditorContextMenu");
    this._cbPanel = document.getElementById("conditional-breakpoint-panel");
    this._cbTextbox = document.getElementById("conditional-breakpoint-panel-textbox");
    this._blackBoxButton = document.getElementById("black-box");
    this._stopBlackBoxButton = document.getElementById("black-boxed-message-button");
    this._prettyPrintButton = document.getElementById("pretty-print");
    this._toggleBreakpointsButton = document.getElementById("toggle-breakpoints");
    this._newTabMenuItem = document.getElementById("debugger-sources-context-newtab");
    this._copyUrlMenuItem = document.getElementById("debugger-sources-context-copyurl");

    if (Prefs.prettyPrintEnabled) {
      this._prettyPrintButton.removeAttribute("hidden");
    }

    window.on(EVENTS.EDITOR_LOADED, this._onEditorLoad, false);
    window.on(EVENTS.EDITOR_UNLOADED, this._onEditorUnload, false);
    this.widget.addEventListener("select", this._onSourceSelect, false);
    this._stopBlackBoxButton.addEventListener("click", this._onStopBlackBoxing, false);
    this._cbPanel.addEventListener("popupshowing", this._onConditionalPopupShowing, false);
    this._cbPanel.addEventListener("popupshown", this._onConditionalPopupShown, false);
    this._cbPanel.addEventListener("popuphiding", this._onConditionalPopupHiding, false);
    this._cbTextbox.addEventListener("keypress", this._onConditionalTextboxKeyPress, false);
    this._copyUrlMenuItem.addEventListener("command", this._onCopyUrlCommand, false);
    this._newTabMenuItem.addEventListener("command", this._onNewTabCommand, false);

    this.allowFocusOnRightClick = true;
    this.autoFocusOnSelection = false;

    
    this.sortContents((aFirst, aSecond) => {
      return +(aFirst.attachment.label.toLowerCase() >
               aSecond.attachment.label.toLowerCase());
    });

    
    this.widget.groupSortPredicate = function(a, b) {
      if ((a in KNOWN_SOURCE_GROUPS) == (b in KNOWN_SOURCE_GROUPS)) {
        return a.localeCompare(b);
      }
      return (a in KNOWN_SOURCE_GROUPS) ? 1 : -1;
    };

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the SourcesView");

    window.off(EVENTS.EDITOR_LOADED, this._onEditorLoad, false);
    window.off(EVENTS.EDITOR_UNLOADED, this._onEditorUnload, false);
    this.widget.removeEventListener("select", this._onSourceSelect, false);
    this._stopBlackBoxButton.removeEventListener("click", this._onStopBlackBoxing, false);
    this._cbPanel.removeEventListener("popupshowing", this._onConditionalPopupShowing, false);
    this._cbPanel.removeEventListener("popupshowing", this._onConditionalPopupShown, false);
    this._cbPanel.removeEventListener("popuphiding", this._onConditionalPopupHiding, false);
    this._cbTextbox.removeEventListener("keypress", this._onConditionalTextboxKeyPress, false);
    this._copyUrlMenuItem.removeEventListener("command", this._onCopyUrlCommand, false);
    this._newTabMenuItem.removeEventListener("command", this._onNewTabCommand, false);
  },

  


  _addCommands: function() {
    XULUtils.addCommands(this._commandset, {
      addBreakpointCommand: e => this._onCmdAddBreakpoint(e),
      addConditionalBreakpointCommand: e => this._onCmdAddConditionalBreakpoint(e),
      blackBoxCommand: () => this.toggleBlackBoxing(),
      unBlackBoxButton: () => this._onStopBlackBoxing(),
      prettyPrintCommand: () => this.togglePrettyPrint(),
      toggleBreakpointsCommand: () =>this.toggleBreakpoints(),
      nextSourceCommand: () => this.selectNextItem(),
      prevSourceCommand: () => this.selectPrevItem()
    });
  },

  



  set preferredSource(aUrl) {
    this._preferredValue = aUrl;

    
    
    if (this.containsValue(aUrl)) {
      this.selectedValue = aUrl;
    }
  },

  








  addSource: function(aSource, aOptions = {}) {
    if (!aSource.url) {
      
      return;
    }

    let { label, group, unicodeUrl } = this._parseUrl(aSource);

    let contents = document.createElement("label");
    contents.className = "plain dbg-source-item";
    contents.setAttribute("value", label);
    contents.setAttribute("crop", "start");
    contents.setAttribute("flex", "1");
    contents.setAttribute("tooltiptext", unicodeUrl);

    
    if (gThreadClient.source(aSource).isBlackBoxed) {
      contents.classList.add("black-boxed");
    }

    
    this.push([contents, aSource.actor], {
      staged: aOptions.staged, 
      attachment: {
        label: label,
        group: group,
        checkboxState: !aSource.isBlackBoxed,
        checkboxTooltip: this._blackBoxCheckboxTooltip,
        source: aSource
      }
    });
  },

  _parseUrl: function(aSource) {
    let fullUrl = aSource.url;
    let url = fullUrl.split(" -> ").pop();
    let label = aSource.addonPath ? aSource.addonPath : SourceUtils.getSourceLabel(url);
    let group = aSource.addonID ? aSource.addonID : SourceUtils.getSourceGroup(url);

    return {
      label: label,
      group: group,
      unicodeUrl: NetworkHelper.convertToUnicode(unescape(fullUrl))
    };
  },

  







  addBreakpoint: function(aBreakpointClient, aOptions = {}) {
    let { location, disabled } = aBreakpointClient;

    
    
    if (this.getBreakpoint(location)) {
      this[disabled ? "disableBreakpoint" : "enableBreakpoint"](location);
      return;
    }

    
    let sourceItem = this.getItemByValue(this.getActorForLocation(location));

    
    let breakpointArgs = Heritage.extend(aBreakpointClient, aOptions);
    let breakpointView = this._createBreakpointView.call(this, breakpointArgs);
    let contextMenu = this._createContextMenu.call(this, breakpointArgs);

    
    sourceItem.append(breakpointView.container, {
      attachment: Heritage.extend(breakpointArgs, {
        actor: location.actor,
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

    window.emit(EVENTS.BREAKPOINT_SHOWN_IN_PANE);
  },

  






  removeBreakpoint: function(aLocation) {
    
    
    let sourceItem = this.getItemByValue(aLocation.actor);
    if (!sourceItem) {
      return;
    }
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return;
    }

    
    sourceItem.remove(breakpointItem);

    window.emit(EVENTS.BREAKPOINT_HIDDEN_IN_PANE);
  },

  







  getBreakpoint: function(aLocation) {
    return this.getItemForPredicate(aItem =>
      aItem.attachment.actor == aLocation.actor &&
      aItem.attachment.line == aLocation.line);
  },

  





  getAllBreakpoints: function(aStore = []) {
    return this.getOtherBreakpoints(undefined, aStore);
  },

  









  getOtherBreakpoints: function(aLocation = {}, aStore = []) {
    for (let source of this) {
      for (let breakpointItem of source) {
        let { actor, line } = breakpointItem.attachment;
        if (actor != aLocation.actor || line != aLocation.line) {
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
    let identifier = this.Breakpoints.getIdentifier(attachment);
    let enableSelfId = prefix + "enableSelf-" + identifier + "-menuitem";
    let disableSelfId = prefix + "disableSelf-" + identifier + "-menuitem";
    document.getElementById(enableSelfId).setAttribute("hidden", "true");
    document.getElementById(disableSelfId).removeAttribute("hidden");

    
    this._toggleBreakpointsButton.removeAttribute("checked");

    
    if (!aOptions.silent) {
      attachment.view.checkbox.setAttribute("checked", "true");
    }

    return this.Breakpoints.addBreakpoint(aLocation, {
      
      
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
    let identifier = this.Breakpoints.getIdentifier(attachment);
    let enableSelfId = prefix + "enableSelf-" + identifier + "-menuitem";
    let disableSelfId = prefix + "disableSelf-" + identifier + "-menuitem";
    document.getElementById(enableSelfId).removeAttribute("hidden");
    document.getElementById(disableSelfId).setAttribute("hidden", "true");

    
    if (!aOptions.silent) {
      attachment.view.checkbox.removeAttribute("checked");
    }

    return this.Breakpoints.removeBreakpoint(aLocation, {
      
      
      noPaneUpdate: true,
      
      
      rememberDisabled: true
    });
  },

  









  highlightBreakpoint: function(aLocation, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return;
    }

    
    this._selectBreakpoint(breakpointItem);

    
    if (!aOptions.noEditorUpdate) {
      this.DebuggerView.setEditorLocation(aLocation.actor, aLocation.line, { noDebug: true });
    }

    
    
    if (aOptions.openPopup) {
      this._openConditionalPopup();
    } else {
      this._hideConditionalPopup();
    }
  },

  



  highlightBreakpointAtCursor: function() {
    let actor = this.selectedValue;
    let line = this.DebuggerView.editor.getCursor().line + 1;

    let location = { actor: actor, line: line };
    this.highlightBreakpoint(location, { noEditorUpdate: true });
  },

  


  unhighlightBreakpoint: function() {
    this._hideConditionalPopup();
    this._unselectBreakpoint();
  },

   


  showBreakpointConditionThrownMessage: function(aLocation, aMessage = "") {
    let breakpointItem = this.getBreakpoint(aLocation);
    if (!breakpointItem) {
      return;
    }
    let attachment = breakpointItem.attachment;
    attachment.view.container.classList.add("dbg-breakpoint-condition-thrown");
    attachment.view.message.setAttribute("value", aMessage);
  },

  



  updateToolbarButtonsState: function() {
    const { source } = this.selectedItem.attachment;
    const sourceClient = gThreadClient.source(source);

    if (sourceClient.isBlackBoxed) {
      this._prettyPrintButton.setAttribute("disabled", true);
      this._blackBoxButton.setAttribute("checked", true);
    } else {
      this._prettyPrintButton.removeAttribute("disabled");
      this._blackBoxButton.removeAttribute("checked");
    }

    if (sourceClient.isPrettyPrinted) {
      this._prettyPrintButton.setAttribute("checked", true);
    } else {
      this._prettyPrintButton.removeAttribute("checked");
    }
  },

  


  togglePrettyPrint: Task.async(function*() {
    if (this._prettyPrintButton.hasAttribute("disabled")) {
      return;
    }

    const resetEditor = ([{ actor }]) => {
      
      if (actor == this.selectedValue) {
        this.DebuggerView.setEditorLocation(actor, 0, { force: true });
      }
    };

    const printError = ([{ url }, error]) => {
      DevToolsUtils.reportException("togglePrettyPrint", error);
    };

    this.DebuggerView.showProgressBar();
    const { source } = this.selectedItem.attachment;
    const sourceClient = gThreadClient.source(source);
    const shouldPrettyPrint = !sourceClient.isPrettyPrinted;

    if (shouldPrettyPrint) {
      this._prettyPrintButton.setAttribute("checked", true);
    } else {
      this._prettyPrintButton.removeAttribute("checked");
    }

    try {
      let resolution = yield this.SourceScripts.togglePrettyPrint(source);
      resetEditor(resolution);
    } catch (rejection) {
      printError(rejection);
    }

    this.DebuggerView.showEditor();
    this.updateToolbarButtonsState();
  }),

  


  toggleBlackBoxing: Task.async(function*() {
    const { source } = this.selectedItem.attachment;
    const sourceClient = gThreadClient.source(source);
    const shouldBlackBox = !sourceClient.isBlackBoxed;

    
    
    
    

    if (shouldBlackBox) {
      this._prettyPrintButton.setAttribute("disabled", true);
      this._blackBoxButton.setAttribute("checked", true);
    } else {
      this._prettyPrintButton.removeAttribute("disabled");
      this._blackBoxButton.removeAttribute("checked");
    }

    try {
      yield this.SourceScripts.setBlackBoxing(source, shouldBlackBox);
    } catch (e) {
      
    }

    this.updateToolbarButtonsState();
  }),

  


  toggleBreakpoints: function() {
    let breakpoints = this.getAllBreakpoints();
    let hasBreakpoints = breakpoints.length > 0;
    let hasEnabledBreakpoints = breakpoints.some(e => !e.attachment.disabled);

    if (hasBreakpoints && hasEnabledBreakpoints) {
      this._toggleBreakpointsButton.setAttribute("checked", true);
      this._onDisableAll();
    } else {
      this._toggleBreakpointsButton.removeAttribute("checked");
      this._onEnableAll();
    }
  },

  hidePrettyPrinting: function() {
    this._prettyPrintButton.style.display = 'none';

    if (this._blackBoxButton.style.display === 'none') {
      let sep = document.querySelector('#sources-toolbar .devtools-separator');
      sep.style.display = 'none';
    }
  },

  hideBlackBoxing: function() {
    this._blackBoxButton.style.display = 'none';

    if (this._prettyPrintButton.style.display === 'none') {
      let sep = document.querySelector('#sources-toolbar .devtools-separator');
      sep.style.display = 'none';
    }
  },

  











  getActorForLocation: function(aLocation) {
    if (aLocation.url) {
      for (var item of this) {
        let source = item.attachment.source;

        if (aLocation.url === source.url) {
          return source.actor;
        }
      }
    }
    return aLocation.actor;
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
    
    
    let breakpointPromise = this.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      breakpointPromise.then(aBreakpointClient => {
        let isConditionalBreakpoint = aBreakpointClient.hasCondition();
        let condition = aBreakpointClient.getCondition();
        doOpen.call(this, isConditionalBreakpoint ? condition : "")
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

    
    
    if (this._cbPanel.hidePopup) {
      this._cbPanel.hidePopup();
    }
  },

  












  _createBreakpointView: function(aOptions) {
    let { location, disabled, text, message } = aOptions;
    let identifier = this.Breakpoints.getIdentifier(location);

    let checkbox = document.createElement("checkbox");
    checkbox.setAttribute("checked", !disabled);
    checkbox.className = "dbg-breakpoint-checkbox";

    let lineNumberNode = document.createElement("label");
    lineNumberNode.className = "plain dbg-breakpoint-line";
    lineNumberNode.setAttribute("value", location.line);

    let lineTextNode = document.createElement("label");
    lineTextNode.className = "plain dbg-breakpoint-text";
    lineTextNode.setAttribute("value", text);
    lineTextNode.setAttribute("crop", "end");
    lineTextNode.setAttribute("flex", "1");

    let tooltip = text ? text.substr(0, BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH) : "";
    lineTextNode.setAttribute("tooltiptext", tooltip);

    let thrownNode = document.createElement("label");
    thrownNode.className = "plain dbg-breakpoint-condition-thrown-message dbg-breakpoint-text";
    thrownNode.setAttribute("value", message);
    thrownNode.setAttribute("crop", "end");
    thrownNode.setAttribute("flex", "1");

    let bpLineContainer = document.createElement("hbox");
    bpLineContainer.className = "plain dbg-breakpoint-line-container";
    bpLineContainer.setAttribute("flex", "1");

    bpLineContainer.appendChild(lineNumberNode);
    bpLineContainer.appendChild(lineTextNode);

    let bpDetailContainer = document.createElement("vbox");
    bpDetailContainer.className = "plain dbg-breakpoint-detail-container";
    bpDetailContainer.setAttribute("flex", "1");

    bpDetailContainer.appendChild(bpLineContainer);
    bpDetailContainer.appendChild(thrownNode);

    let container = document.createElement("hbox");
    container.id = "breakpoint-" + identifier;
    container.className = "dbg-breakpoint side-menu-widget-item-other";
    container.classList.add("devtools-monospace");
    container.setAttribute("align", "center");
    container.setAttribute("flex", "1");

    container.addEventListener("click", this._onBreakpointClick, false);
    checkbox.addEventListener("click", this._onBreakpointCheckboxClick, false);

    container.appendChild(checkbox);
    container.appendChild(bpDetailContainer);

    return {
      container: container,
      checkbox: checkbox,
      lineNumber: lineNumberNode,
      lineText: lineTextNode,
      message: thrownNode
    };
  },

  









  _createContextMenu: function(aOptions) {
    let { location, disabled } = aOptions;
    let identifier = this.Breakpoints.getIdentifier(location);

    let commandset = document.createElement("commandset");
    let menupopup = document.createElement("menupopup");
    commandset.id = "bp-cSet-" + identifier;
    menupopup.id = "bp-mPop-" + identifier;

    createMenuItem.call(this, "enableSelf", !disabled);
    createMenuItem.call(this, "disableSelf", disabled);
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
      commandsetId: commandset.id,
      menupopupId: menupopup.id
    };

    








    function createMenuItem(aName, aHiddenFlag) {
      let menuitem = document.createElement("menuitem");
      let command = document.createElement("command");

      let prefix = "bp-cMenu-"; 
      let commandId = prefix + aName + "-" + identifier + "-command";
      let menuitemId = prefix + aName + "-" + identifier + "-menuitem";

      let label = L10N.getStr("breakpointMenuItem." + aName);
      let func = "_on" + aName.charAt(0).toUpperCase() + aName.slice(1);

      command.id = commandId;
      command.setAttribute("label", label);
      command.addEventListener("command", () => this[func](location), false);

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

  


  _onCopyUrlCommand: function() {
    let selected = this.selectedItem && this.selectedItem.attachment;
    if (!selected) {
      return;
    }
    clipboardHelper.copyString(selected.source.url, document);
  },

  


  _onNewTabCommand: function() {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    let selected = this.selectedItem.attachment;
    win.openUILinkIn(selected.source.url, "tab", { relatedToCurrent: true });
  },

  





  _onBreakpointRemoved: function(aItem) {
    dumpn("Finalizing breakpoint item: " + aItem.stringify());

    
    let contextMenu = aItem.attachment.popup;
    document.getElementById(contextMenu.commandsetId).remove();
    document.getElementById(contextMenu.menupopupId).remove();

    
    if (this._selectedBreakpointItem == aItem) {
      this._selectedBreakpointItem = null;
    }
  },

  


  _onEditorLoad: function(aName, aEditor) {
    aEditor.on("cursorActivity", this._onEditorCursorActivity);
  },

  


  _onEditorUnload: function(aName, aEditor) {
    aEditor.off("cursorActivity", this._onEditorCursorActivity);
  },

  


  _onEditorCursorActivity: function(e) {
    let editor = this.DebuggerView.editor;
    let start  = editor.getCursor("start").line + 1;
    let end    = editor.getCursor().line + 1;
    let actor    = this.selectedValue;

    let location = { actor: actor, line: start };

    if (this.getBreakpoint(location) && start == end) {
      this.highlightBreakpoint(location, { noEditorUpdate: true });
    } else {
      this.unhighlightBreakpoint();
    }
  },

  


  _onSourceSelect: Task.async(function*({ detail: sourceItem }) {
    if (!sourceItem) {
      return;
    }
    const { source } = sourceItem.attachment;
    const sourceClient = gThreadClient.source(source);

    
    this.DebuggerView.setEditorLocation(sourceItem.value);

    
    if (Prefs.autoPrettyPrint && !sourceClient.isPrettyPrinted) {
      let isMinified = yield SourceUtils.isMinified(sourceClient);
      if (isMinified) {
        this.togglePrettyPrint();
      }
    }

    
    
    document.title = L10N.getFormatStr("DebuggerWindowScriptTitle",
                                       sourceItem.attachment.source.url);

    this.DebuggerView.maybeShowBlackBoxMessage();
    this.updateToolbarButtonsState();
  }),

  


  _onStopBlackBoxing: Task.async(function*() {
    const { source } = this.selectedItem.attachment;

    try {
      yield this.SourceScripts.setBlackBoxing(source, false);
    } catch (e) {
      
    }

    this.updateToolbarButtonsState();
  }),

  


  _onBreakpointClick: function(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let attachment = breakpointItem.attachment;

    
    let breakpointPromise = this.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      breakpointPromise.then(aBreakpointClient => {
        doHighlight.call(this, aBreakpointClient.hasCondition());
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
    window.emit(EVENTS.CONDITIONAL_BREAKPOINT_POPUP_SHOWING);
  },

  


  _onConditionalPopupShown: function() {
    this._cbTextbox.focus();
    this._cbTextbox.select();
  },

  


  _onConditionalPopupHiding: Task.async(function*() {
    this._conditionalPopupVisible = false; 

    let breakpointItem = this._selectedBreakpointItem;
    let attachment = breakpointItem.attachment;

    
    
    let breakpointPromise = this.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      let { location } = yield breakpointPromise;
      let condition = this._cbTextbox.value;
      yield this.Breakpoints.updateCondition(location, condition);
    }

    window.emit(EVENTS.CONDITIONAL_BREAKPOINT_POPUP_HIDING);
  }),

  


  _onConditionalTextboxKeyPress: function(e) {
    if (e.keyCode == e.DOM_VK_RETURN) {
      this._hideConditionalPopup();
    }
  },

  


  _onCmdAddBreakpoint: function(e) {
    let actor = this.selectedValue;
    let line = (e && e.sourceEvent.target.tagName == 'menuitem' ?
                this.DebuggerView.clickedLine + 1 :
                this.DebuggerView.editor.getCursor().line + 1);
    let location = { actor, line };
    let breakpointItem = this.getBreakpoint(location);

    
    if (breakpointItem) {
      this.Breakpoints.removeBreakpoint(location);
    }
    
    else {
      this.Breakpoints.addBreakpoint(location);
    }
  },

  


  _onCmdAddConditionalBreakpoint: function(e) {
    let actor = this.selectedValue;
    let line = (e && e.sourceEvent.target.tagName == 'menuitem' ?
                this.DebuggerView.clickedLine + 1 :
                this.DebuggerView.editor.getCursor().line + 1);
    let location = { actor, line };
    let breakpointItem = this.getBreakpoint(location);

    
    if (breakpointItem) {
      this.highlightBreakpoint(location, { openPopup: true });
    }
    
    else {
      this.Breakpoints.addBreakpoint(location, { openPopup: true });
    }
  },

  





  _onSetConditional: function(aLocation) {
    
    this.highlightBreakpoint(aLocation, { openPopup: true });
  },

  





  _onEnableSelf: function(aLocation) {
    
    this.enableBreakpoint(aLocation);
  },

  





  _onDisableSelf: function(aLocation) {
    
    this.disableBreakpoint(aLocation);
  },

  





  _onDeleteSelf: function(aLocation) {
    
    this.removeBreakpoint(aLocation);
    this.Breakpoints.removeBreakpoint(aLocation);
  },

  





  _onEnableOthers: function(aLocation) {
    let enableOthers = aCallback => {
      let other = this.getOtherBreakpoints(aLocation);
      let outstanding = other.map(e => this.enableBreakpoint(e.attachment));
      promise.all(outstanding).then(aCallback);
    }

    
    
    
    if (gThreadClient.state != "paused") {
      gThreadClient.interrupt(() => enableOthers(() => gThreadClient.resume()));
    } else {
      enableOthers();
    }
  },

  





  _onDisableOthers: function(aLocation) {
    let other = this.getOtherBreakpoints(aLocation);
    other.forEach(e => this._onDisableSelf(e.attachment));
  },

  





  _onDeleteOthers: function(aLocation) {
    let other = this.getOtherBreakpoints(aLocation);
    other.forEach(e => this._onDeleteSelf(e.attachment));
  },

  


  _onEnableAll: function() {
    this._onEnableOthers(undefined);
  },

  


  _onDisableAll: function() {
    this._onDisableOthers(undefined);
  },

  


  _onDeleteAll: function() {
    this._onDeleteOthers(undefined);
  },

  _commandset: null,
  _popupset: null,
  _cmPopup: null,
  _cbPanel: null,
  _cbTextbox: null,
  _selectedBreakpointItem: null,
  _conditionalPopupVisible: false
});

DebuggerView.Sources = new SourcesView(DebuggerController, DebuggerView);
