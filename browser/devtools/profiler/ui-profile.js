


"use strict";








let ProfileView = {
  


  initialize: function() {
    this._tabs = $("#profile-content tabs");
    this._panels = $("#profile-content tabpanels");
    this._tabTemplate = $("#profile-content-tab-template");
    this._panelTemplate = $("#profile-content-tabpanel-template");
    this._newtabButton = $("#profile-newtab-button");

    this._recordingInfoByPanel = new WeakMap();
    this._framerateGraphByPanel = new Map();
    this._categoriesGraphByPanel = new Map();
    this._callTreeRootByPanel = new Map();

    this._onTabSelect = this._onTabSelect.bind(this);
    this._onNewTabClick = this._onNewTabClick.bind(this);
    this._onGraphLegendSelection = this._onGraphLegendSelection.bind(this);
    this._onGraphMouseUp = this._onGraphMouseUp.bind(this);
    this._onGraphScroll = this._onGraphScroll.bind(this);
    this._onCallViewFocus = this._onCallViewFocus.bind(this);
    this._onCallViewLink = this._onCallViewLink.bind(this);
    this._onCallViewZoom = this._onCallViewZoom.bind(this);

    this._panels.addEventListener("select", this._onTabSelect, false);
    this._newtabButton.addEventListener("click", this._onNewTabClick, false);
  },

  


  destroy: function() {
    this.removeAllTabs();

    this._panels.removeEventListener("select", this._onTabSelect, false);
    this._newtabButton.removeEventListener("click", this._onNewTabClick, false);
  },

  



  showEmptyNotice: function() {
    $("#profile-pane").selectedPanel = $("#empty-notice");
    window.emit(EVENTS.EMPTY_NOTICE_SHOWN);
  },

  



  showRecordingNotice: function() {
    $("#profile-pane").selectedPanel = $("#recording-notice");
    window.emit(EVENTS.RECORDING_NOTICE_SHOWN);
  },

  



  showLoadingNotice: function() {
    $("#profile-pane").selectedPanel = $("#loading-notice");
    window.emit(EVENTS.LOADING_NOTICE_SHOWN);
  },

  


  showTabbedBrowser: function() {
    $("#profile-pane").selectedPanel = $("#profile-content");
    window.emit(EVENTS.TABBED_BROWSER_SHOWN);
  },

  






  selectTab: function(tabIndex) {
    $("#profile-content").selectedIndex = tabIndex;
  },

  





  addTab: function() {
    let tab = this._tabs.appendChild(this._tabTemplate.cloneNode(true));
    let panel = this._panels.appendChild(this._panelTemplate.cloneNode(true));

    
    tab.removeAttribute("covered");

    let tabIndex = this._tabs.itemCount - 1;
    return tabIndex;
  },

  







  nameTab: function(tabIndex, beginAt, endAt) {
    let tab = this._getTab(tabIndex);
    let a = L10N.numberWithDecimals(beginAt, 2);
    let b = L10N.numberWithDecimals(endAt, 2);
    let labelNode = $(".tab-title-label", tab);
    labelNode.setAttribute("value", L10N.getFormatStr("profile.tab", a, b));
  },

  















  populateTab: Task.async(function*(tabIndex, recordingData, beginAt, endAt, options) {
    let tab = this._getTab(tabIndex);
    let panel = this._getPanel(tabIndex);
    if (!tab || !panel) {
      return;
    }

    this._recordingInfoByPanel.set(panel, {
      recordingData: recordingData,
      displayRange: { beginAt: beginAt, endAt: endAt }
    });

    let { profilerData, ticksData } = recordingData;
    let categoriesData = RecordingUtils.plotCategoriesFor(profilerData, beginAt, endAt);
    let framerateData = RecordingUtils.plotFramerateFor(ticksData, beginAt, endAt);
    RecordingUtils.syncCategoriesWithFramerate(categoriesData, framerateData);

    yield this._populatePanelWidgets(panel, {
      profilerData: profilerData,
      framerateData: framerateData,
      categoriesData: categoriesData
    }, beginAt, endAt, options);
  }),

  













  addTabAndPopulate: Task.async(function*(recordingData, beginAt, endAt, options) {
    let tabIndex = this.addTab();
    this.nameTab(tabIndex, beginAt, endAt);

    
    
    yield DevToolsUtils.waitForTime(RECORDING_DATA_DISPLAY_DELAY);
    yield this.populateTab(tabIndex, recordingData, beginAt, endAt, options);
    this.selectTab(tabIndex);
  }),

  


  removeAllTabs: function() {
    for (let [, graph] of this._framerateGraphByPanel) graph.destroy();
    for (let [, graph] of this._categoriesGraphByPanel) graph.destroy();
    for (let [, root] of this._callTreeRootByPanel) root.remove();

    this._recordingInfoByPanel = new WeakMap();
    this._framerateGraphByPanel.clear();
    this._categoriesGraphByPanel.clear();
    this._callTreeRootByPanel.clear();

    while (this._tabs.hasChildNodes()) {
      this._tabs.firstChild.remove();
    }
    while (this._panels.hasChildNodes()) {
      this._panels.firstChild.remove();
    }
  },

  





  removeTabsAfter: function(tabIndex) {
    tabIndex++;

    while (tabIndex < this._tabs.itemCount) {
      let tab = this._getTab(tabIndex);
      let panel = this._getPanel(tabIndex);

      this._framerateGraphByPanel.delete(panel);
      this._categoriesGraphByPanel.delete(panel);
      this._callTreeRootByPanel.delete(panel);
      tab.remove();
      panel.remove();
    }
  },

  



  get tabCount() {
    let tabs = this._tabs.childNodes.length;
    let tabpanels = this._panels.childNodes.length;
    if (tabs != tabpanels) {
      throw "The number of tabs isn't equal to the number of tabpanels.";
    }
    return tabs;
  },

  



  _spawnTabFromSelection: Task.async(function*() {
    let { recordingData } = this._getRecordingInfo();
    let categoriesGraph = this._getCategoriesGraph();

    
    let { min: beginAt, max: endAt } = categoriesGraph.getMappedSelection();

    
    
    this._newtabButton.hidden = true;

    yield this.addTabAndPopulate(recordingData, beginAt, endAt);

    
    window.emit(EVENTS.TAB_SPAWNED_FROM_SELECTION);
  }),

  






  _spawnTabFromFrameNode: Task.async(function*(frameNode) {
    let { recordingData } = this._getRecordingInfo();
    let sampleTimes = frameNode.sampleTimes;
    let beginAt = sampleTimes[0].start;
    let endAt = sampleTimes[sampleTimes.length - 1].end;

    
    
    this._newtabButton.hidden = true;

    yield this.addTabAndPopulate(recordingData, beginAt, endAt, { skipCallTree: true });
    this._populateCallTreeFromFrameNode(this._getPanel(), frameNode);

    
    window.emit(EVENTS.TAB_SPAWNED_FROM_FRAME_NODE);
  }),

  







  _zoomTreeFromSelection: function(options) {
    let { recordingData, displayRange } = this._getRecordingInfo();
    let categoriesGraph = this._getCategoriesGraph();
    let selectedPanel = this._getPanel();

    
    
    if (!categoriesGraph.hasSelection()) {
      let { beginAt, endAt } = displayRange;
      this._newtabButton.hidden = true;
      this._populateCallTree(selectedPanel, recordingData.profilerData, beginAt, endAt, options);
    }
    
    
    else {
      let { min: beginAt, max: endAt } = categoriesGraph.getMappedSelection();
      this._newtabButton.hidden = (endAt - beginAt) < GRAPH_ZOOM_MIN_TIMESPAN;
      this._populateCallTree(selectedPanel, recordingData.profilerData, beginAt, endAt, options);
    }
  },

  






  _highlightAreaFromFrameNode: function(frameNode) {
    let categoriesGraph = this._getCategoriesGraph();
    if (categoriesGraph) {
      categoriesGraph.setMask(frameNode.sampleTimes);
    }
  },

  
















  _populatePanelWidgets: Task.async(function*(panel, dataSource, beginAt, endAt, options = {}) {
    let { profilerData, framerateData, categoriesData } = dataSource;

    let framerateGraph = yield this._populateFramerateGraph(panel, framerateData, beginAt);
    let categoriesGraph = yield this._populateCategoriesGraph(panel, categoriesData, beginAt);
    CanvasGraphUtils.linkAnimation(framerateGraph, categoriesGraph);
    CanvasGraphUtils.linkSelection(framerateGraph, categoriesGraph);

    if (!options.skipCallTree) {
      this._populateCallTree(panel, profilerData, beginAt, endAt, options);
    }
  }),

  










  _populateFramerateGraph: Task.async(function*(panel, framerateData, beginAt) {
    let oldGraph = this._getFramerateGraph(panel);
    if (oldGraph) {
      oldGraph.destroy();
    }

    
    if (!framerateData || framerateData.length < 2) {
      return null;
    }

    let graph = new LineGraphWidget($(".framerate", panel), L10N.getStr("graphs.fps"));
    graph.fixedHeight = FRAMERATE_GRAPH_HEIGHT;
    graph.minDistanceBetweenPoints = 1;
    graph.dataOffsetX = beginAt;

    yield graph.setDataWhenReady(framerateData);

    graph.on("mouseup", this._onGraphMouseUp);
    graph.on("scroll", this._onGraphScroll);

    this._framerateGraphByPanel.set(panel, graph);
    return graph;
  }),

  










  _populateCategoriesGraph: Task.async(function*(panel, categoriesData, beginAt) {
    let oldGraph = this._getCategoriesGraph(panel);
    if (oldGraph) {
      oldGraph.destroy();
    }
    
    if (!categoriesData || categoriesData.length < 2) {
      return null;
    }

    let graph = new BarGraphWidget($(".categories", panel));
    graph.fixedHeight = CATEGORIES_GRAPH_HEIGHT;
    graph.minBarsWidth = CATEGORIES_GRAPH_MIN_BARS_WIDTH;
    graph.format = CATEGORIES.sort((a, b) => a.ordinal > b.ordinal);
    graph.dataOffsetX = beginAt;

    yield graph.setDataWhenReady(categoriesData);

    graph.on("legend-selection", this._onGraphLegendSelection);
    graph.on("mouseup", this._onGraphMouseUp);
    graph.on("scroll", this._onGraphScroll);

    this._categoriesGraphByPanel.set(panel, graph);
    return graph;
  }),

  















  _populateCallTree: function(panel, profilerData, beginAt, endAt, options) {
    let threadSamples = profilerData.profile.threads[0].samples;
    let contentOnly = !Prefs.showPlatformData;
    let threadNode = new ThreadNode(threadSamples, contentOnly, beginAt, endAt);
    this._populateCallTreeFromFrameNode(panel, threadNode, options);
  },

  











  _populateCallTreeFromFrameNode: function(panel, frameNode, options = {}) {
    let oldRoot = this._getCallTreeRoot(panel);
    if (oldRoot) {
      oldRoot.remove();
    }

    let callTreeRoot = new CallView({ frame: frameNode });
    callTreeRoot.on("focus", this._onCallViewFocus);
    callTreeRoot.on("link", this._onCallViewLink);
    callTreeRoot.on("zoom", this._onCallViewZoom);
    callTreeRoot.attachTo($(".call-tree-cells-container", panel));

    if (!options.skipCallTreeFocus) {
      callTreeRoot.focus();
    }

    let contentOnly = !Prefs.showPlatformData;
    callTreeRoot.toggleCategories(!contentOnly);

    this._callTreeRootByPanel.set(panel, callTreeRoot);
  },

  




  _getRecordingInfo: function(panel = this._getPanel()) {
    return this._recordingInfoByPanel.get(panel);
  },
  _getFramerateGraph: function(panel = this._getPanel()) {
    return this._framerateGraphByPanel.get(panel);
  },
  _getCategoriesGraph: function(panel = this._getPanel()) {
    return this._categoriesGraphByPanel.get(panel);
  },
  _getCallTreeRoot: function(panel = this._getPanel()) {
    return this._callTreeRootByPanel.get(panel);
  },
  _getTab: function(tabIndex = this._getSelectedIndex()) {
    return this._tabs.childNodes[tabIndex];
  },
  _getPanel: function(tabIndex = this._getSelectedIndex()) {
    return this._panels.childNodes[tabIndex];
  },
  _getSelectedIndex: function() {
    return $("#profile-content").selectedIndex;
  },

  


  _onTabSelect: function() {
    let categoriesGraph = this._getCategoriesGraph();
    if (categoriesGraph) {
      this._newtabButton.hidden = !categoriesGraph.hasSelection();
    } else {
      this._newtabButton.hidden = true;
    }

    this.removeTabsAfter(this._getSelectedIndex());
  },

  


  _onNewTabClick: function() {
    this._spawnTabFromSelection();
  },

  


  _onGraphLegendSelection: function() {
    this._zoomTreeFromSelection({ skipCallTreeFocus: true });
  },

  


  _onGraphMouseUp: function() {
    this._zoomTreeFromSelection();
  },

  


  _onGraphScroll: function() {
    setNamedTimeout("graph-scroll", GRAPH_SCROLL_EVENTS_DRAIN, () => {
      this._zoomTreeFromSelection();
    });
  },

  


  _onCallViewFocus: function(event, treeItem) {
    setNamedTimeout("graph-focus", CALL_VIEW_FOCUS_EVENTS_DRAIN, () => {
      this._highlightAreaFromFrameNode(treeItem.frame);
    });
  },

  


  _onCallViewLink: function(event, treeItem) {
    let { url, line } = treeItem.frame.getInfo();
    viewSourceInDebugger(url, line);
  },

  


  _onCallViewZoom: function(event, treeItem) {
    this._spawnTabFromFrameNode(treeItem.frame);
  }
};




