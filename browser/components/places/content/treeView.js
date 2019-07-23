






































PlacesTreeView.prototype = {
  _makeAtom: function PTV__makeAtom(aString) {
    return  Cc["@mozilla.org/atom-service;1"].
            getService(Ci.nsIAtomService).
            getAtom(aString);
  },

  __separatorAtom: null,
  get _separatorAtom() {
    if (!this.__separatorAtom)
      this.__separatorAtom = this._makeAtom("separator");

    return this.__separatorAtom;
  },

  __sessionStartAtom: null,
  get _sessionStartAtom() {
    if (!this.__sessionStartAtom)
      this.__sessionStartAtom = this._makeAtom("session-start");

    return this.__sessionStartAtom;
  },

  __sessionContinueAtom: null,
  get _sessionContinueAtom() {
    if (!this.__sessionContinueAtom)
      this.__sessionContinueAtom = this._makeAtom("session-continue");

    return this.__sessionContinueAtom;
  },

  _ensureValidRow: function PTV__ensureValidRow(aRow) {
    if (aRow < 0 || aRow > this._visibleElements.length)
      throw Cr.NS_ERROR_INVALID_ARG;
  },

  __dateService: null,
  get _dateService() {
    if (!this.__dateService) {
      this.__dateService = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                           getService(Ci.nsIScriptableDateFormat);
    }
    return this.__dateService;
  },

  QueryInterface: function PTV_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsITreeView) ||
        aIID.equals(Ci.nsINavHistoryResultViewer) ||
        aIID.equals(Ci.nsINavHistoryResultTreeViewer) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  




  _finishInit: function PTV__finishInit() {
    if (this._tree && this._result)
      this.sortingChanged(this._result.sortingMode);

    
    this._buildVisibleList();
  },

  _computeShowSessions: function PTV__computeShowSessions() {
    NS_ASSERT(this._result, "Must have a result to show sessions!");
    this._showSessions = false;

    var options = asQuery(this._result.root).queryOptions;
    NS_ASSERT(options, "navHistoryResults must have valid options");

    if (!options.showSessions)
      return; 

    var resultType = options.resultType;
    if (resultType != Ci.nsINavHistoryQueryOptions.RESULTS_AS_VISIT &&
        resultType != Ci.nsINavHistoryQueryOptions.RESULTS_AS_FULL_VISIT)
      return; 

    var sortType = this._result.sortingMode;
    if (sortType != nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING &&
        sortType != nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING)
      return; 

    
    
    var groupings = options.getGroupingMode({});
    for (var i=0; i < groupings.length; i++) {
      if (groupings[i] != Ci.nsINavHistoryQueryOptions.GROUP_BY_DAY)
        return; 
    }

    this._showSessions = true;
  },

  SESSION_STATUS_NONE: 0,
  SESSION_STATUS_START: 1,
  SESSION_STATUS_CONTINUE: 2,
  _getRowSessionStatus: function PTV__getRowSessionStatus(aRow) {
    this._ensureValidRow(aRow);
    var node = this._visibleElements[aRow];
    if (!PlacesUtils.nodeIsVisit(node) || asVisit(node).sessionId == 0)
      return this.SESSION_STATUS_NONE;

    if (aRow == 0)
      return this.SESSION_STATUS_START;

    var previousNode = this._visibleElements[aRow - 1];
    if (!PlacesUtils.nodeIsVisit(previousNode) ||
        node.sessionId != asVisit(previousNode).sessionId)
      return this.SESSION_STATUS_START;

    return this.SESSION_STATUS_CONTINUE;
  },

  






  _buildVisibleList: function PTV__buildVisibleList() {
    if (this._result) {
      
      for (var i = 0; i < this._visibleElements.length; i++) {
        this._visibleElements[i].viewIndex = -1;
      }
    }

    var oldCount = this.rowCount;
    this._visibleElements.splice(0);
    if (this._tree)
      this._tree.rowCountChanged(0, -oldCount);

    var rootNode = this._result.root;
    if (rootNode && this._tree) {
      this._computeShowSessions();

      asContainer(rootNode);
      if (this._showRoot) {
        
        this._visibleElements.push(this._result.root);
        this._tree.rowCountChanged(0, 1);
        this._result.root.viewIndex = 0;
      }
      else if (!rootNode.containerOpen) {
        
        
        rootNode.containerOpen = true;
        return;
      }

      this.invalidateContainer(rootNode);
    }
  },

  











  _buildVisibleSection:
  function PTV__buildVisibleSection(aContainer, aVisible, aVisibleStartIndex)
  {
    if (!aContainer.containerOpen)
      return;  

    var cc = aContainer.childCount;
    for (var i=0; i < cc; i++) {
      var curChild = aContainer.getChild(i);

      
      if (this._collapseDuplicates) {
        var showThis = { value: false };
        while (i <  cc - 1 &&
               this._canCollapseDuplicates(curChild, aContainer.getChild(i+1),
                                           showThis)) {
          if (showThis.value) {
            
            curChild.viewIndex = -1;
            curChild = aContainer.getChild(i+1);
          }
          else {
            
            aContainer.getChild(i+1).viewIndex = -1;
          }
          i++;
        }
      }

      
      if (PlacesUtils.nodeIsSeparator(curChild)) {
        if (this._result.sortingMode !=
            Ci.nsINavHistoryQueryOptions.SORT_BY_NONE) {
          curChild.viewIndex = -1;
          continue;
        }
      }

      
      curChild.viewIndex = aVisibleStartIndex + aVisible.length;
      aVisible.push(curChild);

      
      if (PlacesUtils.nodeIsContainer(curChild)) {
        asContainer(curChild);
        if (curChild.containerOpen && curChild.childCount > 0)
          this._buildVisibleSection(curChild, aVisible, aVisibleStartIndex);
      }
    }
  },

  





  _countVisibleRowsForItem: function PTV__countVisibleRowsForItem(aNode) {
    if (aNode == this._result.root)
      return this._visibleElements.length;

    var viewIndex = aNode.viewIndex;
    NS_ASSERT(viewIndex >= 0, "Item is not visible, no rows to count");
    var outerLevel = aNode.indentLevel;
    for (var i = viewIndex + 1; i < this._visibleElements.length; i++) {
      if (this._visibleElements[i].indentLevel <= outerLevel)
        return i - viewIndex;
    }
    
    return this._visibleElements.length - viewIndex;
  },

  








  _refreshVisibleSection: function PTV__refreshVisibleSection(aContainer) {
    NS_ASSERT(this._result, "Need to have a result to update");
    if (!this._tree)
      return;

    
    
    if (this._showRoot || aContainer != this._result.root) {
      if (aContainer.viewIndex < 0 &&
          aContainer.viewIndex > this._visibleElements.length)
        throw "Trying to expand a node that is not visible";

      NS_ASSERT(this._visibleElements[aContainer.viewIndex] == aContainer,
                "Visible index is out of sync!");
    }

    var startReplacement = aContainer.viewIndex + 1;
    var replaceCount = this._countVisibleRowsForItem(aContainer);

    
    
    
    
    if (aContainer.viewIndex != -1)
      replaceCount-=1;

    
    for (var i = 0; i < replaceCount; i ++)
      this._visibleElements[startReplacement + i].viewIndex = -1;

    
    var newElements = [];
    this._buildVisibleSection(aContainer, newElements, startReplacement);

    
    this._visibleElements = 
      this._visibleElements.slice(0, startReplacement).concat(newElements)
          .concat(this._visibleElements.slice(startReplacement + replaceCount,
                                              this._visibleElements.length));

    if (replaceCount == newElements.length) {
      
      if (replaceCount > 0) {
        this._tree.invalidateRange(startReplacement,
                                   startReplacement + replaceCount - 1);
      }
    }
    else {
      
      
      for (i = startReplacement + newElements.length;
           i < this._visibleElements.length; i ++) {
        this._visibleElements[i].viewIndex = i;
      }

      
      
      
      var minLength = Math.min(newElements.length, replaceCount);
      this._tree.invalidateRange(startReplacement - 1,
                                 startReplacement + minLength - 1);

      
      this._tree.rowCountChanged(startReplacement + minLength,
                                 newElements.length - replaceCount);
    }
  },

  




  _canCollapseDuplicates:
  function PTV__canCollapseDuplicate(aTop, aNext, aShowThisOne) {
    if (!this._collapseDuplicates)
      return false;
    if (!PlacesUtils.nodeIsVisit(aTop) ||
        !PlacesUtils.nodeIsVisit(aNext))
      return false; 

    asVisit(aTop);
    asVisit(aNext);

    if (aTop.uri != aNext.uri)
      return false; 

    
    aShowThisOne.value = aTop.time < aNext.time;
    return true;
  },

  COLUMN_TYPE_UNKNOWN: 0,
  COLUMN_TYPE_TITLE: 1,
  COLUMN_TYPE_URI: 2,
  COLUMN_TYPE_DATE: 3,
  COLUMN_TYPE_VISITCOUNT: 4,
  _getColumnType: function PTV__getColumnType(aColumn) {
    switch (aColumn.id) {
      case "title":
        return this.COLUMN_TYPE_TITLE;
      case "url":
        return this.COLUMN_TYPE_URI;
      case "date":
        return this.COLUMN_TYPE_DATE;
      case "visitCount":
        return this.COLUMN_TYPE_VISITCOUNT;
    }
    return this.COLUMN_TYPE_UNKNOWN;
  },

  _sortTypeToColumnType: function PTV__sortTypeToColumnType(aSortType) {
    switch(aSortType) {
      case Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING:
        return [this.COLUMN_TYPE_TITLE, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_DESCENDING:
        return [this.COLUMN_TYPE_TITLE, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING:
        return [this.COLUMN_TYPE_DATA, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING:
        return [this.COLUMN_TYPE_DATA, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_URI_ASCENDING:
        return [this.COLUMN_TYPE_URI, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_URI_DESCENDING:
        return [this.COLUMN_TYPE_URI, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_ASCENDING:
        return [this.COLUMN_TYPE_VISITCOUNT, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING:
        return [this.COLUMN_TYPE_VISITCOUNT, true];
    }
    return [this.COLUMN_TYPE_UNKNOWN, false];
  },

  
  itemInserted: function PTV_itemInserted(aParent, aItem, aNewIndex) {
    if (!this._tree)
      return;
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    if (PlacesUtils.nodeIsSeparator(aItem) &&
        this._result.sortingMode != Ci.nsINavHistoryQueryOptions.SORT_BY_NONE) {
      aItem.viewIndex = -1;
      return;
    }

    
    
    if (aParent.childCount == 1)
      this.itemChanged(aParent);

    
    var newViewIndex = -1;
    if (aNewIndex == 0) {
      
      
      
      newViewIndex = aParent.viewIndex + 1;
    }
    else {
      
      
      
      
      for (var i = aNewIndex + 1; i < aParent.childCount; i ++) {
        var viewIndex = aParent.getChild(i).viewIndex;
        if (viewIndex >= 0) {
          
          
          newViewIndex = viewIndex;
          break;
        }
      }
      if (newViewIndex < 0) {
        
        
        
        var lastRowCount =
          this._countVisibleRowsForItem(aParent.getChild(aNewIndex - 1));
        newViewIndex =
          aParent.getChild(aNewIndex - 1).viewIndex + lastRowCount;
      }
    }

    
    
    
    
    var showThis =  { value: true };
    if (newViewIndex > 0 &&
        this._canCollapseDuplicates
          (this._visibleElements[newViewIndex - 1], aItem, showThis)) {
      if (!showThis.value) {
        
        aItem.viewIndex = -1;
      }
      else {
        
       this.itemReplaced(aParent, this._visibleElements[newViewIndex - 1],
                         aItem, 0);
      }
      return;
    }

    
    
    if (newViewIndex < this._visibleElements.length &&
        this._canCollapseDuplicates(aItem, this._visibleElements[newViewIndex],
                                    showThis)) {
      if (!showThis.value) {
        
        this.itemReplaced(aParent, this._visibleElements[newViewIndex], aItem,
                          0);
      }
      else {
        
        aItem.viewIndex = -1;
      }
      return;
    }

    
    aItem.viewIndex = newViewIndex;
    this._visibleElements.splice(newViewIndex, 0, aItem);
    for (var i = newViewIndex + 1;
         i < this._visibleElements.length; i ++) {
      this._visibleElements[i].viewIndex = i;
    }
    this._tree.rowCountChanged(newViewIndex, 1);

    
    
    
    
    if (this._showSessions) {
      if (newViewIndex > 0)
        this._tree.invalidateRange(newViewIndex - 1, newViewIndex - 1);
      if (newViewIndex < this._visibleElements.length -1)
        this._tree.invalidateRange(newViewIndex + 1, newViewIndex + 1);
    }

    if (PlacesUtils.nodeIsContainer(aItem) && asContainer(aItem).containerOpen)
      this._refreshVisibleSection(aItem);
  },

  








  itemRemoved: function PTV_itemRemoved(aParent, aItem, aOldIndex) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree)
        return; 

    var oldViewIndex = aItem.viewIndex;
    if (oldViewIndex < 0)
      return; 

    
    var count = this._countVisibleRowsForItem(aItem);

    
    
    
    while (true) {
      NS_ASSERT(oldViewIndex <= this._visibleElements.length,
                "Trying to remove invalid row");
      if (oldViewIndex > this._visibleElements.length)
        return;

      this._visibleElements.splice(oldViewIndex, count);
      for (var i = oldViewIndex; i < this._visibleElements.length; i++)
        this._visibleElements[i].viewIndex = i;

      this._tree.rowCountChanged(oldViewIndex, -count);

      
      if (oldViewIndex > 0 &&
          oldViewIndex < this._visibleElements.length) {
        var showThisOne =  { value: true };
        if (this._canCollapseDuplicates
             (this._visibleElements[oldViewIndex - 1],
              this._visibleElements[oldViewIndex], showThisOne))
        {
          
          
          
          
          
          oldViewIndex = oldViewIndex - 1 + (showThisOne.value ? 1 : 0);
          this._visibleElements[oldViewIndex].viewIndex = -1;
          count = 1; 
          continue;
        }
      }
      break; 
    }

    
    if (!aParent.hasChildren)
      this.itemChanged(aParent);
  },

  







  itemReplaced:
  function PTV_itemReplaced(aParent, aOldItem, aNewItem, aIndexDoNotUse) {
    if (!this._tree)
      return;

    var viewIndex = aOldItem.viewIndex;
    aNewItem.viewIndex = viewIndex;
    if (viewIndex >= 0 &&
        viewIndex < this._visibleElements.length)
      this._visibleElements[viewIndex] = aNewItem;
    aOldItem.viewIndex = -1;
    this._tree.invalidateRow(viewIndex);
  },

  itemChanged: function PTV_itemChanged(aItem) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    var viewIndex = aItem.viewIndex;
    if (this._tree && viewIndex >= 0)
      this._tree.invalidateRow(viewIndex);
  },

  containerOpened: function PTV_containerOpened(aItem) {
    this.invalidateContainer(aItem);
  },

  containerClosed: function PTV_containerClosed(aItem) {
    this.invalidateContainer(aItem);
  },

  invalidateContainer: function PTV_invalidateContainer(aItem) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree)
      return; 
    var viewIndex = aItem.viewIndex;
    if (viewIndex >= this._visibleElements.length) {
      
      throw Cr.NS_ERROR_UNEXPECTED;
    }
    this._refreshVisibleSection(aItem);
  },

  invalidateAll: function PTV_invalidateAll() {
    NS_ASSERT(this._result, "Got message but don't have a result!");
    if (!this._tree)
      return;

    var oldRowCount = this._visibleElements.length;

    
    this._buildVisibleList();

    
    this._tree.rowCountChanged(0, this._visibleElements.length - oldRowCount);
    this._tree.invalidate();
  },

  sortingChanged: function PTV__sortingChanged(aSortingMode) {
    if (!this._tree || !this._result)
      return;

    var columns = this._tree.columns;

    
    var sortedColumn = columns.getSortedColumn();
    if (sortedColumn)
      sortedColumn.element.removeAttribute("sortDirection");

    
    if (aSortingMode == Ci.nsINavHistoryQueryOptions.SORT_BY_NONE)
      return;
    var [desiredColumn, desiredIsDescending] =
      this._sortTypeToColumnType(aSortingMode);
    var colCount = columns.count;
    for (var i = 0; i < colCount; i ++) {
      var column = columns.getColumnAt(i);
      if (this._getColumnType(column) == desiredColumn) {
        
        if (desiredIsDescending)
          column.element.setAttribute("sortDirection", "descending");
        else
          column.element.setAttribute("sortDirection", "ascending");
        break;
      }
    }
  },

  get result() {
    return this._result;
  },

  set result(val) {
    this._result = val;
    this._finishInit();
    return val;
  },

  addViewObserver: function PTV_addViewObserver(aObserver, aWeak) {
    if (aWeak)
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    if (this._observers.indexOf(aObserver) == -1)
      this._observers.push(aObserver);
  },

  removeViewObserver: function PTV_removeViewObserver(aObserver) {
    var index = this._observers.indexOf(aObserver);
    if (index != -1)
      this._observers.splice(index, 1);
  },

  _enumerateObservers: function PTV__enumerateObservers(aFunctionName, aArgs) {
    for (var i=0; i < this._observers.length; i++) {
      
      try {
        var obs = this._observers[i];
        obs[aFunctionName].apply(obs, aArgs);
      }
      catch (ex) { Components.reportError(ex); }
    }
  },

  
  get collapseDuplicates() {
    return this._collapseDuplicates;
  },

  set collapseDuplicates(val) {
    if (this._collapseDuplicates == val)
      return val; 

    this._collapseDuplicates = val;
    if (this._tree && this._result)
      this.invalidateAll();

    return val;
  },

  get flatItemCount() {
    return this._visibleElements.length;
  },

  nodeForTreeIndex: function PTV_nodeForTreeIndex(aIndex) {
    if (aIndex > this._visibleElements.length)
      throw Cr.NS_ERROR_INVALID_ARG;

    return this._visibleElements[aIndex];
  },

  treeIndexForNode: function PTV_treeNodeForIndex(aNode) {
    var viewIndex = aNode.viewIndex;
    if (viewIndex < 0)
      return Ci.nsINavHistoryResultTreeViewer.INDEX_INVISIBLE;

    NS_ASSERT(this._visibleElements[viewIndex] == aNode,
              "Node's visible index and array out of sync");
    return viewIndex;
  },

  
  get rowCount() {
    return this._visibleElements.length;
  },

  get selection() {
    return this._selection;
  },

  set selection(val) {
    return this._selection = val;
  },

  getRowProperties: function PTV_getRowProperties(aRow, aProperties) {
    this._ensureValidRow(aRow);
    var node = this._visibleElements[aRow];

    
    if (!this._showSessions)
      return;

    switch(this._getRowSessionStatus(aRow)) {
      case this.SESSION_STATUS_NONE:
        break;
      case this.SESSION_STATUS_START:
        aProperties.AppendElement(this._sessionStartAtom);
        break;
      case this.SESSION_STATUS_CONTINUE:
        aProperties.AppendElement(this._sessionContinueAtom);
        break
    }
  },

  getCellProperties: function PTV_getCellProperties(aRow, aColumn, aProperties) {
    if (aColumn.id != "title")
      return;

    this._ensureValidRow(aRow);
    var node = this._visibleElements[aRow];

    if (PlacesUtils.nodeIsSeparator(node))
      aProperties.AppendElement(this._separatorAtom);
  },

  getColumnProperties: function(aColumn, aProperties) { },

  isContainer: function PTV_isContainer(aRow) {
    this._ensureValidRow(aRow);
    var node = this._visibleElements[aRow];
    if (PlacesUtils.nodeIsContainer(node)) {
      
      if (PlacesUtils.nodeIsQuery(node)) {
        asQuery(node);
        if (node.queryOptions.expandQueries)
          return true;
        
        if (!node.parent)
          return true;

        return false;
      }
      return true;
    }
    return false;
  },

  isContainerOpen: function PTV_isContainerOpen(aRow) {
    this._ensureValidRow(aRow);
    if (!PlacesUtils.nodeIsContainer(this._visibleElements[aRow]))
      throw Cr.NS_ERROR_INVALID_ARG;

    return asContainer(this._visibleElements[aRow]).containerOpen;
  },

  isContainerEmpty: function PTV_isContainerEmpty(aRow) {
    this._ensureValidRow(aRow);
    if (!PlacesUtils.nodeIsContainer(this._visibleElements[aRow]))
      throw Cr.NS_ERROR_INVALID_ARG;

    return !asContainer(this._visibleElements[aRow]).hasChildren;
  },

  isSeparator: function PTV_isSeparator(aRow) {
    this._ensureValidRow(aRow);
    return PlacesUtils.nodeIsSeparator(this._visibleElements[aRow]);
  },

  isSorted: function PTV_isSorted() {
    return this._result.sortingMode !=
           Components.interfaces.nsINavHistoryQueryOptions.SORT_BY_NONE;
  },

  canDrop: function PTV_canDrop(aRow, aOrientation) {
    for (var i=0; i < this._observers.length; i++) {
      if (this._observers[i].canDrop(aRow, aOrientation))
        return true;
    }
    return false;
  },

  drop: function PTV_drop(aRow, aOrientation) {
    this._enumerateObservers("onDrop", [aRow, aOrientation]);
  },

  getParentIndex: function PTV_getParentIndex(aRow) {
    this._ensureValidRow(aRow);
    var parent = this._visibleElements[aRow].parent;
    if (!parent || parent.viewIndex < 0)
      return -1;

    return parent.viewIndex;
  },

  hasNextSibling: function PTV_hasNextSibling(aRow, aAfterIndex) {
    this._ensureValidRow(aRow);
    if (aRow == this._visibleElements.length -1) {
      
      return false;
    }

    return this._visibleElements[aRow].indentLevel ==
           this._visibleElements[aRow + 1].indentLevel;
  },

  getLevel: function PTV_getLevel(aRow) {
    this._ensureValidRow(aRow);

    
    
    
    
    
    if (this._showRoot)
      return this._visibleElements[aRow].indentLevel + 1;

    return this._visibleElements[aRow].indentLevel;
  },

  getImageSrc: function PTV_getImageSrc(aRow, aColumn) {
    this._ensureValidRow(aRow);

    
    if (aColumn.index != 0)
      return "";

    var node = this._visibleElements[aRow];

    
    
    
    if (PlacesUtils.nodeIsSeparator(node) ||
        (PlacesUtils.nodeIsContainer(node) && !node.icon))
      return "";

    
    
    var icon = node.icon || PlacesUtils.favicons.defaultFavicon;
    return icon ? icon.spec : "";
  },

  getProgressMode: function(aRow, aColumn) { },
  getCellValue: function(aRow, aColumn) { },

  getCellText: function PTV_getCellText(aRow, aColumn) {
    this._ensureValidRow(aRow);

    var node = this._visibleElements[aRow];
    switch (this._getColumnType(aColumn)) {
      case this.COLUMN_TYPE_TITLE:
        
        
        
        
        if (PlacesUtils.nodeIsSeparator(node))
          return "";
        return node.title || PlacesUtils.getString("noTitle");
      case this.COLUMN_TYPE_URI:
        if (PlacesUtils.nodeIsURI(node))
          return node.uri;

        return "";
      case this.COLUMN_TYPE_DATE:
        if (node.time == 0 || !PlacesUtils.nodeIsURI(node)) {
          
          
          
          
          
          return "";
        }
        if (this._getRowSessionStatus(aRow) != this.SESSION_STATUS_CONTINUE) {
          var nodeTime = node.time / 1000; 
          var nodeTimeObj = new Date(nodeTime);

          
          
          
          
          var ago = new Date(Date.now() - nodeTime);
          var dateFormat = Ci.nsIScriptableDateFormat.dateFormatShort;
          if (ago > -10000 && ago < (1000 * 24 * 60 * 60)) {
            var midnight = new Date(nodeTime);
            midnight.setHours(0);
            midnight.setMinutes(0);
            midnight.setSeconds(0);
            midnight.setMilliseconds(0);

            if (nodeTime > midnight.getTime())
              dateFormat = Ci.nsIScriptableDateFormat.dateFormatNone;
          }

          return (this._dateService.FormatDateTime("", dateFormat,
            Ci.nsIScriptableDateFormat.timeFormatNoSeconds,
            nodeTimeObj.getFullYear(), nodeTimeObj.getMonth() + 1,
            nodeTimeObj.getDate(), nodeTimeObj.getHours(),
            nodeTimeObj.getMinutes(), nodeTimeObj.getSeconds()));
        }
        return "";
      case this.COLUMN_TYPE_VISITCOUNT:
        return node.accessCount;
    }
    return "";
  },

  setTree: function PTV_setTree(aTree) {
    var hasOldTree = this._tree != null;
    this._tree = aTree;

    
    
    
    this._finishInit();

    if (!aTree && hasOldTree && this._result) {
      
      
      this._result.viewer = null;
    }
  },

  toggleOpenState: function PTV_toggleOpenState(aRow) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;
    this._ensureValidRow(aRow);

    this._enumerateObservers("onToggleOpenState", [aRow]);

    var node = this._visibleElements[aRow];
    if (!PlacesUtils.nodeIsContainer(node))
      return; 

    asContainer(node);
    node.containerOpen = !node.containerOpen;
  },

  cycleHeader: function PTV_cycleHeader(aColumn) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    this._enumerateObservers("onCycleHeader", [aColumn]);

    
    
    
    
    
    
    
    
    
    
    
    var allowTriState = PlacesUtils.nodeIsFolder(this._result.root);

    var oldSort = this._result.sortingMode;
    var newSort;
    const NHQO = Ci.nsINavHistoryQueryOptions;
    switch (this._getColumnType(aColumn)) {
      case this.COLUMN_TYPE_TITLE:
        if (oldSort == NHQO.SORT_BY_TITLE_ASCENDING)
          newSort = NHQO.SORT_BY_TITLE_DESCENDING;
        else if (allowTriState && oldSort == NHQO.SORT_BY_TITLE_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_TITLE_ASCENDING;

        break;
      case this.COLUMN_TYPE_URI:
        if (oldSort == NHQO.SORT_BY_URI_ASCENDING)
          newSort = NHQO.SORT_BY_URI_DESCENDING;
        else if (allowTriState && oldSort == NHQO.SORT_BY_URI_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_URI_ASCENDING;

        break;
      case this.COLUMN_TYPE_DATE:
        if (oldSort == NHQO.SORT_BY_DATE_ASCENDING)
          newSort = NHQO.SORT_BY_DATE_DESCENDING;
        else if (allowTriState &&
                 oldSort == NHQO.SORT_BY_DATE_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_DATE_ASCENDING;

        break;
      case this.COLUMN_TYPE_VISITCOUNT:
        
        
        
        if (oldSort == NHQO.SORT_BY_VISITCOUNT_DESCENDING)
          newSort = NHQO.SORT_BY_VISITCOUNT_ASCENDING;
        else if (allowTriState && oldSort == NHQO.SORT_BY_VISITCOUNT_ASCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_VISITCOUNT_DESCENDING;

        break;
      default:
        throw Cr.NS_ERROR_INVALID_ARG;
    }
    this._result.sortingMode = newSort;
  },

  selectionChanged: function PTV_selectionChnaged() {
    this._enumerateObservers("onSelectionChanged");
  },

  cycleCell: function PTV_cycleCell(aRow, aColumn) {
    this._enumerateObservers("onCycleCell", [aRow, aColumn]);
  },

  isEditable: function(aRow, aColumn) { return false; },
  isSelectable: function(aRow, aColumn) { return false; },
  setCellText: function(aRow, aColumn) { },

  performAction: function PTV_performAction(aAction) {
    this._enumerateObservers("onPerformAction", [aAction]);
  },

  performActionOnRow: function PTV_perfromActionOnRow(aAction, aRow) {
    this._enumerateObservers("onPerformActionOnRow", [aAction, aRow]);
  },

  performActionOnCell:
  function PTV_performActionOnCell(aAction, aRow, aColumn) {
    this._enumerateObservers("onPerformActionOnRow", [aAction, aRow, aColumn]);
  }
};

function PlacesTreeView(aShowRoot) {
  this._tree = null;
  this._result = null;
  this._collapseDuplicates = true;
  this._showSessions = false;
  this._selection = null;
  this._visibleElements = [];
  this._observers = [];
  this._showRoot = aShowRoot;
}
