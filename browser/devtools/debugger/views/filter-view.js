


"use strict";





function FilterView(DebuggerController, DebuggerView) {
  dumpn("FilterView was instantiated");

  this.Parser = DebuggerController.Parser;

  this.DebuggerView = DebuggerView;
  this.FilteredSources = new FilteredSourcesView(DebuggerView);
  this.FilteredFunctions = new FilteredFunctionsView(DebuggerController.SourceScripts,
                                                     DebuggerController.Parser,
                                                     DebuggerView);

  this._onClick = this._onClick.bind(this);
  this._onInput = this._onInput.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
  this._onBlur = this._onBlur.bind(this);
}

FilterView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the FilterView");

    this._searchbox = document.getElementById("searchbox");
    this._searchboxHelpPanel = document.getElementById("searchbox-help-panel");
    this._filterLabel = document.getElementById("filter-label");
    this._globalOperatorButton = document.getElementById("global-operator-button");
    this._globalOperatorLabel = document.getElementById("global-operator-label");
    this._functionOperatorButton = document.getElementById("function-operator-button");
    this._functionOperatorLabel = document.getElementById("function-operator-label");
    this._tokenOperatorButton = document.getElementById("token-operator-button");
    this._tokenOperatorLabel = document.getElementById("token-operator-label");
    this._lineOperatorButton = document.getElementById("line-operator-button");
    this._lineOperatorLabel = document.getElementById("line-operator-label");
    this._variableOperatorButton = document.getElementById("variable-operator-button");
    this._variableOperatorLabel = document.getElementById("variable-operator-label");

    this._fileSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("fileSearchKey"));
    this._globalSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("globalSearchKey"));
    this._filteredFunctionsKey = ShortcutUtils.prettifyShortcut(document.getElementById("functionSearchKey"));
    this._tokenSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("tokenSearchKey"));
    this._lineSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("lineSearchKey"));
    this._variableSearchKey = ShortcutUtils.prettifyShortcut(document.getElementById("variableSearchKey"));

    this._searchbox.addEventListener("click", this._onClick, false);
    this._searchbox.addEventListener("select", this._onInput, false);
    this._searchbox.addEventListener("input", this._onInput, false);
    this._searchbox.addEventListener("keypress", this._onKeyPress, false);
    this._searchbox.addEventListener("blur", this._onBlur, false);

    let placeholder = L10N.getFormatStr("emptySearchText", this._fileSearchKey);
    this._searchbox.setAttribute("placeholder", placeholder);

    this._globalOperatorButton.setAttribute("label", SEARCH_GLOBAL_FLAG);
    this._functionOperatorButton.setAttribute("label", SEARCH_FUNCTION_FLAG);
    this._tokenOperatorButton.setAttribute("label", SEARCH_TOKEN_FLAG);
    this._lineOperatorButton.setAttribute("label", SEARCH_LINE_FLAG);
    this._variableOperatorButton.setAttribute("label", SEARCH_VARIABLE_FLAG);

    this._filterLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelFilter", this._fileSearchKey));
    this._globalOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGlobal", this._globalSearchKey));
    this._functionOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelFunction", this._filteredFunctionsKey));
    this._tokenOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelToken", this._tokenSearchKey));
    this._lineOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelGoToLine", this._lineSearchKey));
    this._variableOperatorLabel.setAttribute("value",
      L10N.getFormatStr("searchPanelVariable", this._variableSearchKey));

    this.FilteredSources.initialize();
    this.FilteredFunctions.initialize();

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the FilterView");

    this._searchbox.removeEventListener("click", this._onClick, false);
    this._searchbox.removeEventListener("select", this._onInput, false);
    this._searchbox.removeEventListener("input", this._onInput, false);
    this._searchbox.removeEventListener("keypress", this._onKeyPress, false);
    this._searchbox.removeEventListener("blur", this._onBlur, false);

    this.FilteredSources.destroy();
    this.FilteredFunctions.destroy();
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      fileSearchCommand: () => this._doFileSearch(),
      globalSearchCommand: () => this._doGlobalSearch(),
      functionSearchCommand: () => this._doFunctionSearch(),
      tokenSearchCommand: () => this._doTokenSearch(),
      lineSearchCommand: () => this._doLineSearch(),
      variableSearchCommand: () => this._doVariableSearch(),
      variablesFocusCommand: () => this._doVariablesFocus()
    });
  },

  



  get searchData() {
    let operator = "", args = [];

    let rawValue = this._searchbox.value;
    let rawLength = rawValue.length;
    let globalFlagIndex = rawValue.indexOf(SEARCH_GLOBAL_FLAG);
    let functionFlagIndex = rawValue.indexOf(SEARCH_FUNCTION_FLAG);
    let variableFlagIndex = rawValue.indexOf(SEARCH_VARIABLE_FLAG);
    let tokenFlagIndex = rawValue.lastIndexOf(SEARCH_TOKEN_FLAG);
    let lineFlagIndex = rawValue.lastIndexOf(SEARCH_LINE_FLAG);

    
    if (globalFlagIndex != 0 && functionFlagIndex != 0 && variableFlagIndex != 0) {
      
      if (tokenFlagIndex != -1) {
        operator = SEARCH_TOKEN_FLAG;
        args.push(rawValue.slice(0, tokenFlagIndex)); 
        args.push(rawValue.substr(tokenFlagIndex + 1, rawLength)); 
      } else if (lineFlagIndex != -1) {
        operator = SEARCH_LINE_FLAG;
        args.push(rawValue.slice(0, lineFlagIndex)); 
        args.push(+rawValue.substr(lineFlagIndex + 1, rawLength) || 0); 
      } else {
        args.push(rawValue);
      }
    }
    
    else if (globalFlagIndex == 0) {
      operator = SEARCH_GLOBAL_FLAG;
      args.push(rawValue.slice(1));
    }
    
    else if (functionFlagIndex == 0) {
      operator = SEARCH_FUNCTION_FLAG;
      args.push(rawValue.slice(1));
    }
    
    else if (variableFlagIndex == 0) {
      operator = SEARCH_VARIABLE_FLAG;
      args.push(rawValue.slice(1));
    }

    return [operator, args];
  },

  



  get searchOperator() this.searchData[0],

  



  get searchArguments() this.searchData[1],

  


  clearSearch: function() {
    this._searchbox.value = "";
    this.clearViews();

    this.FilteredSources.clearView();
    this.FilteredFunctions.clearView();
  },

  


  clearViews: function() {
    this.DebuggerView.GlobalSearch.clearView();
    this.FilteredSources.clearView();
    this.FilteredFunctions.clearView();
    this._searchboxHelpPanel.hidePopup();
  },

  






  _performLineSearch: function(aLine) {
    
    if (aLine) {
      this.DebuggerView.editor.setCursor({ line: aLine - 1, ch: 0 }, "center");
    }
  },

  






  _performTokenSearch: function(aToken) {
    
    if (!aToken) {
      return;
    }
    this.DebuggerView.editor.find(aToken);
  },

  


  _onClick: function() {
    
    
    if (!this._searchbox.value) {
      this._searchboxHelpPanel.openPopup(this._searchbox);
    }
  },

  


  _onInput: function() {
    this.clearViews();

    
    if (!this._searchbox.value) {
      return;
    }

    
    switch (this.searchOperator) {
      case SEARCH_GLOBAL_FLAG:
        
        this.DebuggerView.GlobalSearch.scheduleSearch(this.searchArguments[0]);
        break;
      case SEARCH_FUNCTION_FLAG:
      
        this.FilteredFunctions.scheduleSearch(this.searchArguments[0]);
        break;
      case SEARCH_VARIABLE_FLAG:
        
        this.DebuggerView.Variables.scheduleSearch(this.searchArguments[0]);
        break;
      case SEARCH_TOKEN_FLAG:
        
        this.FilteredSources.scheduleSearch(this.searchArguments[0]);
        this._performTokenSearch(this.searchArguments[1]);
        break;
      case SEARCH_LINE_FLAG:
        
        this.FilteredSources.scheduleSearch(this.searchArguments[0]);
        this._performLineSearch(this.searchArguments[1]);
        break;
      default:
        
        this.FilteredSources.scheduleSearch(this.searchArguments[0]);
        break;
    }
  },

  


  _onKeyPress: function(e) {
    
    e.char = String.fromCharCode(e.charCode);

    
    let [operator, args] = this.searchData;
    let isGlobalSearch = operator == SEARCH_GLOBAL_FLAG;
    let isFunctionSearch = operator == SEARCH_FUNCTION_FLAG;
    let isVariableSearch = operator == SEARCH_VARIABLE_FLAG;
    let isTokenSearch = operator == SEARCH_TOKEN_FLAG;
    let isLineSearch = operator == SEARCH_LINE_FLAG;
    let isFileOnlySearch = !operator && args.length == 1;

    
    let actionToPerform;

    
    if ((e.char == "g" && e.metaKey) || e.char == "n" && e.ctrlKey) {
      actionToPerform = "selectNext";
    }
    
    else if ((e.char == "G" && e.metaKey) || e.char == "p" && e.ctrlKey) {
      actionToPerform = "selectPrev";
    }
    
    
    else switch (e.keyCode) {
      case e.DOM_VK_RETURN:
        var isReturnKey = true;
        
        actionToPerform = e.shiftKey ? "selectPrev" : "selectNext";
        break;
      case e.DOM_VK_DOWN:
        actionToPerform = "selectNext";
        break;
      case e.DOM_VK_UP:
        actionToPerform = "selectPrev";
        break;
    }

    
    
    if (!actionToPerform || (!operator && !args.length)) {
      this.DebuggerView.editor.dropSelection();
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    
    
    if (isGlobalSearch) {
      let targetView = this.DebuggerView.GlobalSearch;
      if (!isReturnKey) {
        targetView[actionToPerform]();
      } else if (targetView.hidden) {
        targetView.scheduleSearch(args[0], 0);
      }
      return;
    }

    
    
    if (isFunctionSearch) {
      let targetView = this.FilteredFunctions;
      if (!isReturnKey) {
        targetView[actionToPerform]();
      } else if (targetView.hidden) {
        targetView.scheduleSearch(args[0], 0);
      } else {
        if (!targetView.selectedItem) {
          targetView.selectedIndex = 0;
        }
        this.clearSearch();
      }
      return;
    }

    
    if (isVariableSearch) {
      let targetView = this.DebuggerView.Variables;
      if (isReturnKey) {
        targetView.scheduleSearch(args[0], 0);
      }
      return;
    }

    
    
    if (isFileOnlySearch) {
      let targetView = this.FilteredSources;
      if (!isReturnKey) {
        targetView[actionToPerform]();
      } else if (targetView.hidden) {
        targetView.scheduleSearch(args[0], 0);
      } else {
        if (!targetView.selectedItem) {
          targetView.selectedIndex = 0;
        }
        this.clearSearch();
      }
      return;
    }

    
    if (isTokenSearch) {
      let methods = { selectNext: "findNext", selectPrev: "findPrev" };
      this.DebuggerView.editor[methods[actionToPerform]]();
      return;
    }

    
    if (isLineSearch) {
      let [, line] = args;
      let amounts = { selectNext: 1, selectPrev: -1 };

      
      line += !isReturnKey ? amounts[actionToPerform] : 0;
      let lineCount = this.DebuggerView.editor.lineCount();
      let lineTarget = line < 1 ? 1 : line > lineCount ? lineCount : line;
      this._doSearch(SEARCH_LINE_FLAG, lineTarget);
      return;
    }
  },

  


  _onBlur: function() {
    this.clearViews();
  },

  





  _doSearch: function(aOperator = "", aText = "") {
    this._searchbox.focus();
    this._searchbox.value = ""; 

    if (aText) {
      this._searchbox.value = aOperator + aText;
      return;
    }
    if (this.DebuggerView.editor.somethingSelected()) {
      this._searchbox.value = aOperator + this.DebuggerView.editor.getSelection();
      return;
    }
    if (SEARCH_AUTOFILL.indexOf(aOperator) != -1) {
      let cursor = this.DebuggerView.editor.getCursor();
      let content = this.DebuggerView.editor.getText();
      let location = this.DebuggerView.Sources.selectedItem.attachment.source.url;
      let source = this.Parser.get(content, location);
      let identifier = source.getIdentifierAt({ line: cursor.line+1, column: cursor.ch });

      if (identifier && identifier.name) {
        this._searchbox.value = aOperator + identifier.name;
        this._searchbox.select();
        this._searchbox.selectionStart += aOperator.length;
        return;
      }
    }
    this._searchbox.value = aOperator;
  },

  


  _doFileSearch: function() {
    this._doSearch();
    this._searchboxHelpPanel.openPopup(this._searchbox);
  },

  


  _doGlobalSearch: function() {
    this._doSearch(SEARCH_GLOBAL_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doFunctionSearch: function() {
    this._doSearch(SEARCH_FUNCTION_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doTokenSearch: function() {
    this._doSearch(SEARCH_TOKEN_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doLineSearch: function() {
    this._doSearch(SEARCH_LINE_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doVariableSearch: function() {
    this._doSearch(SEARCH_VARIABLE_FLAG);
    this._searchboxHelpPanel.hidePopup();
  },

  


  _doVariablesFocus: function() {
    this.DebuggerView.showInstrumentsPane();
    this.DebuggerView.Variables.focusFirstVisibleItem();
  },

  _searchbox: null,
  _searchboxHelpPanel: null,
  _globalOperatorButton: null,
  _globalOperatorLabel: null,
  _functionOperatorButton: null,
  _functionOperatorLabel: null,
  _tokenOperatorButton: null,
  _tokenOperatorLabel: null,
  _lineOperatorButton: null,
  _lineOperatorLabel: null,
  _variableOperatorButton: null,
  _variableOperatorLabel: null,
  _fileSearchKey: "",
  _globalSearchKey: "",
  _filteredFunctionsKey: "",
  _tokenSearchKey: "",
  _lineSearchKey: "",
  _variableSearchKey: "",
};




function FilteredSourcesView(DebuggerView) {
  dumpn("FilteredSourcesView was instantiated");

  this.DebuggerView = DebuggerView;

  this._onClick = this._onClick.bind(this);
  this._onSelect = this._onSelect.bind(this);
}

FilteredSourcesView.prototype = Heritage.extend(ResultsPanelContainer.prototype, {
  


  initialize: function() {
    dumpn("Initializing the FilteredSourcesView");

    this.anchor = document.getElementById("searchbox");
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the FilteredSourcesView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("click", this._onClick, false);
    this.anchor = null;
  },

  







  scheduleSearch: function(aToken, aWait) {
    
    let maxDelay = FILE_SEARCH_ACTION_MAX_DELAY;
    let delay = aWait === undefined ? maxDelay / aToken.length : aWait;

    
    setNamedTimeout("sources-search", delay, () => this._doSearch(aToken));
  },

  





  _doSearch: function(aToken, aStore = []) {
    
    
    
    if (!aToken) {
      return;
    }

    for (let item of this.DebuggerView.Sources.items) {
      let lowerCaseLabel = item.attachment.label.toLowerCase();
      let lowerCaseToken = aToken.toLowerCase();
      if (lowerCaseLabel.match(lowerCaseToken)) {
        aStore.push(item);
      }

      
      
      if (aStore.length >= RESULTS_PANEL_MAX_RESULTS) {
        this._syncView(aStore);
        return;
      }
    }

    
    
    this._syncView(aStore);
  },

  





  _syncView: function(aSearchResults) {
    
    
    if (!aSearchResults.length) {
      window.emit(EVENTS.FILE_SEARCH_MATCH_NOT_FOUND);
      return;
    }

    for (let item of aSearchResults) {
      let url = item.attachment.source.url;

      if (url) {
        
        let itemView = this._createItemView(
          SourceUtils.trimUrlLength(item.attachment.label),
          SourceUtils.trimUrlLength(url, 0, "start")
        );

        
        this.push([itemView], {
          index: -1, 
          attachment: {
            url: url
          }
        });
      }
    }

    
    
    if (this._autoSelectFirstItem || this.DebuggerView.Filtering.searchOperator) {
      this.selectedIndex = 0;
    }
    this.hidden = false;

    
    window.emit(EVENTS.FILE_SEARCH_MATCH_FOUND);
  },

  


  _onClick: function(e) {
    let locationItem = this.getItemForElement(e.target);
    if (locationItem) {
      this.selectedItem = locationItem;
      this.DebuggerView.Filtering.clearSearch();
    }
  },

  





  _onSelect: function({ detail: locationItem }) {
    if (locationItem) {
      let actor = this.DebuggerView.Sources.getActorForLocation({ url: locationItem.attachment.url });
      this.DebuggerView.setEditorLocation(actor, undefined, {
        noCaret: true,
        noDebug: true
      });
    }
  }
});




function FilteredFunctionsView(SourceScripts, Parser, DebuggerView) {
  dumpn("FilteredFunctionsView was instantiated");

  this.SourceScripts = SourceScripts;
  this.Parser = Parser;
  this.DebuggerView = DebuggerView;

  this._onClick = this._onClick.bind(this);
  this._onSelect = this._onSelect.bind(this);
}

FilteredFunctionsView.prototype = Heritage.extend(ResultsPanelContainer.prototype, {
  


  initialize: function() {
    dumpn("Initializing the FilteredFunctionsView");

    this.anchor = document.getElementById("searchbox");
    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the FilteredFunctionsView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("click", this._onClick, false);
    this.anchor = null;
  },

  







  scheduleSearch: function(aToken, aWait) {
    
    let maxDelay = FUNCTION_SEARCH_ACTION_MAX_DELAY;
    let delay = aWait === undefined ? maxDelay / aToken.length : aWait;

    
    setNamedTimeout("function-search", delay, () => {
      
      let actors = this.DebuggerView.Sources.values;
      let sourcesFetched = this.SourceScripts.getTextForSources(actors);
      sourcesFetched.then(aSources => this._doSearch(aToken, aSources));
    });
  },

  








  _doSearch: function(aToken, aSources, aStore = []) {
    
    

    
    
    let currentActor = this.DebuggerView.Sources.selectedValue;
    let currentSource = aSources.filter(([actor]) => actor == currentActor)[0];
    aSources.splice(aSources.indexOf(currentSource), 1);
    aSources.unshift(currentSource);

    
    
    if (!aToken) {
      aSources.splice(1);
    }

    for (let [actor, contents] of aSources) {
      let item = this.DebuggerView.Sources.getItemByValue(actor);
      let url = item.attachment.source.url;
      if (!url) {
        continue;
      }

      let parsedSource = this.Parser.get(contents, url);
      let sourceResults = parsedSource.getNamedFunctionDefinitions(aToken);

      for (let scriptResult of sourceResults) {
        for (let parseResult of scriptResult) {
          aStore.push({
            sourceUrl: scriptResult.sourceUrl,
            scriptOffset: scriptResult.scriptOffset,
            functionName: parseResult.functionName,
            functionLocation: parseResult.functionLocation,
            inferredName: parseResult.inferredName,
            inferredChain: parseResult.inferredChain,
            inferredLocation: parseResult.inferredLocation
          });

          
          
          if (aStore.length >= RESULTS_PANEL_MAX_RESULTS) {
            this._syncView(aStore);
            return;
          }
        }
      }
    }

    
    
    this._syncView(aStore);
  },

  





  _syncView: function(aSearchResults) {
    
    
    if (!aSearchResults.length) {
      window.emit(EVENTS.FUNCTION_SEARCH_MATCH_NOT_FOUND);
      return;
    }

    for (let item of aSearchResults) {
      
      
      
      if (item.functionName && item.inferredName &&
          item.functionName != item.inferredName) {
        let s = " " + L10N.getStr("functionSearchSeparatorLabel") + " ";
        item.displayedName = item.inferredName + s + item.functionName;
      }
      
      else if (item.inferredName) {
        item.displayedName = item.inferredName;
      }
      
      else {
        item.displayedName = item.functionName;
      }

      
      
      if (item.inferredLocation) {
        item.actualLocation = item.inferredLocation;
      } else {
        item.actualLocation = item.functionLocation;
      }

      
      let itemView = this._createItemView(
        SourceUtils.trimUrlLength(item.displayedName + "()"),
        SourceUtils.trimUrlLength(item.sourceUrl, 0, "start"),
        (item.inferredChain || []).join(".")
      );

      
      this.push([itemView], {
        index: -1, 
        attachment: item
      });
    }

    
    
    if (this._autoSelectFirstItem) {
      this.selectedIndex = 0;
    }
    this.hidden = false;

    
    window.emit(EVENTS.FUNCTION_SEARCH_MATCH_FOUND);
  },

  


  _onClick: function(e) {
    let functionItem = this.getItemForElement(e.target);
    if (functionItem) {
      this.selectedItem = functionItem;
      this.DebuggerView.Filtering.clearSearch();
    }
  },

  


  _onSelect: function({ detail: functionItem }) {
    if (functionItem) {
      let sourceUrl = functionItem.attachment.sourceUrl;
      let actor = this.DebuggerView.Sources.getActorForLocation({ url: sourceUrl });
      let scriptOffset = functionItem.attachment.scriptOffset;
      let actualLocation = functionItem.attachment.actualLocation;

      this.DebuggerView.setEditorLocation(actor, actualLocation.start.line, {
        charOffset: scriptOffset,
        columnOffset: actualLocation.start.column,
        align: "center",
        noDebug: true
      });
    }
  },

  _searchTimeout: null,
  _searchFunction: null,
  _searchedToken: ""
});

DebuggerView.Filtering = new FilterView(DebuggerController, DebuggerView);
