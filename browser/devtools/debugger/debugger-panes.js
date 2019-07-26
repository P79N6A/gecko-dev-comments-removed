




"use strict";




function StackFramesView() {
  dumpn("StackFramesView was instantiated");
  MenuContainer.call(this);
  this._onClick = this._onClick.bind(this);
  this._onScroll = this._onScroll.bind(this);
}

create({ constructor: StackFramesView, proto: MenuContainer.prototype }, {
  


  initialize: function DVSF_initialize() {
    dumpn("Initializing the StackFramesView");
    this._container = new StackList(document.getElementById("stackframes"));
    this._container.emptyText = L10N.getStr("emptyStackText");

    this._container.addEventListener("click", this._onClick, false);
    this._container.addEventListener("scroll", this._onScroll, true);
    window.addEventListener("resize", this._onScroll, true);

    this._cache = new Map();
  },

  


  destroy: function DVSF_destroy() {
    dumpn("Destroying the StackFramesView");
    this._container.removeEventListener("click", this._onClick, true);
    this._container.removeEventListener("scroll", this._onScroll, true);
    window.removeEventListener("resize", this._onScroll, true);
  },

  












  addFrame:
  function DVSF_addFrame(aFrameName, aFrameDetails, aDepth, aOptions = {}) {
    
    DebuggerView.showPanesSoon();

    
    let stackframeItem = this.push(aFrameName, aFrameDetails, {
      forced: true,
      unsorted: true,
      relaxed: true,
      attachment: aOptions.attachment
    });

    
    if (!stackframeItem) {
      return;
    }

    let element = stackframeItem.target;
    element.id = "stackframe-" + aDepth;
    element.className = "dbg-stackframe list-item";
    element.labelNode.className = "dbg-stackframe-name plain";
    element.valueNode.className = "dbg-stackframe-details plain";

    this._cache.set(aDepth, stackframeItem);
  },

  





  highlightFrame: function DVSF_highlightFrame(aDepth) {
    this._container.selectedItem = this._cache.get(aDepth).target;
  },

  


  dirty: false,

  


  _onClick: function DVSF__onClick(e) {
    let item = this.getItemForElement(e.target);
    if (item) {
      
      DebuggerController.StackFrames.selectFrame(item.attachment.depth);
    }
  },

  


  _onScroll: function DVSF__onScroll() {
    
    if (this.dirty) {
      let list = this._container._list;

      
      
      if (list.scrollTop >= (list.scrollHeight - list.clientHeight) * 0.95) {
        DebuggerController.StackFrames.addMoreFrames();
        this.dirty = false;
      }
    }
  },

  _cache: null
});




let StackFrameUtils = {
  






  getFrameTitle: function SFU_getFrameTitle(aFrame) {
    if (aFrame.type == "call") {
      return aFrame.calleeName || "(anonymous)";
    }
    return "(" + aFrame.type + ")";
  }
};




function BreakpointsView() {
  dumpn("BreakpointsView was instantiated");
  MenuContainer.call(this);
  this._createItemView = this._createItemView.bind(this);
  this._onBreakpointRemoved = this._onBreakpointRemoved.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onCheckboxClick = this._onCheckboxClick.bind(this);
}

