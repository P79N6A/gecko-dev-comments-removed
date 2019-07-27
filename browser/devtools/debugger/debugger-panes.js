




"use strict";


const SAMPLE_SIZE = 50; 
const INDENT_COUNT_THRESHOLD = 5; 
const CHARACTER_LIMIT = 250; 



const KNOWN_SOURCE_GROUPS = {
  "Add-on SDK": "resource://gre/modules/commonjs/",
};

KNOWN_SOURCE_GROUPS[L10N.getStr("evalGroupLabel")] = "eval";




function SourcesView() {
  dumpn("SourcesView was instantiated");

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
    utils.addCommands(this._commandset, {
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
    let identifier = DebuggerController.Breakpoints.getIdentifier(attachment);
    let enableSelfId = prefix + "enableSelf-" + identifier + "-menuitem";
    let disableSelfId = prefix + "disableSelf-" + identifier + "-menuitem";
    document.getElementById(enableSelfId).setAttribute("hidden", "true");
    document.getElementById(disableSelfId).removeAttribute("hidden");

    
    this._toggleBreakpointsButton.removeAttribute("checked");

    
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
    let identifier = DebuggerController.Breakpoints.getIdentifier(attachment);
    let enableSelfId = prefix + "enableSelf-" + identifier + "-menuitem";
    let disableSelfId = prefix + "disableSelf-" + identifier + "-menuitem";
    document.getElementById(enableSelfId).removeAttribute("hidden");
    document.getElementById(disableSelfId).setAttribute("hidden", "true");

    
    if (!aOptions.silent) {
      attachment.view.checkbox.removeAttribute("checked");
    }

    return DebuggerController.Breakpoints.removeBreakpoint(aLocation, {
      
      
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
      DebuggerView.setEditorLocation(aLocation.actor, aLocation.line, { noDebug: true });
    }

    
    
    if (aOptions.openPopup) {
      this._openConditionalPopup();
    } else {
      this._hideConditionalPopup();
    }
  },

  



  highlightBreakpointAtCursor: function() {
    let actor = DebuggerView.Sources.selectedValue;
    let line = DebuggerView.editor.getCursor().line + 1;

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
        DebuggerView.setEditorLocation(actor, 0, { force: true });
      }
    };

    const printError = ([{ url }, error]) => {
      DevToolsUtils.reportException("togglePrettyPrint", error);
    };

    DebuggerView.showProgressBar();
    const { source } = this.selectedItem.attachment;
    const sourceClient = gThreadClient.source(source);
    const shouldPrettyPrint = !sourceClient.isPrettyPrinted;

    if (shouldPrettyPrint) {
      this._prettyPrintButton.setAttribute("checked", true);
    } else {
      this._prettyPrintButton.removeAttribute("checked");
    }

    try {
      let resolution = yield DebuggerController.SourceScripts.togglePrettyPrint(source);
      resetEditor(resolution);
    } catch (rejection) {
      printError(rejection);
    }

    DebuggerView.showEditor();
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
      yield DebuggerController.SourceScripts.setBlackBoxing(source, shouldBlackBox);
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
    
    
    let breakpointPromise = DebuggerController.Breakpoints._getAdded(attachment);
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
    let identifier = DebuggerController.Breakpoints.getIdentifier(location);

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
    let identifier = DebuggerController.Breakpoints.getIdentifier(location);

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
    let editor = DebuggerView.editor;
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

    
    DebuggerView.setEditorLocation(sourceItem.value);

    
    if (Prefs.autoPrettyPrint && !sourceClient.isPrettyPrinted) {
      let isMinified = yield SourceUtils.isMinified(sourceClient);
      if (isMinified) {
        this.togglePrettyPrint();
      }
    }

    
    
    document.title = L10N.getFormatStr("DebuggerWindowScriptTitle",
                                       sourceItem.attachment.source.url);

    DebuggerView.maybeShowBlackBoxMessage();
    this.updateToolbarButtonsState();
  }),

  


  _onStopBlackBoxing: Task.async(function*() {
    const { source } = this.selectedItem.attachment;

    try {
      yield DebuggerController.SourceScripts.setBlackBoxing(source, false);
    } catch (e) {
      
    }

    this.updateToolbarButtonsState();
  }),

  


  _onBreakpointClick: function(e) {
    let sourceItem = this.getItemForElement(e.target);
    let breakpointItem = this.getItemForElement.call(sourceItem, e.target);
    let attachment = breakpointItem.attachment;

    
    let breakpointPromise = DebuggerController.Breakpoints._getAdded(attachment);
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

    
    
    let breakpointPromise = DebuggerController.Breakpoints._getAdded(attachment);
    if (breakpointPromise) {
      let { location } = yield breakpointPromise;
      let condition = this._cbTextbox.value;
      yield DebuggerController.Breakpoints.updateCondition(location, condition);
    }

    window.emit(EVENTS.CONDITIONAL_BREAKPOINT_POPUP_HIDING);
  }),

  


  _onConditionalTextboxKeyPress: function(e) {
    if (e.keyCode == e.DOM_VK_RETURN) {
      this._hideConditionalPopup();
    }
  },

  


  _onCmdAddBreakpoint: function(e) {
    let actor = DebuggerView.Sources.selectedValue;
    let line = (e && e.sourceEvent.target.tagName == 'menuitem' ?
                DebuggerView.clickedLine + 1 :
                DebuggerView.editor.getCursor().line + 1);
    let location = { actor, line };
    let breakpointItem = this.getBreakpoint(location);

    
    if (breakpointItem) {
      DebuggerController.Breakpoints.removeBreakpoint(location);
    }
    
    else {
      DebuggerController.Breakpoints.addBreakpoint(location);
    }
  },

  


  _onCmdAddConditionalBreakpoint: function(e) {
    let actor = DebuggerView.Sources.selectedValue;
    let line = (e && e.sourceEvent.target.tagName == 'menuitem' ?
                DebuggerView.clickedLine + 1 :
                DebuggerView.editor.getCursor().line + 1);
    let location = { actor, line };
    let breakpointItem = this.getBreakpoint(location);

    
    if (breakpointItem) {
      this.highlightBreakpoint(location, { openPopup: true });
    }
    
    else {
      DebuggerController.Breakpoints.addBreakpoint(location, { openPopup: true });
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
    DebuggerController.Breakpoints.removeBreakpoint(aLocation);
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




function TracerView() {
  this._selectedItem = null;
  this._matchingItems = null;
  this.widget = null;

  this._highlightItem = this._highlightItem.bind(this);
  this._isNotSelectedItem = this._isNotSelectedItem.bind(this);

  this._unhighlightMatchingItems =
    DevToolsUtils.makeInfallible(this._unhighlightMatchingItems.bind(this));
  this._onToggleTracing =
    DevToolsUtils.makeInfallible(this._onToggleTracing.bind(this));
  this._onStartTracing =
    DevToolsUtils.makeInfallible(this._onStartTracing.bind(this));
  this._onClear =
    DevToolsUtils.makeInfallible(this._onClear.bind(this));
  this._onSelect =
    DevToolsUtils.makeInfallible(this._onSelect.bind(this));
  this._onMouseOver =
    DevToolsUtils.makeInfallible(this._onMouseOver.bind(this));
  this._onSearch =
    DevToolsUtils.makeInfallible(this._onSearch.bind(this));
}

TracerView.MAX_TRACES = 200;

TracerView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the TracerView");

    this._traceButton = document.getElementById("trace");
    this._tracerTab = document.getElementById("tracer-tab");

    
    
    if (!Prefs.tracerEnabled) {
      this._traceButton.remove();
      this._traceButton = null;
      this._tracerTab.remove();
      this._tracerTab = null;
      return;
    }

    this.widget = new FastListWidget(document.getElementById("tracer-traces"));
    this._traceButton.removeAttribute("hidden");
    this._tracerTab.removeAttribute("hidden");

    this._search = document.getElementById("tracer-search");
    this._template = document.getElementsByClassName("trace-item-template")[0];
    this._templateItem = this._template.getElementsByClassName("trace-item")[0];
    this._templateTypeIcon = this._template.getElementsByClassName("trace-type")[0];
    this._templateNameNode = this._template.getElementsByClassName("trace-name")[0];

    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("mouseover", this._onMouseOver, false);
    this.widget.addEventListener("mouseout", this._unhighlightMatchingItems, false);
    this._search.addEventListener("input", this._onSearch, false);

    this._startTooltip = L10N.getStr("startTracingTooltip");
    this._stopTooltip = L10N.getStr("stopTracingTooltip");
    this._tracingNotStartedString = L10N.getStr("tracingNotStartedText");
    this._noFunctionCallsString = L10N.getStr("noFunctionCallsText");

    this._traceButton.setAttribute("tooltiptext", this._startTooltip);
    this.emptyText = this._tracingNotStartedString;

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the TracerView");

    if (!this.widget) {
      return;
    }

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("mouseover", this._onMouseOver, false);
    this.widget.removeEventListener("mouseout", this._unhighlightMatchingItems, false);
    this._search.removeEventListener("input", this._onSearch, false);
  },

  


  _addCommands: function() {
    utils.addCommands(document.getElementById('debuggerCommands'), {
      toggleTracing: () => this._onToggleTracing(),
      startTracing: () => this._onStartTracing(),
      clearTraces: () => this._onClear()
    });
  },

  


  _onToggleTracing: function() {
    if (DebuggerController.Tracer.tracing) {
      this._onStopTracing();
    } else {
      this._onStartTracing();
    }
  },

  






  _onStartTracing: function() {
    this._traceButton.setAttribute("checked", true);
    this._traceButton.setAttribute("tooltiptext", this._stopTooltip);

    this.empty();
    this.emptyText = this._noFunctionCallsString;

    let deferred = promise.defer();
    DebuggerController.Tracer.startTracing(deferred.resolve);
    return deferred.promise;
  },

  






  _onStopTracing: function() {
    this._traceButton.removeAttribute("checked");
    this._traceButton.setAttribute("tooltiptext", this._startTooltip);

    this.emptyText = this._tracingNotStartedString;

    let deferred = promise.defer();
    DebuggerController.Tracer.stopTracing(deferred.resolve);
    return deferred.promise;
  },

  


  _onClear: function() {
    this.empty();
  },

  










  _populateVariable: function(aName, aParent, aValue) {
    let item = aParent.addItem(aName, { value: aValue });

    if (aValue) {
      let wrappedValue = new DebuggerController.Tracer.WrappedObject(aValue);
      DebuggerView.Variables.controller.populate(item, wrappedValue);
      item.expand();
      item.twisty = false;
    }
  },

  







  _onSelect: function _onSelect({ detail: traceItem }) {
    if (!traceItem) {
      return;
    }

    const data = traceItem.attachment.trace;
    const { location: { url, line } } = data;
    DebuggerView.setEditorLocation(
      DebuggerView.Sources.getActorForLocation({ url }),
      line,
      { noDebug: true }
    );

    DebuggerView.Variables.empty();
    const scope = DebuggerView.Variables.addScope();

    if (data.type == "call") {
      const params = DevToolsUtils.zip(data.parameterNames, data.arguments);
      for (let [name, val] of params) {
        if (val === undefined) {
          scope.addItem(name, { value: "<value not available>" });
        } else {
          this._populateVariable(name, scope, val);
        }
      }
    } else {
      const varName = "<" + (data.type == "throw" ? "exception" : data.type) + ">";
      this._populateVariable(varName, scope, data.returnVal);
    }

    scope.expand();
    DebuggerView.showInstrumentsPane();
  },

  


  _highlightItem: function(aItem) {
    if (!aItem || !aItem.target) {
      return;
    }
    const trace = aItem.target.querySelector(".trace-item");
    trace.classList.add("selected-matching");
  },

  


  _unhighlightItem: function(aItem) {
    if (!aItem || !aItem.target) {
      return;
    }
    const match = aItem.target.querySelector(".selected-matching");
    if (match) {
      match.classList.remove("selected-matching");
    }
  },

  


  _unhighlightMatchingItems: function() {
    if (this._matchingItems) {
      this._matchingItems.forEach(this._unhighlightItem);
      this._matchingItems = null;
    }
  },

  


  _isNotSelectedItem: function(aItem) {
    return aItem !== this.selectedItem;
  },

  


  _highlightMatchingItems: function(aItem) {
    const frameId = aItem.attachment.trace.frameId;
    const predicate = e => e.attachment.trace.frameId == frameId;

    this._unhighlightMatchingItems();
    this._matchingItems = this.items.filter(predicate);
    this._matchingItems
      .filter(this._isNotSelectedItem)
      .forEach(this._highlightItem);
  },

  


  _onMouseOver: function({ target }) {
    const traceItem = this.getItemForElement(target);
    if (traceItem) {
      this._highlightMatchingItems(traceItem);
    }
  },

  


  _onSearch: function() {
    const query = this._search.value.trim().toLowerCase();
    const predicate = name => name.toLowerCase().contains(query);
    this.filterContents(item => predicate(item.attachment.trace.name));
  },

  


  selectTab: function() {
    const tabs = this._tracerTab.parentElement;
    tabs.selectedIndex = Array.indexOf(tabs.children, this._tracerTab);
  },

  



  commit: function() {
    WidgetMethods.commit.call(this);
    
    
    this.widget.flush();
  },

  





  addTrace: function(aTrace) {
    
    let view = this._createView(aTrace);

    
    this.push([view], {
      staged: true,
      attachment: {
        trace: aTrace
      }
    });
  },

  





  _createView: function(aTrace) {
    let { type, name, location, blackBoxed, depth, frameId } = aTrace;
    let { parameterNames, returnVal, arguments: args } = aTrace;
    let fragment = document.createDocumentFragment();

    this._templateItem.classList.toggle("black-boxed", blackBoxed);
    this._templateItem.setAttribute("tooltiptext", SourceUtils.trimUrl(location.url));
    this._templateItem.style.MozPaddingStart = depth + "em";

    const TYPES = ["call", "yield", "return", "throw"];
    for (let t of TYPES) {
      this._templateTypeIcon.classList.toggle("trace-" + t, t == type);
    }
    this._templateTypeIcon.setAttribute("value", {
      call: "\u2192",
      yield: "Y",
      return: "\u2190",
      throw: "E",
      terminated: "TERMINATED"
    }[type]);

    this._templateNameNode.setAttribute("value", name);

    
    const addedNodes = [];

    if (parameterNames) {
      const syntax = (p) => {
        const el = document.createElement("label");
        el.setAttribute("value", p);
        el.classList.add("trace-syntax");
        el.classList.add("plain");
        addedNodes.push(el);
        return el;
      };

      this._templateItem.appendChild(syntax("("));

      for (let i = 0, n = parameterNames.length; i < n; i++) {
        let param = document.createElement("label");
        param.setAttribute("value", parameterNames[i]);
        param.classList.add("trace-param");
        param.classList.add("plain");
        addedNodes.push(param);
        this._templateItem.appendChild(param);

        if (i + 1 !== n) {
          this._templateItem.appendChild(syntax(", "));
        }
      }

      this._templateItem.appendChild(syntax(")"));
    }

    
    for (let node of this._template.childNodes) {
      fragment.appendChild(node.cloneNode(true));
    }

    
    for (let node of addedNodes) {
      this._templateItem.removeChild(node);
    }

    return fragment;
  }
});




