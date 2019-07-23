









































const Cc = Components.classes;
const Ci = Components.interfaces;

var gSanitizePromptDialog = {

  get bundleBrowser()
  {
    if (!this._bundleBrowser)
      this._bundleBrowser = document.getElementById("bundleBrowser");
    return this._bundleBrowser;
  },

  get selectedTimespan()
  {
    var durList = document.getElementById("sanitizeDurationChoice");
    return parseInt(durList.value);
  },

  get sanitizePreferences()
  {
    if (!this._sanitizePreferences) {
      this._sanitizePreferences =
        document.getElementById("sanitizePreferences");
    }
    return this._sanitizePreferences;
  },

  get warningBox()
  {
    return document.getElementById("sanitizeEverythingWarningBox");
  },

  init: function ()
  {
    
    this._inited = true;

    var s = new Sanitizer();
    s.prefDomain = "privacy.cpd.";
    for (let i = 0; i < this.sanitizePreferences.childNodes.length; ++i) {
      var preference = this.sanitizePreferences.childNodes[i];
      var name = s.getNameFromPreference(preference.name);
      if (!s.canClearItem(name)) 
        preference.disabled = true;
    }

    document.documentElement.getButton("accept").label =
      this.bundleBrowser.getString("sanitizeButtonOK");

    if (this.selectedTimespan === Sanitizer.TIMESPAN_EVERYTHING) {
      this.prepareWarning();
      this.warningBox.hidden = false;
    }
    else
      this.warningBox.hidden = true;
  },

  selectByTimespan: function ()
  {
    
    
    if (!this._inited)
      return;

    var warningBox = this.warningBox;

    
    if (this.selectedTimespan === Sanitizer.TIMESPAN_EVERYTHING) {
      this.prepareWarning();
      if (warningBox.hidden) {
        warningBox.hidden = false;
        window.resizeBy(0, warningBox.boxObject.height);
      }
      window.document.title =
        this.bundleBrowser.getString("sanitizeDialog2.everything.title");
      return;
    }

    
    if (!warningBox.hidden) {
      window.resizeBy(0, -warningBox.boxObject.height);
      warningBox.hidden = true;
    }
    window.document.title =
      window.document.documentElement.getAttribute("noneverythingtitle");
  },

  sanitize: function ()
  {
    
    this.updatePrefs();
    var s = new Sanitizer();
    s.prefDomain = "privacy.cpd.";

    s.range = Sanitizer.getClearRange(this.selectedTimespan);
    s.ignoreTimespan = !s.range;

    try {
      s.sanitize();
    } catch (er) {
      Components.utils.reportError("Exception during sanitize: " + er);
    }
    return true;
  },

  







  prepareWarning: function (aDontShowItemList) {
    
    
    

    var warningStringID;
    if (this.hasCustomizedItemSelection()) {
      warningStringID = "sanitizeSelectedWarning";
      if (!aDontShowItemList)
        this.showItemList();
    }
    else {
      warningStringID = "sanitizeEverythingWarning2";
    }

    var warningDesc = document.getElementById("sanitizeEverythingWarning");
    warningDesc.textContent =
      this.bundleBrowser.getString(warningStringID);
  },

  



  onReadGeneric: function ()
  {
    var found = false;

    
    var i = 0;
    while (!found && i < this.sanitizePreferences.childNodes.length) {
      var preference = this.sanitizePreferences.childNodes[i];

      found = !!preference.value &&
              !preference.disabled;
      i++;
    }

    try {
      document.documentElement.getButton("accept").disabled = !found;
    }
    catch (e) { }

    
    this.prepareWarning(true);

    return undefined;
  },

  







  updatePrefs : function ()
  {
    var tsPref = document.getElementById("privacy.sanitize.timeSpan");
    Sanitizer.prefs.setIntPref("timeSpan", this.selectedTimespan);

    
    document.getElementById("privacy.cpd.downloads").value =
      document.getElementById("privacy.cpd.history").value;

    
    
    var prefs = this.sanitizePreferences.rootBranch;
    for (let i = 0; i < this.sanitizePreferences.childNodes.length; ++i) {
      var p = this.sanitizePreferences.childNodes[i];
      prefs.setBoolPref(p.name, p.value);
    }
  },

  


  hasCustomizedItemSelection: function () {
    let checkboxes = document.querySelectorAll("#itemList > [preference]");
    for (let i = 0; i < checkboxes.length; ++i) {
      let pref = document.getElementById(checkboxes[i].getAttribute("preference"));
      if (pref.value != pref.defaultValue)
        return true;
    }
    return false;
  },

  


  showItemList: function () {
    var itemList = document.getElementById("itemList");
    var expanderButton = document.getElementById("detailsExpander");

    if (itemList.collapsed) {
      expanderButton.className = "expander-up";
      itemList.setAttribute("collapsed", "false");
      if (document.documentElement.boxObject.height)
        window.resizeBy(0, itemList.boxObject.height);
    }
  },

  


  hideItemList: function () {
    var itemList = document.getElementById("itemList");
    var expanderButton = document.getElementById("detailsExpander");

    if (!itemList.collapsed) {
      expanderButton.className = "expander-down";
      window.resizeBy(0, -itemList.boxObject.height);
      itemList.setAttribute("collapsed", "true");
    }
  },

  


  toggleItemList: function ()
  {
    var itemList = document.getElementById("itemList");

    if (itemList.collapsed)
      this.showItemList();
    else
      this.hideItemList();
  }

#ifdef CRH_DIALOG_TREE_VIEW
  
  
  
  get TIMESPAN_CUSTOM()
  {
    return -1;
  },

  get placesTree()
  {
    if (!this._placesTree)
      this._placesTree = document.getElementById("placesTree");
    return this._placesTree;
  },

  init: function ()
  {
    
    this._inited = true;

    var s = new Sanitizer();
    s.prefDomain = "privacy.cpd.";
    for (let i = 0; i < this.sanitizePreferences.childNodes.length; ++i) {
      var preference = this.sanitizePreferences.childNodes[i];
      var name = s.getNameFromPreference(preference.name);
      if (!s.canClearItem(name)) 
        preference.disabled = true;
    }

    document.documentElement.getButton("accept").label =
      this.bundleBrowser.getString("sanitizeButtonOK");

    this.selectByTimespan();
  },

  





  initDurationDropdown: function ()
  {
    
    this.durationStartTimes = {};
    var durVals = [];
    var durPopup = document.getElementById("sanitizeDurationPopup");
    var durMenuitems = durPopup.childNodes;
    for (let i = 0; i < durMenuitems.length; i++) {
      let durMenuitem = durMenuitems[i];
      let durVal = parseInt(durMenuitem.value);
      if (durMenuitem.localName === "menuitem" &&
          durVal !== Sanitizer.TIMESPAN_EVERYTHING &&
          durVal !== this.TIMESPAN_CUSTOM) {
        durVals.push(durVal);
        let durTimes = Sanitizer.getClearRange(durVal);
        this.durationStartTimes[durVal] = durTimes[0];
      }
    }

    
    
    
    durVals.sort();

    
    
    
    this.durationRowsToVals = {};
    this.durationValsToRows = {};
    var view = this.placesTree.view;
    
    for (let i = 0; i < view.rowCount - 1; i++) {
      let unfoundDurVals = [];
      let nodeTime = view.QueryInterface(Ci.nsINavHistoryResultTreeViewer).
                     nodeForTreeIndex(i).time;
      
      
      
      for (let j = 0; j < durVals.length; j++) {
        let durVal = durVals[j];
        let durStartTime = this.durationStartTimes[durVal];
        if (nodeTime < durStartTime) {
          this.durationValsToRows[durVal] = i - 1;
          this.durationRowsToVals[i - 1] = durVal;
        }
        else
          unfoundDurVals.push(durVal);
      }
      durVals = unfoundDurVals;
    }

    
    
    
    for (let i = 0; i < durVals.length; i++) {
      let durVal = durVals[i];
      this.durationValsToRows[durVal] = view.rowCount - 2;
      this.durationRowsToVals[view.rowCount - 2] = durVal;
    }
  },

  


  ensurePlacesTreeIsInited: function ()
  {
    if (this._placesTreeIsInited)
      return;

    this._placesTreeIsInited = true;

    
    
    
    var times = Sanitizer.getClearRange(Sanitizer.TIMESPAN_TODAY);

    
    if (times[1] - times[0] < 14400000000) { 
      times = Sanitizer.getClearRange(Sanitizer.TIMESPAN_4HOURS);
    }

    var histServ = Cc["@mozilla.org/browser/nav-history-service;1"].
                   getService(Ci.nsINavHistoryService);
    var query = histServ.getNewQuery();
    query.beginTimeReference = query.TIME_RELATIVE_EPOCH;
    query.beginTime = times[0];
    query.endTimeReference = query.TIME_RELATIVE_EPOCH;
    query.endTime = times[1];
    var opts = histServ.getNewQueryOptions();
    opts.sortingMode = opts.SORT_BY_DATE_DESCENDING;
    opts.queryType = opts.QUERY_TYPE_HISTORY;
    var result = histServ.executeQuery(query, opts);

    var view = gContiguousSelectionTreeHelper.setTree(this.placesTree,
                                                      new PlacesTreeView());
    result.viewer = view;
    this.initDurationDropdown();
  },

  





  selectByTimespan: function ()
  {
    
    
    if (!this._inited)
      return;

    var durDeck = document.getElementById("durationDeck");
    var durList = document.getElementById("sanitizeDurationChoice");
    var durVal = parseInt(durList.value);
    var durCustom = document.getElementById("sanitizeDurationCustom");

    
    
    
    
    if (durVal === this.TIMESPAN_CUSTOM) {
      durCustom.hidden = false;
      return;
    }
    durCustom.hidden = true;

    
    if (durVal === Sanitizer.TIMESPAN_EVERYTHING) {
      this.prepareWarning();
      durDeck.selectedIndex = 1;
      window.document.title =
        this.bundleBrowser.getString("sanitizeDialog2.everything.title");
      document.documentElement.getButton("accept").disabled = false;
      return;
    }

    
    
    this.ensurePlacesTreeIsInited();
    durDeck.selectedIndex = 0;
    window.document.title =
      window.document.documentElement.getAttribute("noneverythingtitle");
    var durRow = this.durationValsToRows[durVal];
    gContiguousSelectionTreeHelper.rangedSelect(durRow);
    gContiguousSelectionTreeHelper.scrollToGrippy();

    
    
    document.documentElement.getButton("accept").disabled = durRow < 0;
  },

  sanitize: function ()
  {
    
    this.updatePrefs();
    var s = new Sanitizer();
    s.prefDomain = "privacy.cpd.";

    var durList = document.getElementById("sanitizeDurationChoice");
    var durValue = parseInt(durList.value);
    s.ignoreTimespan = durValue === Sanitizer.TIMESPAN_EVERYTHING;

    
    if (!s.ignoreTimespan) {
      
      if (durValue === this.TIMESPAN_CUSTOM) {
        var view = this.placesTree.view;
        var now = Date.now() * 1000;
        
        
        if (view.selection.getRangeCount() === 0)
          s.range = [now, now];
        else {
          var startIndexRef = {};
          
          view.selection.getRangeAt(0, {}, startIndexRef);
          view.QueryInterface(Ci.nsINavHistoryResultTreeViewer);
          var startNode = view.nodeForTreeIndex(startIndexRef.value);
          s.range = [startNode.time, now];
        }
      }
      
      else
        s.range = [this.durationStartTimes[durValue], Date.now() * 1000];
    }

    try {
      s.sanitize();
    } catch (er) {
      Components.utils.reportError("Exception during sanitize: " + er);
    }
    return true;
  },

  




  unload: function ()
  {
    var view = this.placesTree.view;
    view.QueryInterface(Ci.nsINavHistoryResultViewer).result.viewer = null;
    this.placesTree.view = null;
  },

  










  grippyMoved: function (aEventName, aEvent)
  {
    gContiguousSelectionTreeHelper[aEventName](aEvent);
    var lastSelRow = gContiguousSelectionTreeHelper.getGrippyRow() - 1;
    var durList = document.getElementById("sanitizeDurationChoice");
    var durValue = parseInt(durList.value);

    
    
    if ((durValue !== this.TIMESPAN_CUSTOM ||
         lastSelRow in this.durationRowsToVals) &&
        (durValue === this.TIMESPAN_CUSTOM ||
         this.durationValsToRows[durValue] !== lastSelRow)) {
      
      
      if (lastSelRow in this.durationRowsToVals)
        durList.value = this.durationRowsToVals[lastSelRow];
      else
        durList.value = this.TIMESPAN_CUSTOM;
    }

    
    document.documentElement.getButton("accept").disabled = lastSelRow < 0;
  }
#endif

};