create({ constructor: BreakpointsView, proto: MenuContainer.prototype }, {
  


  initialize: function DVB_initialize() {
    dumpn("Initializing the BreakpointsView");
    this._container = new StackList(document.getElementById("breakpoints"));
    this._popupset = document.getElementById("debuggerPopupset");

    this._container.emptyText = L10N.getStr("emptyBreakpointsText");
    this._container.itemFactory = this._createItemView;
    this._container.uniquenessQualifier = 2;
    this._container.addEventListener("click", this._onClick, false);

    this._cache = new Map();
  },

  


  destroy: function DVB_destroy() {
    dumpn("Destroying the BreakpointsView");
    this._container.removeEventListener("click", this._onClick, false);
  },

  













  addBreakpoint:
  function DVB_addBreakpoint(aLineInfo, aLineText, aSourceLocation, aLineNumber, aId) {
    
    let breakpointItem = this.push(aLineInfo.trim(), aLineText.trim(), {
      forced: true,
      attachment: {
        enabled: true,
        sourceLocation: aSourceLocation,
        lineNumber: aLineNumber
      }
    });

    
    if (!breakpointItem) {
      this.enableBreakpoint(aSourceLocation, aLineNumber, { id: aId });
      return;
    }

    let element = breakpointItem.target;
    element.id = "breakpoint-" + aId;
    element.className = "dbg-breakpoint list-item";
    element.infoNode.className = "dbg-breakpoint-info plain";
    element.textNode.className = "dbg-breakpoint-text plain";
    element.setAttribute("contextmenu", this._createContextMenu(element));

    breakpointItem.finalize = this._onBreakpointRemoved;
    this._cache.set(this._key(aSourceLocation, aLineNumber), breakpointItem);
  },

  







  removeBreakpoint: function DVB_removeBreakpoint(aSourceLocation, aLineNumber) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (breakpointItem) {
      this.remove(breakpointItem);
      this._cache.delete(this._key(aSourceLocation, aLineNumber));
    }
  },

  
















  enableBreakpoint:
  function DVB_enableBreakpoint(aSourceLocation, aLineNumber, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (breakpointItem) {
      
      if (aOptions.id) {
        breakpointItem.target.id = "breakpoint-" + aOptions.id;
      }
      
      if (!aOptions.silent) {
        breakpointItem.target.checkbox.setAttribute("checked", "true");
      }

      let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
      let breakpointLocation = { url: url, line: line };
      DebuggerController.Breakpoints.addBreakpoint(breakpointLocation, aOptions.callback, {
        noPaneUpdate: true
      });

      
      breakpointItem.attachment.enabled = true;
      return true;
    }
    return false;
  },

  















  disableBreakpoint:
  function DVB_disableBreakpoint(aSourceLocation, aLineNumber, aOptions = {}) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (breakpointItem) {
      
      if (!aOptions.silent) {
        breakpointItem.target.checkbox.removeAttribute("checked");
      }

      let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
      let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);
      DebuggerController.Breakpoints.removeBreakpoint(breakpointClient, aOptions.callback, {
        noPaneUpdate: true
      });

      
      breakpointItem.attachment.enabled = false;
      return true;
    }
    return false;
  },

  







  highlightBreakpoint: function DVB_highlightBreakpoint(aSourceLocation, aLineNumber) {
    let breakpointItem = this.getBreakpoint(aSourceLocation, aLineNumber);
    if (breakpointItem) {
      this._container.selectedItem = breakpointItem.target;
    } else {
      this._container.selectedIndex = -1;
    }
  },

  


  unhighlightBreakpoint: function DVB_highlightBreakpoint() {
    this.highlightBreakpoint(null);
  },

  











  getBreakpoint: function DVB_getBreakpoint(aSourceLocation, aLineNumber) {
    return this._cache.get(this._key(aSourceLocation, aLineNumber));
  },

  











  _createItemView: function DVB__createItemView(aElementNode, aInfo, aText, aAttachment) {
    let checkbox = document.createElement("checkbox");
    checkbox.setAttribute("checked", "true");
    checkbox.addEventListener("click", this._onCheckboxClick, false);

    let lineInfo = document.createElement("label");
    lineInfo.setAttribute("value", aInfo);
    lineInfo.setAttribute("crop", "end");

    let lineText = document.createElement("label");
    lineText.setAttribute("value", aText);
    lineText.setAttribute("crop", "end");
    lineText.setAttribute("tooltiptext",
      aText.substr(0, BREAKPOINT_LINE_TOOLTIP_MAX_LENGTH));

    let state = document.createElement("vbox");
    state.className = "state";
    state.appendChild(checkbox);

    let content = document.createElement("vbox");
    content.className = "content";
    content.setAttribute("flex", "1");
    content.appendChild(lineInfo);
    content.appendChild(lineText);

    aElementNode.appendChild(state);
    aElementNode.appendChild(content);

    aElementNode.checkbox = checkbox;
    aElementNode.infoNode = lineInfo;
    aElementNode.textNode = lineText;
  },

  







  _createContextMenu: function DVB__createContextMenu(aElementNode) {
    let breakpointId = aElementNode.id;
    let commandsetId = "breakpointCommandset-" + breakpointId;
    let menupopupId = "breakpointMenupopup-" + breakpointId;

    let commandset = document.createElement("commandset");
    let menupopup = document.createElement("menupopup");
    commandset.setAttribute("id", commandsetId);
    menupopup.setAttribute("id", menupopupId);

    createMenuItem.call(this, "deleteAll");
    createMenuSeparator();
    createMenuItem.call(this, "enableAll");
    createMenuItem.call(this, "disableAll");
    createMenuSeparator();
    createMenuItem.call(this, "enableOthers");
    createMenuItem.call(this, "disableOthers");
    createMenuItem.call(this, "deleteOthers");
    createMenuSeparator();
    createMenuItem.call(this, "enableSelf", true);
    createMenuItem.call(this, "disableSelf");
    createMenuItem.call(this, "deleteSelf");

    this._popupset.appendChild(menupopup);
    document.documentElement.appendChild(commandset);

    aElementNode.commandset = commandset;
    aElementNode.menupopup = menupopup;
    return menupopupId;

    








    function createMenuItem(aName, aHiddenFlag) {
      let menuitem = document.createElement("menuitem");
      let command = document.createElement("command");

      let prefix = "bp-cMenu-";
      let commandId = prefix + aName + "-" + breakpointId + "-command";
      let menuitemId = prefix + aName + "-" + breakpointId + "-menuitem";

      let label = L10N.getStr("breakpointMenuItem." + aName);
      let func = this["_on" + aName.charAt(0).toUpperCase() + aName.slice(1)];

      command.setAttribute("id", commandId);
      command.setAttribute("label", label);
      command.addEventListener("command", func.bind(this, aElementNode), false);

      menuitem.setAttribute("id", menuitemId);
      menuitem.setAttribute("command", commandId);
      menuitem.setAttribute("hidden", aHiddenFlag);

      commandset.appendChild(command);
      menupopup.appendChild(menuitem);
      aElementNode[aName] = { menuitem: menuitem, command: command };
    }

    



    function createMenuSeparator() {
      let menuseparator = document.createElement("menuseparator");
      menupopup.appendChild(menuseparator);
    }
  },

  





  _destroyContextMenu: function DVB__destroyContextMenu(aElementNode) {
    let commandset = aElementNode.commandset;
    let menupopup = aElementNode.menupopup;

    commandset.parentNode.removeChild(commandset);
    menupopup.parentNode.removeChild(menupopup);
  },

  


  _onBreakpointRemoved: function DVB__onBreakpointRemoved(aItem) {
    this._destroyContextMenu(aItem.target);
  },

  


  _onClick: function DVB__onClick(e) {
    let breakpointItem = this.getItemForElement(e.target);
    if (!breakpointItem) {
      
      return;
    }
    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;

    DebuggerView.updateEditor(url, line, { noDebug: true });
    this.highlightBreakpoint(url, line);
  },

  


  _onCheckboxClick: function DVB__onCheckboxClick(e) {
    let breakpointItem = this.getItemForElement(e.target);
    if (!breakpointItem) {
      
      return;
    }
    let { sourceLocation: url, lineNumber: line, enabled } = breakpointItem.attachment;

    
    e.preventDefault();
    e.stopPropagation();

    this[enabled
      ? "disableBreakpoint"
      : "enableBreakpoint"](url, line, { silent: true });
  },

  





  _onEnableSelf: function DVB__onEnableSelf(aTarget) {
    if (!aTarget) {
      return;
    }
    let breakpointItem = this.getItemForElement(aTarget);
    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;

    if (this.enableBreakpoint(url, line)) {
      aTarget.enableSelf.menuitem.setAttribute("hidden", "true");
      aTarget.disableSelf.menuitem.removeAttribute("hidden");
    }
  },

  





  _onDisableSelf: function DVB__onDisableSelf(aTarget) {
    if (!aTarget) {
      return;
    }
    let breakpointItem = this.getItemForElement(aTarget);
    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;

    if (this.disableBreakpoint(url, line)) {
      aTarget.enableSelf.menuitem.removeAttribute("hidden");
      aTarget.disableSelf.menuitem.setAttribute("hidden", "true");
    }
  },

  





  _onDeleteSelf: function DVB__onDeleteSelf(aTarget) {
    if (!aTarget) {
      return;
    }
    let breakpointItem = this.getItemForElement(aTarget);
    let { sourceLocation: url, lineNumber: line } = breakpointItem.attachment;
    let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);

    this.removeBreakpoint(url, line);
    DebuggerController.Breakpoints.removeBreakpoint(breakpointClient);
  },

  





  _onEnableOthers: function DVB__onEnableOthers(aTarget) {
    for (let item in this) {
      if (item.target != aTarget) {
        this._onEnableSelf(item.target);
      }
    }
  },

  





  _onDisableOthers: function DVB__onDisableOthers(aTarget) {
    for (let item in this) {
      if (item.target != aTarget) {
        this._onDisableSelf(item.target);
      }
    }
  },

  





  _onDeleteOthers: function DVB__onDeleteOthers(aTarget) {
    for (let item in this) {
      if (item.target != aTarget) {
        this._onDeleteSelf(item.target);
      }
    }
  },

  





  _onEnableAll: function DVB__onEnableAll(aTarget) {
    this._onEnableOthers(aTarget);
    this._onEnableSelf(aTarget);
  },

  





  _onDisableAll: function DVB__onDisableAll(aTarget) {
    this._onDisableOthers(aTarget);
    this._onDisableSelf(aTarget);
  },

  





  _onDeleteAll: function DVB__onDeleteAll(aTarget) {
    this._onDeleteOthers(aTarget);
    this._onDeleteSelf(aTarget);
  },

  


  _key: function DVB__key(aSourceLocation, aLineNumber) {
    return aSourceLocation + aLineNumber;
  },

  _popupset: null,
  _cache: null
});




