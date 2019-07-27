


"use strict";




function GlobalSearchView(DebuggerController, DebuggerView) {
  dumpn("GlobalSearchView was instantiated");

  this.SourceScripts = DebuggerController.SourceScripts;
  this.DebuggerView = DebuggerView;

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
      
      let actors = this.DebuggerView.Sources.values;
      let sourcesFetched = this.SourceScripts.getTextForSources(actors);
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
      let item = this.DebuggerView.Sources.getItemByValue(actor);
      let url = item.attachment.source.url;
      if (!url) {
        continue;
      }

      
      if (!text.toLowerCase().includes(lowerCaseToken)) {
        continue;
      }
      
      let sourceResults = new SourceResults(actor,
                                            globalResults,
                                            this.DebuggerView.Sources);

      
      text.split("\n").forEach((aString, aLine) => {
        
        let lowerCaseLine = aString.toLowerCase();

        
        if (!lowerCaseLine.includes(lowerCaseToken)) {
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

    this.DebuggerView.setEditorLocation(actor, line + 1, { noDebug: true });

    let range = lineResultsItem.lineData.range;
    let cursor = this.DebuggerView.editor.getOffset({ line: line, ch: 0 });
    let [ anchor, head ] = this.DebuggerView.editor.getPosition(
      cursor + range.start,
      cursor + range.start + range.length
    );

    this.DebuggerView.editor.setSelection(anchor, head);
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

DebuggerView.GlobalSearch = new GlobalSearchView(DebuggerController, DebuggerView);





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










function SourceResults(aActor, aGlobalResults, sourcesView) {
  let item = sourcesView.getItemByValue(aActor);
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