#ifdef CRH_DIALOG_TREE_VIEW



var gContiguousSelectionTreeHelper = {

  


  get tree()
  {
    return this._tree;
  },

  












  setTree: function CSTH_setTree(aTreeElement, aProtoTreeView)
  {
    this._tree = aTreeElement;
    var newView = this._makeTreeView(aProtoTreeView || aTreeElement.view);
    aTreeElement.view = newView;
    return newView;
  },

  






  getGrippyRow: function CSTH_getGrippyRow()
  {
    var sel = this.tree.view.selection;
    var rangeCount = sel.getRangeCount();
    if (rangeCount === 0)
      return 0;
    if (rangeCount !== 1) {
      throw "contiguous selection tree helper: getGrippyRow called with " +
            "multiple selection ranges";
    }
    var max = {};
    sel.getRangeAt(0, {}, max);
    return max.value + 1;
  },

  






  ondragover: function CSTH_ondragover(aEvent)
  {
    
    
    var ds = Cc["@mozilla.org/widget/dragservice;1"].
             getService(Ci.nsIDragService).
             getCurrentSession();
    ds.canDrop = true;
    ds.dragAction = 0;

    var tbo = this.tree.treeBoxObject;
    aEvent.QueryInterface(Ci.nsIDOMMouseEvent);
    var hoverRow = tbo.getRowAt(aEvent.clientX, aEvent.clientY);

    if (hoverRow < 0)
      return;

    this.rangedSelect(hoverRow - 1);
  },

  






  ondragstart: function CSTH_ondragstart(aEvent)
  {
    var tbo = this.tree.treeBoxObject;
    var clickedRow = tbo.getRowAt(aEvent.clientX, aEvent.clientY);

    if (clickedRow !== this.getGrippyRow())
      return;

    
    
    
    
    var arr = Cc["@mozilla.org/supports-array;1"].
              createInstance(Ci.nsISupportsArray);
    var trans = Cc["@mozilla.org/widget/transferable;1"].
                createInstance(Ci.nsITransferable);
    trans.setTransferData('dummy-flavor', null, 0);
    arr.AppendElement(trans);
    var reg = Cc["@mozilla.org/gfx/region;1"].
              createInstance(Ci.nsIScriptableRegion);
    reg.setToRect(Infinity, Infinity, 1, 1);
    var ds = Cc["@mozilla.org/widget/dragservice;1"].
             getService(Ci.nsIDragService);
    ds.invokeDragSession(aEvent.target, arr, reg, ds.DRAGDROP_ACTION_MOVE);
  },

  







  onkeypress: function CSTH_onkeypress(aEvent)
  {
    var grippyRow = this.getGrippyRow();
    var tbo = this.tree.treeBoxObject;
    var rangeEnd;
    switch (aEvent.keyCode) {
    case aEvent.DOM_VK_HOME:
      rangeEnd = 0;
      break;
    case aEvent.DOM_VK_PAGE_UP:
      rangeEnd = grippyRow - tbo.getPageLength();
      break;
    case aEvent.DOM_VK_UP:
      rangeEnd = grippyRow - 2;
      break;
    case aEvent.DOM_VK_DOWN:
      rangeEnd = grippyRow;
      break;
    case aEvent.DOM_VK_PAGE_DOWN:
      rangeEnd = grippyRow + tbo.getPageLength();
      break;
    case aEvent.DOM_VK_END:
      rangeEnd = this.tree.view.rowCount - 2;
      break;
    default:
      return;
      break;
    }

    aEvent.stopPropagation();

    
    
    if (rangeEnd < 0)
      rangeEnd = -1;
    else if (this.tree.view.rowCount - 2 < rangeEnd)
      rangeEnd = this.tree.view.rowCount - 2;

    
    this.rangedSelect(rangeEnd);

    
    
    if (rangeEnd < grippyRow) 
      tbo.ensureRowIsVisible(rangeEnd < 0 ? 0 : rangeEnd);
    else {                    
      if (rangeEnd + 2 < this.tree.view.rowCount)
        tbo.ensureRowIsVisible(rangeEnd + 2);
      else if (rangeEnd + 1 < this.tree.view.rowCount)
        tbo.ensureRowIsVisible(rangeEnd + 1);
    }
  },

  







  onmousedown: function CSTH_onmousedown(aEvent)
  {
    var tbo = this.tree.treeBoxObject;
    var clickedRow = tbo.getRowAt(aEvent.clientX, aEvent.clientY);

    if (clickedRow < 0 || clickedRow >= this.tree.view.rowCount)
      return;

    if (clickedRow < this.getGrippyRow())
      this.rangedSelect(clickedRow);
    else if (clickedRow > this.getGrippyRow())
      this.rangedSelect(clickedRow - 1);
  },

  







  rangedSelect: function CSTH_rangedSelect(aEndRow)
  {
    var tbo = this.tree.treeBoxObject;
    if (aEndRow < 0)
      this.tree.view.selection.clearSelection();
    else
      this.tree.view.selection.rangedSelect(0, aEndRow, false);
    tbo.invalidateRange(tbo.getFirstVisibleRow(), tbo.getLastVisibleRow());
  },

  


  scrollToGrippy: function CSTH_scrollToGrippy()
  {
    var rowCount = this.tree.view.rowCount;
    var tbo = this.tree.treeBoxObject;
    var pageLen = tbo.getPageLength() ||
                  parseInt(this.tree.getAttribute("rows")) ||
                  10;

    
    if (rowCount <= pageLen)
      return;

    var scrollToRow = this.getGrippyRow() - Math.ceil(pageLen / 2.0);

    
    if (scrollToRow < 0)
      scrollToRow = 0;

    
    else if (rowCount < scrollToRow + pageLen)
      scrollToRow = rowCount - pageLen;

    tbo.scrollToRow(scrollToRow);
  },

  







  _makeTreeView: function CSTH__makeTreeView(aProtoTreeView)
  {
    var atomServ = Cc["@mozilla.org/atom-service;1"].
                   getService(Ci.nsIAtomService);

    var view = aProtoTreeView;
    var that = this;

    
    
    view.isSeparator = function CSTH_View_isSeparator(aRow)
    {
      return aRow === that.getGrippyRow();
    };

    
    view.__defineGetter__("_rowCount", view.__lookupGetter__("rowCount"));
    view.__defineGetter__("rowCount",
      function CSTH_View_rowCount()
      {
        return this._rowCount + 1;
      });

    
    
    view.canDrop = function CSTH_View_canDrop() { return false; };

    
    view.cycleHeader = function CSTH_View_cycleHeader() {};
    view.sortingChanged = function CSTH_View_sortingChanged() {};

    

    view._getCellProperties = view.getCellProperties;
    view.getCellProperties =
      function CSTH_View_getCellProperties(aRow, aCol, aProps)
      {
        var grippyRow = that.getGrippyRow();
        if (aRow === grippyRow)
          aProps.AppendElement(atomServ.getAtom("grippyRow"));
        else if (aRow < grippyRow)
          this._getCellProperties(aRow, aCol, aProps);
        else
          this._getCellProperties(aRow - 1, aCol, aProps);
      };

    view._getRowProperties = view.getRowProperties;
    view.getRowProperties =
      function CSTH_View_getRowProperties(aRow, aProps)
      {
        var grippyRow = that.getGrippyRow();
        if (aRow === grippyRow)
          aProps.AppendElement(atomServ.getAtom("grippyRow"));
        else if (aRow < grippyRow)
          this._getRowProperties(aRow, aProps);
        else
          this._getRowProperties(aRow - 1, aProps);
      };

    view._getCellText = view.getCellText;
    view.getCellText =
      function CSTH_View_getCellText(aRow, aCol)
      {
        var grippyRow = that.getGrippyRow();
        if (aRow === grippyRow)
          return "";
        aRow = aRow < grippyRow ? aRow : aRow - 1;
        return this._getCellText(aRow, aCol);
      };

    view._getImageSrc = view.getImageSrc;
    view.getImageSrc =
      function CSTH_View_getImageSrc(aRow, aCol)
      {
        var grippyRow = that.getGrippyRow();
        if (aRow === grippyRow)
          return "";
        aRow = aRow < grippyRow ? aRow : aRow - 1;
        return this._getImageSrc(aRow, aCol);
      };

    view.isContainer = function CSTH_View_isContainer(aRow) { return false; };
    view.getParentIndex = function CSTH_View_getParentIndex(aRow) { return -1; };
    view.getLevel = function CSTH_View_getLevel(aRow) { return 0; };
    view.hasNextSibling = function CSTH_View_hasNextSibling(aRow, aAfterIndex)
    {
      return aRow < this.rowCount - 1;
    };

    return view;
  }
};
#endif