function GlobalSearchView() {
  dumpn("GlobalSearchView was instantiated");
  MenuContainer.call(this);
  this._startSearch = this._startSearch.bind(this);
  this._onFetchSourceFinished = this._onFetchSourceFinished.bind(this);
  this._onFetchSourcesFinished = this._onFetchSourcesFinished.bind(this);
  this._createItemView = this._createItemView.bind(this);
  this._onScroll = this._onScroll.bind(this);
  this._onHeaderClick = this._onHeaderClick.bind(this);
  this._onLineClick = this._onLineClick.bind(this);
  this._onMatchClick = this._onMatchClick.bind(this);
}

create({ constructor: GlobalSearchView, proto: MenuContainer.prototype }, {
  


  initialize: function DVGS_initialize() {
    dumpn("Initializing the GlobalSearchView");
    this._container = new StackList(document.getElementById("globalsearch"));
    this._splitter = document.getElementById("globalsearch-splitter");

    this._container.emptyText = L10N.getStr("noMatchingStringsText");
    this._container.itemFactory = this._createItemView;
    this._container.addEventListener("scroll", this._onScroll, false);

    this._cache = new Map();
  },

  


  destroy: function DVGS_destroy() {
    dumpn("Destroying the GlobalSearchView");
    this._container.removeEventListener("scroll", this._onScroll, false);
  },

  



  get hidden() this._container.getAttribute("hidden") == "true",

  



  set hidden(aFlag) {
    this._container.setAttribute("hidden", aFlag);
    this._splitter.setAttribute("hidden", aFlag);
  },

  


  clearView: function DVGS_clearView() {
    this.hidden = true;
    this.empty();
    window.dispatchEvent("Debugger:GlobalSearch:ViewCleared");
  },

  


  clearCache: function DVGS_clearCache() {
    this._cache = new Map();
    window.dispatchEvent("Debugger:GlobalSearch:CacheCleared");
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

  


  scheduleSearch: function DVGS_scheduleSearch() {
    window.clearTimeout(this._searchTimeout);
    this._searchTimeout = window.setTimeout(this._startSearch, GLOBAL_SEARCH_ACTION_DELAY);
  },

  


  _startSearch: function DVGS__startSearch() {
    let locations = DebuggerView.Sources.values;
    this._sourcesCount = locations.length;

    this._fetchSources(
      this._onFetchSourceFinished,
      this._onFetchSourcesFinished, locations);
  },

  









  _fetchSources: function DVGS__fetchSources(aFetchCallback, aFetchedCallback, aLocations) {
    
    if (this._cache.size == aLocations.length) {
      aFetchedCallback();
      return;
    }

    
    for (let location of aLocations) {
      if (this._cache.has(location)) {
        continue;
      }
      let sourceItem = DebuggerView.Sources.getItemByValue(location);
      DebuggerController.SourceScripts.getText(sourceItem.attachment, aFetchCallback);
    }
  },

  







  _onFetchSourceFinished: function DVGS__onFetchSourceFinished(aLocation, aContents) {
    
    this._cache.set(aLocation, aContents);

    
    if (this._cache.size == this._sourcesCount) {
      this._onFetchSourcesFinished();
    }
  },

  


  _onFetchSourcesFinished: function DVGS__onFetchSourcesFinished() {
    
    this._performGlobalSearch();
  },

  



  _performGlobalSearch: function DVGS__performGlobalSearch() {
    
    let token = DebuggerView.Filtering.searchedToken;

    
    if (!token) {
      this.clearView();
      window.dispatchEvent("Debugger:GlobalSearch:TokenEmpty");
      return;
    }

    
    let lowerCaseToken = token.toLowerCase();
    let tokenLength = token.length;

    
    let globalResults = new GlobalResults();

    for (let [location, contents] of this._cache) {
      
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
      window.dispatchEvent("Debugger:GlobalSearch:MatchFound");
    } else {
      window.dispatchEvent("Debugger:GlobalSearch:MatchNotFound");
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
    
    let sourceResultsItem = this.push(aLocation, aSourceResults.matchCount, {
      forced: true,
      unsorted: true,
      relaxed: true,
      attachment: {
        sourceResults: aSourceResults,
        expandFlag: aExpandFlag
      }
    });
  },

  











  _createItemView:
  function DVGS__createItemView(aElementNode, aLocation, aMatchCount, aAttachment) {
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

    sourceResultsItem.instance.expand(true);
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
    let { clientHeight } = this._container._parent;

    if (top - height <= clientHeight || this._forceExpandResults) {
      sourceResultsItem.instance.expand(true);
    }
  },

  





  _scrollMatchIntoViewIfNeeded:  function DVGS__scrollMatchIntoViewIfNeeded(aMatch) {
    let { top, height } = aMatch.getBoundingClientRect();
    let { clientHeight } = this._container._parent;

    let style = window.getComputedStyle(aMatch);
    let topBorderSize = window.parseInt(style.getPropertyValue("border-top-width"));
    let bottomBorderSize = window.parseInt(style.getPropertyValue("border-bottom-width"));

    let marginY = top - (height + topBorderSize + bottomBorderSize) * 2;
    if (marginY <= 0) {
      this._container._parent.scrollTop += marginY;
    }
    if (marginY + height > clientHeight) {
      this._container._parent.scrollTop += height - (clientHeight - marginY);
    }
  },

  





  _bounceMatch: function DVGS__bounceMatch(aMatch) {
    Services.tm.currentThread.dispatch({ run: function() {
      aMatch.setAttribute("focused", "");

      aMatch.addEventListener("transitionend", function onEvent() {
        aMatch.removeEventListener("transitionend", onEvent);
        aMatch.removeAttribute("focused");
      });
    }}, 0);
  },

  _splitter: null,
  _currentlyFocusedMatch: -1,
  _forceExpandResults: false,
  _searchTimeout: null,
  _sourcesCount: -1,
  _cache: null
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

  





  expand: function SR_expand(aSkipAnimationFlag) {
    this._target.resultsContainer.setAttribute("open", "");
    this._target.arrow.setAttribute("open", "");

    if (!aSkipAnimationFlag) {
      this._target.resultsContainer.setAttribute("animated", "");
    }
  },

  


  collapse: function SR_collapse() {
    this._target.resultsContainer.removeAttribute("animated");
    this._target.resultsContainer.removeAttribute("open");
    this._target.arrow.removeAttribute("open");
  },

  


  toggle: function SR_toggle(e) {
    if (e instanceof Event) {
      this._userToggled = true;
    }
    this.expanded ^= 1;
  },

  



  get expanded() this._target.resultsContainer.hasAttribute("open"),

  



  set expanded(aFlag) this[aFlag ? "expand" : "collapse"](),

  



  get toggled() this._userToggled,

  



  get target() this._target,

  















  createView:
  function SR_createView(aElementNode, aLocation, aMatchCount, aExpandFlag, aCallbacks) {
    this._target = aElementNode;

    let arrow = document.createElement("box");
    arrow.className = "arrow";

    let locationNode = document.createElement("label");
    locationNode.className = "plain location";
    locationNode.setAttribute("value", SourceUtils.trimUrlLength(aLocation));

    let matchCountNode = document.createElement("label");
    matchCountNode.className = "plain match-count";
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

    for (let [lineNumber, lineResults] of this._store) {
      lineResults.createView(resultsContainer, lineNumber, aCallbacks)
    }

    aElementNode.arrow = arrow;
    aElementNode.resultsHeader = resultsHeader;
    aElementNode.resultsContainer = resultsContainer;

    if (aExpandFlag && aMatchCount < GLOBAL_SEARCH_EXPAND_MAX_RESULTS) {
      this.expand(true);
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

    lineNumberNode.className = "plain line-number";
    lineNumberNode.setAttribute("value", aLineNumber + 1);
    lineContentsNode.className = "line-contents";
    lineContentsNode.setAttribute("flex", "1");

    for (let chunk of this._store) {
      let { string, range, match } = chunk;
      lineString = string.substr(0, GLOBAL_SEARCH_LINE_MAX_LENGTH - lineLength);
      lineLength += string.length;

      let label = document.createElement("label");
      label.className = "plain string";
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
    label.className = "plain string";
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
  for (let [_, item] of this._itemsByElement) {
    if (!item.nonenumerable) {
      count++;
    }
  }
  return count;
};




DebuggerView.StackFrames = new StackFramesView();
DebuggerView.Breakpoints = new BreakpointsView();
DebuggerView.GlobalSearch = new GlobalSearchView();