let RecordingUtils = {
  _frameratePlotsCache: new WeakMap(),

  












  plotCategoriesFor: function(profilerData, beginAt, endAt) {
    let categoriesData = [];
    let profile = profilerData.profile;
    let samples = profile.threads[0].samples;

    
    for (let { frames, time } of samples) {
      if (!time || time < beginAt || time > endAt) continue;
      let blocks = [];

      for (let { category: bitmask } of frames) {
        if (!bitmask) continue;
        let category = CATEGORY_MAPPINGS[bitmask];

        
        
        
        if (!category) {
          category = CATEGORY_MAPPINGS[CATEGORY_OTHER];
        }

        if (!blocks[category.ordinal]) {
          blocks[category.ordinal] = 1;
        } else {
          blocks[category.ordinal]++;
        }
      }

      
      
      if (blocks.length == 0) {
        blocks[CATEGORY_MAPPINGS[CATEGORY_OTHER].ordinal] = frames.length;
      }

      categoriesData.push({
        delta: time,
        values: blocks
      });
    }

    return categoriesData;
  },

  












  plotFramerateFor: function(ticksData, beginAt, endAt) {
    
    
    if (ticksData == null) {
      return [];
    }

    let framerateData = this._frameratePlotsCache.get(ticksData);
    if (framerateData == null) {
      framerateData = FramerateFront.plotFPS(ticksData, FRAMERATE_CALC_INTERVAL);
      this._frameratePlotsCache.set(ticksData, framerateData);
    }

    
    
    
    let earliestValidIndex = findFirstIndex(framerateData, e => e.delta >= beginAt);
    let oldestValidIndex = findLastIndex(framerateData, e => e.delta <= endAt);
    let totalValues = framerateData.length;

    
    
    if (earliestValidIndex == 0 && oldestValidIndex == totalValues - 1) {
      return framerateData;
    }

    
    
    
    
    let slicedData = framerateData.slice(earliestValidIndex, oldestValidIndex + 1);
    if (earliestValidIndex > 0) {
      slicedData.unshift({
        delta: beginAt,
        value: framerateData[earliestValidIndex - 1].value
      });
    }
    if (oldestValidIndex < totalValues - 1) {
      slicedData.push({
        delta: endAt,
        value: framerateData[oldestValidIndex + 1].value
      });
    }

    return slicedData;
  },

  








  syncCategoriesWithFramerate: function(categoriesData, framerateData) {
    if (categoriesData.length < 2 || framerateData.length < 2) {
      return;
    }
    let categoryBegin = categoriesData[0];
    let categoryEnd = categoriesData[categoriesData.length - 1];
    let framerateBegin = framerateData[0];
    let framerateEnd = framerateData[framerateData.length - 1];

    if (categoryBegin.delta > framerateBegin.delta) {
      categoriesData.unshift({
        delta: framerateBegin.delta,
        values: categoryBegin.values
      });
    } else {
      framerateData.unshift({
        delta: categoryBegin.delta,
        value: framerateBegin.value
      });
    }
    if (categoryEnd.delta < framerateEnd.delta) {
      categoriesData.push({
        delta: framerateEnd.delta,
        values: categoryEnd.values
      });
    } else {
      framerateData.push({
        delta: categoryEnd.delta,
        value: framerateEnd.value
      });
    }
  }
};







function findFirstIndex(array, predicate) {
  for (let i = 0, len = array.length; i < len; i++) {
    if (predicate(array[i])) return i;
  }
}







function findLastIndex(array, predicate) {
  for (let i = array.length - 1; i >= 0; i--) {
    if (predicate(array[i])) return i;
  }
}







function viewSourceInDebugger(url, line) {
  let showSource = ({ DebuggerView }) => {
    if (DebuggerView.Sources.containsValue(url)) {
      DebuggerView.setEditorLocation(url, line, { noDebug: true }).then(() => {
        window.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER);
      }, () => {
        window.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER);
      });
    }
  };

  
  
  
  let debuggerAlreadyOpen = gToolbox.getPanel("jsdebugger");
  gToolbox.selectTool("jsdebugger").then(({ panelWin: dbg }) => {
    if (debuggerAlreadyOpen) {
      showSource(dbg);
    } else {
      dbg.once(dbg.EVENTS.SOURCES_ADDED, () => showSource(dbg));
    }
  });
}