let SourceUtils = {
  _labelsCache: new Map(), 
  _groupsCache: new Map(),
  _minifiedCache: new WeakMap(),

  






  isJavaScript: function(aUrl, aContentType = "") {
    return (aUrl && /\.jsm?$/.test(this.trimUrlQuery(aUrl))) ||
           aContentType.contains("javascript");
  },

  






  isMinified: Task.async(function*(sourceClient) {
    if (this._minifiedCache.has(sourceClient)) {
      return this._minifiedCache.get(sourceClient);
    }

    let [, text] = yield DebuggerController.SourceScripts.getText(sourceClient);
    let isMinified;
    let lineEndIndex = 0;
    let lineStartIndex = 0;
    let lines = 0;
    let indentCount = 0;
    let overCharLimit = false;

    
    text = text.replace(/\/\*[\S\s]*?\*\/|\/\/(.+|\n)/g, "");

    while (lines++ < SAMPLE_SIZE) {
      lineEndIndex = text.indexOf("\n", lineStartIndex);
      if (lineEndIndex == -1) {
         break;
      }
      if (/^\s+/.test(text.slice(lineStartIndex, lineEndIndex))) {
        indentCount++;
      }
      
      if ((lineEndIndex - lineStartIndex) > CHARACTER_LIMIT) {
        overCharLimit = true;
        break;
      }
      lineStartIndex = lineEndIndex + 1;
    }

    isMinified =
      ((indentCount / lines) * 100) < INDENT_COUNT_THRESHOLD || overCharLimit;

    this._minifiedCache.set(sourceClient, isMinified);
    return isMinified;
  }),

  




  clearCache: function() {
    this._labelsCache.clear();
    this._groupsCache.clear();
    this._minifiedCache.clear();
  },

  







  getSourceLabel: function(aUrl) {
    let cachedLabel = this._labelsCache.get(aUrl);
    if (cachedLabel) {
      return cachedLabel;
    }

    let sourceLabel = null;

    for (let name of Object.keys(KNOWN_SOURCE_GROUPS)) {
      if (aUrl.startsWith(KNOWN_SOURCE_GROUPS[name])) {
        sourceLabel = aUrl.substring(KNOWN_SOURCE_GROUPS[name].length);
      }
    }

    if (!sourceLabel) {
      sourceLabel = this.trimUrl(aUrl);
    }

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

    let groupLabel = uri.prePath;

    for (let name of Object.keys(KNOWN_SOURCE_GROUPS)) {
      if (aUrl.startsWith(KNOWN_SOURCE_GROUPS[name])) {
        groupLabel = name;
      }
    }

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
      
      
      if (!DebuggerView.Sources.getItemForAttachment(e => e.label == aLabel)) {
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




function VariableBubbleView() {
  dumpn("VariableBubbleView was instantiated");

  this._onMouseMove = this._onMouseMove.bind(this);
  this._onMouseOut = this._onMouseOut.bind(this);
  this._onPopupHiding = this._onPopupHiding.bind(this);
}

VariableBubbleView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the VariableBubbleView");

    this._editorContainer = document.getElementById("editor");
    this._editorContainer.addEventListener("mousemove", this._onMouseMove, false);
    this._editorContainer.addEventListener("mouseout", this._onMouseOut, false);

    this._tooltip = new Tooltip(document, {
      closeOnEvents: [{
        emitter: DebuggerController._toolbox,
        event: "select"
      }, {
        emitter: this._editorContainer,
        event: "scroll",
        useCapture: true
      }]
    });
    this._tooltip.defaultPosition = EDITOR_VARIABLE_POPUP_POSITION;
    this._tooltip.defaultShowDelay = EDITOR_VARIABLE_HOVER_DELAY;
    this._tooltip.panel.addEventListener("popuphiding", this._onPopupHiding);
  },

  


  destroy: function() {
    dumpn("Destroying the VariableBubbleView");

    this._tooltip.panel.removeEventListener("popuphiding", this._onPopupHiding);
    this._editorContainer.removeEventListener("mousemove", this._onMouseMove, false);
    this._editorContainer.removeEventListener("mouseout", this._onMouseOut, false);
  },

  



  _ignoreLiterals: true,

  






  _findIdentifier: function(x, y) {
    let editor = DebuggerView.editor;

    
    let hoveredPos = editor.getPositionFromCoords({ left: x, top: y });
    let hoveredOffset = editor.getOffset(hoveredPos);
    let hoveredLine = hoveredPos.line;
    let hoveredColumn = hoveredPos.ch;

    
    
    let contents = editor.getText();
    let location = DebuggerView.Sources.selectedValue;
    let parsedSource = DebuggerController.Parser.get(contents, location);
    let scriptInfo = parsedSource.getScriptInfo(hoveredOffset);

    
    if (scriptInfo.length == -1) {
      return;
    }

    
    
    let scriptStart = editor.getPosition(scriptInfo.start);
    let scriptLineOffset = scriptStart.line;
    let scriptColumnOffset = (hoveredLine == scriptStart.line ? scriptStart.ch : 0);

    let scriptLine = hoveredLine - scriptLineOffset;
    let scriptColumn = hoveredColumn - scriptColumnOffset;
    let identifierInfo = parsedSource.getIdentifierAt({
      line: scriptLine + 1,
      column: scriptColumn,
      scriptIndex: scriptInfo.index,
      ignoreLiterals: this._ignoreLiterals
    });

    
    if (!identifierInfo) {
      return;
    }

    
    
    let { start: identifierStart, end: identifierEnd } = identifierInfo.location;
    let identifierCoords = {
      line: identifierStart.line + scriptLineOffset,
      column: identifierStart.column + scriptColumnOffset,
      length: identifierEnd.column - identifierStart.column
    };

    
    
    DebuggerController.StackFrames.evaluate(identifierInfo.evalString)
      .then(frameFinished => {
        if ("return" in frameFinished) {
          this.showContents({
            coords: identifierCoords,
            evalPrefix: identifierInfo.evalString,
            objectActor: frameFinished.return
          });
        } else {
          let msg = "Evaluation has thrown for: " + identifierInfo.evalString;
          console.warn(msg);
          dumpn(msg);
        }
      })
      .then(null, err => {
        let msg = "Couldn't evaluate: " + err.message;
        console.error(msg);
        dumpn(msg);
      });
  },

  









  showContents: function({ coords, evalPrefix, objectActor }) {
    let editor = DebuggerView.editor;
    let { line, column, length } = coords;

    
    this._markedText = editor.markText(
      { line: line - 1, ch: column },
      { line: line - 1, ch: column + length });

    
    
    if (VariablesView.isPrimitive({ value: objectActor })) {
      let className = VariablesView.getClass(objectActor);
      let textContent = VariablesView.getString(objectActor);
      this._tooltip.setTextContent({
        messages: [textContent],
        messagesClass: className,
        containerClass: "plain"
      }, [{
        label: L10N.getStr('addWatchExpressionButton'),
        className: "dbg-expression-button",
        command: () => {
          DebuggerView.VariableBubble.hideContents();
          DebuggerView.WatchExpressions.addExpression(evalPrefix, true);
        }
      }]);
    } else {
      this._tooltip.setVariableContent(objectActor, {
        searchPlaceholder: L10N.getStr("emptyPropertiesFilterText"),
        searchEnabled: Prefs.variablesSearchboxVisible,
        eval: (variable, value) => {
          let string = variable.evaluationMacro(variable, value);
          DebuggerController.StackFrames.evaluate(string);
          DebuggerView.VariableBubble.hideContents();
        }
      }, {
        getEnvironmentClient: aObject => gThreadClient.environment(aObject),
        getObjectClient: aObject => gThreadClient.pauseGrip(aObject),
        simpleValueEvalMacro: this._getSimpleValueEvalMacro(evalPrefix),
        getterOrSetterEvalMacro: this._getGetterOrSetterEvalMacro(evalPrefix),
        overrideValueEvalMacro: this._getOverrideValueEvalMacro(evalPrefix)
      }, {
        fetched: (aEvent, aType) => {
          if (aType == "properties") {
            window.emit(EVENTS.FETCHED_BUBBLE_PROPERTIES);
          }
        }
      }, [{
        label: L10N.getStr("addWatchExpressionButton"),
        className: "dbg-expression-button",
        command: () => {
          DebuggerView.VariableBubble.hideContents();
          DebuggerView.WatchExpressions.addExpression(evalPrefix, true);
        }
      }], DebuggerController._toolbox);
    }

    this._tooltip.show(this._markedText.anchor);
  },

  


  hideContents: function() {
    clearNamedTimeout("editor-mouse-move");
    this._tooltip.hide();
  },

  





  contentsShown: function() {
    return this._tooltip.isShown();
  },

  





  _getSimpleValueEvalMacro: function(aPrefix) {
    return (item, string) =>
      VariablesView.simpleValueEvalMacro(item, string, aPrefix);
  },
  _getGetterOrSetterEvalMacro: function(aPrefix) {
    return (item, string) =>
      VariablesView.getterOrSetterEvalMacro(item, string, aPrefix);
  },
  _getOverrideValueEvalMacro: function(aPrefix) {
    return (item, string) =>
      VariablesView.overrideValueEvalMacro(item, string, aPrefix);
  },

  


  _onMouseMove: function(e) {
    
    
    
    let isResumed = gThreadClient && gThreadClient.state != "paused";
    let isSelecting = DebuggerView.editor.somethingSelected() && e.buttons > 0;
    let isPopupVisible = !this._tooltip.isHidden();
    if (isResumed || isSelecting || isPopupVisible) {
      clearNamedTimeout("editor-mouse-move");
      return;
    }
    
    
    setNamedTimeout("editor-mouse-move",
      EDITOR_VARIABLE_HOVER_DELAY, () => this._findIdentifier(e.clientX, e.clientY));
  },

  


  _onMouseOut: function() {
    clearNamedTimeout("editor-mouse-move");
  },

  


  _onPopupHiding: function({ target }) {
    if (this._tooltip.panel != target) {
      return;
    }
    if (this._markedText) {
      this._markedText.clear();
      this._markedText = null;
    }
    if (!this._tooltip.isEmpty()) {
      this._tooltip.empty();
    }
  },

  _editorContainer: null,
  _markedText: null,
  _tooltip: null
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

    this.widget = new SimpleListWidget(document.getElementById("expressions"));
    this.widget.setAttribute("context", "debuggerWatchExpressionsContextMenu");
    this.widget.addEventListener("click", this._onClick, false);

    this.headerText = L10N.getStr("addWatchExpressionText");
    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the WatchExpressionsView");

    this.widget.removeEventListener("click", this._onClick, false);
  },

  


  _addCommands: function() {
    utils.addCommands(document.getElementById('debuggerCommands'), {
      addWatchExpressionCommand: () => this._onCmdAddExpression(),
      removeAllWatchExpressionsCommand: () => this._onCmdRemoveAllExpressions()
    });
  },

  








  addExpression: function(aExpression = "", aSkipUserInput = false) {
    
    DebuggerView.showInstrumentsPane();

    
    let itemView = this._createItemView(aExpression);

    
    let expressionItem = this.push([itemView.container], {
      index: 0, 
      attachment: {
        view: itemView,
        initialExpression: aExpression,
        currentExpression: "",
      }
    });

    
    
    if (!aSkipUserInput) {
      expressionItem.attachment.view.inputNode.select();
      expressionItem.attachment.view.inputNode.focus();
      DebuggerView.Variables.parentNode.scrollTop = 0;
    }
    
    else {
      this.toggleContents(false);
      this._onBlur({ target: expressionItem.attachment.view.inputNode });
    }
  },

  









  switchExpression: function(aVar, aExpression) {
    let expressionItem =
      [i for (i of this) if (i.attachment.currentExpression == aVar.name)][0];

    
    if (!aExpression || this.getAllStrings().indexOf(aExpression) != -1) {
      this.deleteExpression(aVar);
      return;
    }

    
    expressionItem.attachment.currentExpression = aExpression;
    expressionItem.attachment.view.inputNode.value = aExpression;

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  







  deleteExpression: function(aVar) {
    let expressionItem =
      [i for (i of this) if (i.attachment.currentExpression == aVar.name)][0];

    
    this.remove(expressionItem);

    
    DebuggerController.StackFrames.syncWatchExpressions();
  },

  







  getString: function(aIndex) {
    return this.getItemAtIndex(aIndex).attachment.currentExpression;
  },

  





  getAllStrings: function() {
    return this.items.map(e => e.attachment.currentExpression);
  },

  





  _createItemView: function(aExpression) {
    let container = document.createElement("hbox");
    container.className = "list-widget-item dbg-expression";
    container.setAttribute("align", "center");

    let arrowNode = document.createElement("hbox");
    arrowNode.className = "dbg-expression-arrow";

    let inputNode = document.createElement("textbox");
    inputNode.className = "plain dbg-expression-input devtools-monospace";
    inputNode.setAttribute("value", aExpression);
    inputNode.setAttribute("flex", "1");

    let closeNode = document.createElement("toolbarbutton");
    closeNode.className = "plain variables-view-delete";

    closeNode.addEventListener("click", this._onClose, false);
    inputNode.addEventListener("blur", this._onBlur, false);
    inputNode.addEventListener("keypress", this._onKeyPress, false);

    container.appendChild(arrowNode);
    container.appendChild(inputNode);
    container.appendChild(closeNode);

    return {
      container: container,
      arrowNode: arrowNode,
      inputNode: inputNode,
      closeNode: closeNode
    };
  },

  


  _onCmdAddExpression: function(aText) {
    
    if (this.getAllStrings().indexOf("") == -1) {
      this.addExpression(aText || DebuggerView.editor.getSelection());
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
    switch (e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ESCAPE:
        e.stopPropagation();
        DebuggerView.editor.focus();
    }
  }
});




function EventListenersView() {
  dumpn("EventListenersView was instantiated");

  this._onCheck = this._onCheck.bind(this);
  this._onClick = this._onClick.bind(this);
}

EventListenersView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the EventListenersView");

    this.widget = new SideMenuWidget(document.getElementById("event-listeners"), {
      showItemCheckboxes: true,
      showGroupCheckboxes: true
    });

    this.emptyText = L10N.getStr("noEventListenersText");
    this._eventCheckboxTooltip = L10N.getStr("eventCheckboxTooltip");
    this._onSelectorString = " " + L10N.getStr("eventOnSelector") + " ";
    this._inSourceString = " " + L10N.getStr("eventInSource") + " ";
    this._inNativeCodeString = L10N.getStr("eventNative");

    this.widget.addEventListener("check", this._onCheck, false);
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the EventListenersView");

    this.widget.removeEventListener("check", this._onCheck, false);
    this.widget.removeEventListener("click", this._onClick, false);
  },

  








  addListener: function(aListener, aOptions = {}) {
    let { node: { selector }, function: { url }, type } = aListener;
    if (!type) return;

    
    
    if (!url) {
      url = this._inNativeCodeString;
    }

    
    
    let eventItem = this.getItemForPredicate(aItem =>
      aItem.attachment.url == url &&
      aItem.attachment.type == type);

    if (eventItem) {
      let { selectors, view: { targets } } = eventItem.attachment;
      if (selectors.indexOf(selector) == -1) {
        selectors.push(selector);
        targets.setAttribute("value", L10N.getFormatStr("eventNodes", selectors.length));
      }
      return;
    }

    
    
    let is = (...args) => args.indexOf(type) != -1;
    let has = str => type.contains(str);
    let starts = str => type.startsWith(str);
    let group;

    if (starts("animation")) {
      group = L10N.getStr("animationEvents");
    } else if (starts("audio")) {
      group = L10N.getStr("audioEvents");
    } else if (is("levelchange")) {
      group = L10N.getStr("batteryEvents");
    } else if (is("cut", "copy", "paste")) {
      group = L10N.getStr("clipboardEvents");
    } else if (starts("composition")) {
      group = L10N.getStr("compositionEvents");
    } else if (starts("device")) {
      group = L10N.getStr("deviceEvents");
    } else if (is("fullscreenchange", "fullscreenerror", "orientationchange",
      "overflow", "resize", "scroll", "underflow", "zoom")) {
      group = L10N.getStr("displayEvents");
    } else if (starts("drag") || starts("drop")) {
      group = L10N.getStr("dragAndDropEvents");
    } else if (starts("gamepad")) {
      group = L10N.getStr("gamepadEvents");
    } else if (is("canplay", "canplaythrough", "durationchange", "emptied",
      "ended", "loadeddata", "loadedmetadata", "pause", "play", "playing",
      "ratechange", "seeked", "seeking", "stalled", "suspend", "timeupdate",
      "volumechange", "waiting")) {
      group = L10N.getStr("mediaEvents");
    } else if (is("blocked", "complete", "success", "upgradeneeded", "versionchange")) {
      group = L10N.getStr("indexedDBEvents");
    } else if (is("blur", "change", "focus", "focusin", "focusout", "invalid",
      "reset", "select", "submit")) {
      group = L10N.getStr("interactionEvents");
    } else if (starts("key") || is("input")) {
      group = L10N.getStr("keyboardEvents");
    } else if (starts("mouse") || has("click") || is("contextmenu", "show", "wheel")) {
      group = L10N.getStr("mouseEvents");
    } else if (starts("DOM")) {
      group = L10N.getStr("mutationEvents");
    } else if (is("abort", "error", "hashchange", "load", "loadend", "loadstart",
      "pagehide", "pageshow", "progress", "timeout", "unload", "uploadprogress",
      "visibilitychange")) {
      group = L10N.getStr("navigationEvents");
    } else if (is("pointerlockchange", "pointerlockerror")) {
      group = L10N.getStr("pointerLockEvents");
    } else if (is("compassneedscalibration", "userproximity")) {
      group = L10N.getStr("sensorEvents");
    } else if (starts("storage")) {
      group = L10N.getStr("storageEvents");
    } else if (is("beginEvent", "endEvent", "repeatEvent")) {
      group = L10N.getStr("timeEvents");
    } else if (starts("touch")) {
      group = L10N.getStr("touchEvents");
    } else {
      group = L10N.getStr("otherEvents");
    }

    
    let itemView = this._createItemView(type, selector, url);

    
    
    let checkboxState =
      DebuggerController.Breakpoints.DOM.activeEventNames.indexOf(type) != -1;

    
    this.push([itemView.container], {
      staged: aOptions.staged, 
      attachment: {
        url: url,
        type: type,
        view: itemView,
        selectors: [selector],
        group: group,
        checkboxState: checkboxState,
        checkboxTooltip: this._eventCheckboxTooltip
      }
    });
  },

  





  getAllEvents: function() {
    return this.attachments.map(e => e.type);
  },

  





  getCheckedEvents: function() {
    return this.attachments.filter(e => e.checkboxState).map(e => e.type);
  },

  











  _createItemView: function(aType, aSelector, aUrl) {
    let container = document.createElement("hbox");
    container.className = "dbg-event-listener";

    let eventType = document.createElement("label");
    eventType.className = "plain dbg-event-listener-type";
    eventType.setAttribute("value", aType);
    container.appendChild(eventType);

    let typeSeparator = document.createElement("label");
    typeSeparator.className = "plain dbg-event-listener-separator";
    typeSeparator.setAttribute("value", this._onSelectorString);
    container.appendChild(typeSeparator);

    let eventTargets = document.createElement("label");
    eventTargets.className = "plain dbg-event-listener-targets";
    eventTargets.setAttribute("value", aSelector);
    container.appendChild(eventTargets);

    let selectorSeparator = document.createElement("label");
    selectorSeparator.className = "plain dbg-event-listener-separator";
    selectorSeparator.setAttribute("value", this._inSourceString);
    container.appendChild(selectorSeparator);

    let eventLocation = document.createElement("label");
    eventLocation.className = "plain dbg-event-listener-location";
    eventLocation.setAttribute("value", SourceUtils.getSourceLabel(aUrl));
    eventLocation.setAttribute("flex", "1");
    eventLocation.setAttribute("crop", "center");
    container.appendChild(eventLocation);

    return {
      container: container,
      type: eventType,
      targets: eventTargets,
      location: eventLocation
    };
  },

  


  _onCheck: function({ detail: { description, checked }, target }) {
    if (description == "item") {
      this.getItemForElement(target).attachment.checkboxState = checked;
      DebuggerController.Breakpoints.DOM.scheduleEventBreakpointsUpdate();
      return;
    }

    
    this.items
      .filter(e => e.attachment.group == description)
      .forEach(e => this.callMethod("checkItem", e.target, checked));
  },

  


  _onClick: function({ target }) {
    
    
    
    let eventItem = this.getItemForElement(target, { noSiblings: true });
    if (eventItem) {
      let newState = eventItem.attachment.checkboxState ^= 1;
      this.callMethod("checkItem", eventItem.target, newState);
    }
  },

  _eventCheckboxTooltip: "",
  _onSelectorString: "",
  _inSourceString: "",
  _inNativeCodeString: ""
});




function GlobalSearchView() {
  dumpn("GlobalSearchView was instantiated");

  this._onHeaderClick = this._onHeaderClick.bind(this);
  this._onLineClick = this._onLineClick.bind(this);
  this._onMatchClick = this._onMatchClick.bind(this);
}

GlobalSearchView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the GlobalSearchView");

    this.widget = new SimpleListWidget(document.getElementById("globalsearch"));
    this._splitter = document.querySelector("#globalsearch + .devtools-horizontal-splitter");

    this.emptyText = L10N.getStr("noMatchingStringsText");
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
      
      let actors = DebuggerView.Sources.values;
      let sourcesFetched = DebuggerController.SourceScripts.getTextForSources(actors);
      sourcesFetched.then(aSources => this._doSearch(aToken, aSources));
    });
  },

  








  _doSearch: function(aToken, aSources) {
    
    if (!aToken) {
      this.clearView();
      return;
    }

    
    let lowerCaseToken = aToken.toLowerCase();
    let tokenLength = aToken.length;

    
    let globalResults = new GlobalResults();

    
    for (let [actor, text] of aSources) {
      let item = DebuggerView.Sources.getItemByValue(actor);
      let url = item.attachment.source.url;
      if (!url) {
        continue;
      }

      
      if (!text.toLowerCase().contains(lowerCaseToken)) {
        continue;
      }
      
      let sourceResults = new SourceResults(actor, globalResults);

      
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
      window.emit(EVENTS.GLOBAL_SEARCH_MATCH_FOUND);
    } else {
      window.emit(EVENTS.GLOBAL_SEARCH_MATCH_NOT_FOUND);
    }
  },

  





  _createGlobalResultsUI: function(aGlobalResults) {
    let i = 0;

    for (let sourceResults of aGlobalResults) {
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
    
    let container = document.createElement("hbox");
    aSourceResults.createView(container, {
      onHeaderClick: this._onHeaderClick,
      onLineClick: this._onLineClick,
      onMatchClick: this._onMatchClick
    });

    
    let item = this.push([container], {
      index: -1, 
      attachment: {
        sourceResults: aSourceResults
      }
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

    let actor = sourceResultsItem.instance.actor;
    let line = lineResultsItem.instance.line;

    DebuggerView.setEditorLocation(actor, line + 1, { noDebug: true });

    let range = lineResultsItem.lineData.range;
    let cursor = DebuggerView.editor.getOffset({ line: line, ch: 0 });
    let [ anchor, head ] = DebuggerView.editor.getPosition(
      cursor + range.start,
      cursor + range.start + range.length
    );

    DebuggerView.editor.setSelection(anchor, head);
  },

  





  _scrollMatchIntoViewIfNeeded: function(aMatch) {
    this.widget.ensureElementIsVisible(aMatch);
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










function SourceResults(aActor, aGlobalResults) {
  let item = DebuggerView.Sources.getItemByValue(aActor);
  this.actor = aActor;
  this.label = item.attachment.source.url;
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
    locationNode.setAttribute("value", this.label);

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

    aElementNode.id = "source-results-" + this.actor;
    aElementNode.className = "dbg-source-results";
    aElementNode.appendChild(resultsBox);

    SourceResults._itemsByElement.set(aElementNode, { instance: this });
  },

  actor: "",
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
    lineContentsNode.className = "dbg-results-line-contents";
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
        lineContentsNode.appendChild(this._ellipsis.cloneNode(true));
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




GlobalResults.prototype[Symbol.iterator] =
SourceResults.prototype[Symbol.iterator] =
LineResults.prototype[Symbol.iterator] = function*() {
  yield* this._store;
};









SourceResults.getItemForElement =
LineResults.getItemForElement = function(aElement) {
  return WidgetMethods.getItemForElement.call(this, aElement, { noSiblings: true });
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
DebuggerView.VariableBubble = new VariableBubbleView();
DebuggerView.Tracer = new TracerView();
DebuggerView.WatchExpressions = new WatchExpressionsView();
DebuggerView.EventListeners = new EventListenersView();
DebuggerView.GlobalSearch = new GlobalSearchView();
