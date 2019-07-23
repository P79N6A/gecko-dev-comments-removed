






































PlacesTreeView.prototype = {
  _makeAtom: function PTV__makeAtom(aString) {
    return  Cc["@mozilla.org/atom-service;1"].
            getService(Ci.nsIAtomService).
            getAtom(aString);
  },

  _atoms: [],
  _getAtomFor: function PTV__getAtomFor(aName) {
    if (!this._atoms[aName])
      this._atoms[aName] = this._makeAtom(aName);

    return this._atoms[aName];
  },

  _ensureValidRow: function PTV__ensureValidRow(aRow) {
    if (aRow < 0 || aRow >= this._visibleElements.length)
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
  function PTV__buildVisibleSection(aContainer, aVisible, aToOpen, aVisibleStartIndex)
  {
    if (!aContainer.containerOpen)
      return;  

    const openLiteral = PlacesUtils.RDF.GetResource("http://home.netscape.com/NC-rdf#open");
    const trueLiteral = PlacesUtils.RDF.GetLiteral("true");

    var cc = aContainer.childCount;
    for (var i=0; i < cc; i++) {
      var curChild = aContainer.getChild(i);
      var curChildType = curChild.type;

      
      if (this._collapseDuplicates) {
        var showThis = { value: false };
        while (i <  cc - 1 &&
               this._canCollapseDuplicates(curChild, aContainer.getChild(i+1),
                                           showThis)) {
          if (showThis.value) {
            
            curChild.viewIndex = -1;
            curChild = aContainer.getChild(i+1);
            curChildType = curChild.type;
          }
          else {
            
            aContainer.getChild(i+1).viewIndex = -1;
          }
          i++;
        }
      }

      
      if (curChildType == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
        if (this._result.sortingMode !=
            Ci.nsINavHistoryQueryOptions.SORT_BY_NONE) {
          curChild.viewIndex = -1;
          continue;
        }
      }

      
      curChild.viewIndex = aVisibleStartIndex + aVisible.length;
      aVisible.push(curChild);

      
      if (!this._flatList && PlacesUtils.containerTypes.indexOf(curChildType) != -1) {
        asContainer(curChild);

        var resource = this._getResourceForNode(curChild);
        var isopen = resource != null &&
                     PlacesUtils.localStore.HasAssertion(resource, openLiteral,
                                                         trueLiteral, true);
        if (isopen != curChild.containerOpen)
          aToOpen.push(curChild);
        else if (curChild.containerOpen && curChild.childCount > 0)
          this._buildVisibleSection(curChild, aVisible, aToOpen, aVisibleStartIndex);
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
      if (aContainer.viewIndex < 0 ||
          aContainer.viewIndex > this._visibleElements.length)
        throw "Trying to expand a node that is not visible";

      NS_ASSERT(this._visibleElements[aContainer.viewIndex] == aContainer,
                "Visible index is out of sync!");
    }

    var startReplacement = aContainer.viewIndex + 1;
    var replaceCount = this._countVisibleRowsForItem(aContainer);

    
    
    
    
    if (aContainer.viewIndex != -1)
      replaceCount-=1;

    
    var previouslySelectedNodes = [];
    var selection = this.selection;
    var rc = selection.getRangeCount();
    for (var rangeIndex = 0; rangeIndex < rc; rangeIndex++) {
      var min = { }, max = { };
      selection.getRangeAt(rangeIndex, min, max);
      var lastIndex = Math.min(max.value, startReplacement + replaceCount -1);
      if (min.value < startReplacement || min.value > lastIndex)
        continue;

      for (var nodeIndex = min.value; nodeIndex <= lastIndex; nodeIndex++)
        previouslySelectedNodes.push({ node: this._visibleElements[nodeIndex],
                                       oldIndex: nodeIndex });
    }

    
    for (var i = 0; i < replaceCount; i++)
      this._visibleElements[startReplacement + i].viewIndex = -1;

    
    var newElements = [];
    var toOpenElements = [];
    this._buildVisibleSection(aContainer, newElements, toOpenElements, startReplacement);

    
    this._visibleElements =
      this._visibleElements.slice(0, startReplacement).concat(newElements)
          .concat(this._visibleElements.slice(startReplacement + replaceCount,
                                              this._visibleElements.length));

    
    
    if (replaceCount != newElements.length) {
      for (i = startReplacement + newElements.length;
           i < this._visibleElements.length; i ++) {
        this._visibleElements[i].viewIndex = i;
      }
    }

    
    if (previouslySelectedNodes.length > 0)
      selection.selectEventsSuppressed = true;

    this._tree.beginUpdateBatch();
    if (replaceCount)
      this._tree.rowCountChanged(startReplacement, -replaceCount);
    if (newElements.length)
      this._tree.rowCountChanged(startReplacement, newElements.length);

    
    for (var i = 0; i < toOpenElements.length; i++) {
      var item = toOpenElements[i];
      var parent = item.parent;
      
      while (parent) {
        if (parent.uri == item.uri)
          break;
        parent = parent.parent;
      }
      
      
      if (!parent)
        item.containerOpen = !item.containerOpen;
    }

    this._tree.endUpdateBatch();

    
    if (previouslySelectedNodes.length > 0) {
      for (var i = 0; i < previouslySelectedNodes.length; i++) {
        var nodeInfo = previouslySelectedNodes[i];
        var index = nodeInfo.node.viewIndex;

        
        
        if (index == -1) { 
          var itemId = PlacesUtils.getConcreteItemId(nodeInfo.node);
          if (itemId != 1) { 
            for (var j = 0; j < newElements.length && index == -1; j++) {
              if (PlacesUtils.getConcreteItemId(newElements[j]) == itemId)
                index = newElements[j].viewIndex;
            }
          }
          else { 
            var uri = nodeInfo.node.uri;
            if (uri) {
              for (var j = 0; j < newElements.length && index == -1; j++) {
                if (newElements[j].uri == uri)
                  index = newElements[j].viewIndex;
              }
            }
          }
        }
        if (index != -1)
          selection.rangedSelect(index, index, true);
      }

      
      
      if (previouslySelectedNodes.length == 1 &&
          selection.getRangeCount() == 0 &&
          this._visibleElements.length > previouslySelectedNodes[0].oldIndex) {
        selection.rangedSelect(previouslySelectedNodes[0].oldIndex,
                               previouslySelectedNodes[0].oldIndex, true);
      }

      selection.selectEventsSuppressed = false;
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

  _convertPRTimeToString: function PTV__convertPRTimeToString(aTime) {
    var timeInMilliseconds = aTime / 1000; 
    var timeObj = new Date(timeInMilliseconds);

    
    
    
    
    var ago = new Date(Date.now() - timeInMilliseconds);
    var dateFormat = Ci.nsIScriptableDateFormat.dateFormatShort;
    if (ago > -10000 && ago < (1000 * 24 * 60 * 60)) {
      var midnight = new Date(timeInMilliseconds);
      midnight.setHours(0);
      midnight.setMinutes(0);
      midnight.setSeconds(0);
      midnight.setMilliseconds(0);

      if (timeInMilliseconds > midnight.getTime())
        dateFormat = Ci.nsIScriptableDateFormat.dateFormatNone;
    }

    return (this._dateService.FormatDateTime("", dateFormat,
      Ci.nsIScriptableDateFormat.timeFormatNoSeconds,
      timeObj.getFullYear(), timeObj.getMonth() + 1,
      timeObj.getDate(), timeObj.getHours(),
      timeObj.getMinutes(), timeObj.getSeconds()));
  },

  COLUMN_TYPE_UNKNOWN: 0,
  COLUMN_TYPE_TITLE: 1,
  COLUMN_TYPE_URI: 2,
  COLUMN_TYPE_DATE: 3,
  COLUMN_TYPE_VISITCOUNT: 4,
  COLUMN_TYPE_KEYWORD: 5,
  COLUMN_TYPE_DESCRIPTION: 6,
  COLUMN_TYPE_DATEADDED: 7,
  COLUMN_TYPE_LASTMODIFIED: 8,
  COLUMN_TYPE_TAGS: 9,

  _getColumnType: function PTV__getColumnType(aColumn) {
    var columnType = aColumn.id || aColumn.element.getAttribute("anonid");
    switch (columnType) {
      case "title":
        return this.COLUMN_TYPE_TITLE;
      case "url":
        return this.COLUMN_TYPE_URI;
      case "date":
        return this.COLUMN_TYPE_DATE;
      case "visitCount":
        return this.COLUMN_TYPE_VISITCOUNT;
      case "keyword":
        return this.COLUMN_TYPE_KEYWORD;
      case "description":
        return this.COLUMN_TYPE_DESCRIPTION;
      case "dateAdded":
        return this.COLUMN_TYPE_DATEADDED;
      case "lastModified":
        return this.COLUMN_TYPE_LASTMODIFIED;
      case "tags":
        return this.COLUMN_TYPE_TAGS;
    }
    return this.COLUMN_TYPE_UNKNOWN;
  },

  _sortTypeToColumnType: function PTV__sortTypeToColumnType(aSortType) {
    switch (aSortType) {
      case Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING:
        return [this.COLUMN_TYPE_TITLE, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_DESCENDING:
        return [this.COLUMN_TYPE_TITLE, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING:
        return [this.COLUMN_TYPE_DATE, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING:
        return [this.COLUMN_TYPE_DATE, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_URI_ASCENDING:
        return [this.COLUMN_TYPE_URI, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_URI_DESCENDING:
        return [this.COLUMN_TYPE_URI, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_ASCENDING:
        return [this.COLUMN_TYPE_VISITCOUNT, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING:
        return [this.COLUMN_TYPE_VISITCOUNT, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_KEYWORD_ASCENDING:
        return [this.COLUMN_TYPE_KEYWORD, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_KEYWORD_DESCENDING:
        return [this.COLUMN_TYPE_KEYWORD, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_ASCENDING:
        if (this._result.sortingAnnotation == DESCRIPTION_ANNO)
          return [this.COLUMN_TYPE_DESCRIPTION, false];
        break;
      case Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING:
        if (this._result.sortingAnnotation == DESCRIPTION_ANNO)
          return [this.COLUMN_TYPE_DESCRIPTION, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_ASCENDING:
        return [this.COLUMN_TYPE_DATEADDED, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING:
        return [this.COLUMN_TYPE_DATEADDED, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_ASCENDING:
        return [this.COLUMN_TYPE_LASTMODIFIED, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_DESCENDING:
        return [this.COLUMN_TYPE_LASTMODIFIED, true];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_TAGS_ASCENDING:
        return [this.COLUMN_TYPE_TAGS, false];
      case Ci.nsINavHistoryQueryOptions.SORT_BY_TAGS_DESCENDING:
        return [this.COLUMN_TYPE_TAGS, true];
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
        
        
        
        var prevChild = aParent.getChild(aNewIndex - 1);
        newViewIndex = prevChild.viewIndex + this._countVisibleRowsForItem(prevChild);
        
        if (prevChild.parent == aItem.parent && 
            aItem.viewIndex != -1 && 
            prevChild.viewIndex > aItem.viewIndex)
          newViewIndex--;
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

  
  
  _fixViewIndexOnRemove: function PTV_fixViewIndexOnRemove(aItem, aParent) {
    var oldViewIndex = aItem.viewIndex;
    
    var count = this._countVisibleRowsForItem(aItem);

    
    
    
    while (true) {
      if (oldViewIndex > this._visibleElements.length)
        throw("Trying to remove an item with an invalid viewIndex");

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

    return;
  },

  








  itemRemoved: function PTV_itemRemoved(aParent, aItem, aOldIndex) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree)
      return; 

    var oldViewIndex = aItem.viewIndex;
    if (oldViewIndex < 0)
      return; 

    
    
    var selectNext = false;
    var selection = this.selection;
    if (selection.getRangeCount() == 1) {
      var min = { }, max = { };
      selection.getRangeAt(0, min, max);
      if (min.value == max.value &&
          this.nodeForTreeIndex(min.value) == aItem)
        selectNext = true;
    }

    
    this._fixViewIndexOnRemove(aItem, aParent);

    
    if (selectNext && this._visibleElements.length > oldViewIndex)
      selection.rangedSelect(oldViewIndex, oldViewIndex, true);
  },

  



  itemMoved:
  function PTV_itemMoved(aItem, aOldParent, aOldIndex, aNewParent, aNewIndex) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree)
      return; 

    var oldViewIndex = aItem.viewIndex;
    if (oldViewIndex < 0)
      return; 

    
    var count = this._countVisibleRowsForItem(aItem);

    
    var nodesToSelect = [];
    var selection = this.selection;
    var rc = selection.getRangeCount();
    for (var rangeIndex = 0; rangeIndex < rc; rangeIndex++) {
      var min = { }, max = { };
      selection.getRangeAt(rangeIndex, min, max);
      var lastIndex = Math.min(max.value, oldViewIndex + count -1);
      if (min.value < oldViewIndex || min.value > lastIndex)
        continue;

      for (var nodeIndex = min.value; nodeIndex <= lastIndex; nodeIndex++)
        nodesToSelect.push(this._visibleElements[nodeIndex]);
    }
    if (nodesToSelect.length > 0)
      selection.selectEventsSuppressed = true;

    
    this._fixViewIndexOnRemove(aItem, aOldParent);

    
    this.itemInserted(aNewParent, aItem, aNewIndex);

    
    if (nodesToSelect.length > 0) {
      for (var i = 0; i < nodesToSelect.length; i++) {
        var node = nodesToSelect[i];
        var index = node.viewIndex;
        selection.rangedSelect(index, index, true);
      }
      selection.selectEventsSuppressed = false;
    }
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
  },

  sortingChanged: function PTV__sortingChanged(aSortingMode) {
    if (!this._tree || !this._result)
      return;

    
    window.updateCommands("sort");

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
    if (this._result != val) {
      this._result = val;
      this._finishInit();
    }
    return val;
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

  _getResourceForNode: function PTV_getResourceForNode(aNode)
  {
    var uri = aNode.uri;
    NS_ASSERT(uri, "if there is no uri, we can't persist the open state");
    return uri ? PlacesUtils.RDF.GetResource(uri) : null;
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

    
    if (!this._showSessions)
      return;

    var status = this._getRowSessionStatus(aRow);
    switch (status) {
      case this.SESSION_STATUS_NONE:
        break;
      case this.SESSION_STATUS_START:
        aProperties.AppendElement(this._getAtomFor("session-start"));
        break;
      case this.SESSION_STATUS_CONTINUE:
        aProperties.AppendElement(this._getAtomFor("session-continue"));
        break
    }
  },

  getCellProperties: function PTV_getCellProperties(aRow, aColumn, aProperties) {
    this._ensureValidRow(aRow);

    
    var columnType = aColumn.element.getAttribute("anonid");
    if (columnType)
      aProperties.AppendElement(this._getAtomFor(columnType));
    else
      var columnType = aColumn.id;

    if (columnType != "title")
      return;

    var node = this._visibleElements[aRow];

    var nodeType = node.type;
    if (PlacesUtils.containerTypes.indexOf(nodeType) != -1) {
      
      
      
      if (this._flatList)
        aProperties.AppendElement(this._getAtomFor("container"));
      if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY)
        aProperties.AppendElement(this._getAtomFor("query"));
      else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER ||
               nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT) {
        if (PlacesUtils.annotations.itemHasAnnotation(node.itemId,
                                                      LMANNO_FEEDURI))
          aProperties.AppendElement(this._getAtomFor("livemark"));
        else if (PlacesUtils.bookmarks.getFolderIdForItem(node.itemId) ==
                 PlacesUtils.tagsFolderId)
          aProperties.AppendElement(this._getAtomFor("tagContainer"));
      }
    }
    else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR)
      aProperties.AppendElement(this._getAtomFor("separator"));
  },

  getColumnProperties: function(aColumn, aProperties) { },

  isContainer: function PTV_isContainer(aRow) {
    this._ensureValidRow(aRow);
    if (this._flatList)
      return false; 

    var node = this._visibleElements[aRow];
    if (PlacesUtils.nodeIsContainer(node)) {
      
      if (!node.parent)
        return true;

      
      if (PlacesUtils.nodeIsQuery(node)) {
        asQuery(node);
        return node.queryOptions.expandQueries;
      }
      return true;
    }
    return false;
  },

  isContainerOpen: function PTV_isContainerOpen(aRow) {
    if (this._flatList)
      return false;

    this._ensureValidRow(aRow);
    if (!PlacesUtils.nodeIsContainer(this._visibleElements[aRow]))
      throw Cr.NS_ERROR_INVALID_ARG;

    return this._visibleElements[aRow].containerOpen;
  },

  isContainerEmpty: function PTV_isContainerEmpty(aRow) {
    if (this._flatList)
      return true;

    this._ensureValidRow(aRow);

    if (!PlacesUtils.nodeIsContainer(this._visibleElements[aRow]))
      throw Cr.NS_ERROR_INVALID_ARG;

    return !this._visibleElements[aRow].hasChildren;
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
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    var node = aRow != -1 ? this.nodeForTreeIndex(aRow) : this._result.root;

    if (aOrientation == Ci.nsITreeView.DROP_ON) {
      
      var dragService =  Cc["@mozilla.org/widget/dragservice;1"].
                         getService(Ci.nsIDragService);
      var dragSession = dragService.getCurrentSession();
      var elt = dragSession.sourceNode.parentNode;
      if (elt.localName == "tree" && elt.view == this &&
          this.selection.isSelected(aRow))
        return false;
      if (node.parent && PlacesUtils.nodeIsReadOnly(node.parent))
        return false;
    }
    return PlacesControllerDragHelper.canDrop(this, aOrientation);
  },

  
  
  
  _disallowInsertion: function PTV__disallowInsertion(aContainer) {
    
    return (!PlacesUtils.nodeIsFolder(aContainer) ||
            PlacesUtils.nodeIsReadOnly(aContainer));
  },

  _getInsertionPoint: function PTV__getInsertionPoint(index, orientation) {
    var container = this._result.root;
    
    
    if (index != -1) {
      var lastSelected = this.nodeForTreeIndex(index);
      if (this.isContainer(index) && orientation == Ci.nsITreeView.DROP_ON) {
        
        
        container = lastSelected;
        index = -1;
      }
      else if (!this._disallowInsertion(lastSelected) &&
               lastSelected.containerOpen &&
               orientation == Ci.nsITreeView.DROP_AFTER) {
        
        
        container = lastSelected;
        orientation = Ci.nsITreeView.DROP_BEFORE;
        index = 0;
      }
      else {
        
        
        
        container = lastSelected.parent || container;

        
        
        if (this._disallowInsertion(container))
          return null;

        var lsi = PlacesUtils.getIndexOfNode(lastSelected);
        index = orientation == Ci.nsITreeView.DROP_BEFORE ? lsi : lsi + 1;
      }
    }

    if (this._disallowInsertion(container))
      return null;

    return new InsertionPoint(PlacesUtils.getConcreteItemId(container),
                              index, orientation);
  },

  drop: function PTV_drop(aRow, aOrientation) {
    
    
    
    var ip = this._getInsertionPoint(aRow, aOrientation);
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    PlacesControllerDragHelper.onDrop(ip);
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

    var thisLevel = this._visibleElements[aRow].indentLevel;
    for (var i = aAfterIndex + 1; i < this._visibleElements.length; ++i) {
      var nextLevel = this._visibleElements[i].indentLevel;
      if (nextLevel == thisLevel)
        return true;
      if (nextLevel < thisLevel)
        break;
    }
    return false;
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
    var columnType = this._getColumnType(aColumn);
    switch (columnType) {
      case this.COLUMN_TYPE_TITLE:
        
        
        
        
        if (PlacesUtils.nodeIsSeparator(node))
          return "";
        return node.title || PlacesUtils.getString("noTitle");
      case this.COLUMN_TYPE_TAGS:
        return node.tags;
      case this.COLUMN_TYPE_URI:
        if (PlacesUtils.nodeIsURI(node))
          return node.uri;
        return "";
      case this.COLUMN_TYPE_DATE:
        if (node.time == 0 || !PlacesUtils.nodeIsURI(node)) {
          
          
          
          
          
          return "";
        }
        if (this._getRowSessionStatus(aRow) != this.SESSION_STATUS_CONTINUE)
          return this._convertPRTimeToString(node.time);
        return "";
      case this.COLUMN_TYPE_VISITCOUNT:
        return node.accessCount;
      case this.COLUMN_TYPE_KEYWORD:
        if (PlacesUtils.nodeIsBookmark(node))
          return PlacesUtils.bookmarks.getKeywordForBookmark(node.itemId);
        return "";
      case this.COLUMN_TYPE_DESCRIPTION:
        const annos = PlacesUtils.annotations;
        if (annos.itemHasAnnotation(node.itemId, DESCRIPTION_ANNO))
          return annos.getItemAnnotation(node.itemId, DESCRIPTION_ANNO)
        return "";
      case this.COLUMN_TYPE_DATEADDED:
        if (node.dateAdded)
          return this._convertPRTimeToString(node.dateAdded);
        return "";
      case this.COLUMN_TYPE_LASTMODIFIED:
        if (node.lastModified)
          return this._convertPRTimeToString(node.lastModified);
        return "";
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

    var node = this._visibleElements[aRow];
    if (!PlacesUtils.nodeIsContainer(node))
      return; 

    var resource = this._getResourceForNode(node);
    if (resource) {
      const openLiteral = PlacesUtils.RDF.GetResource("http://home.netscape.com/NC-rdf#open");
      const trueLiteral = PlacesUtils.RDF.GetLiteral("true");

      if (node.containerOpen)
        PlacesUtils.localStore.Unassert(resource, openLiteral, trueLiteral);
      else
        PlacesUtils.localStore.Assert(resource, openLiteral, trueLiteral, true);
    }

    node.containerOpen = !node.containerOpen;
  },

  cycleHeader: function PTV_cycleHeader(aColumn) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    
    
    
    
    
    
    
    
    
    
    
    var allowTriState = PlacesUtils.nodeIsFolder(this._result.root);

    var oldSort = this._result.sortingMode;
    var oldSortingAnnotation = this._result.sortingAnnotation;
    var newSort;
    var newSortingAnnotation = "";
    const NHQO = Ci.nsINavHistoryQueryOptions;
    var columnType = this._getColumnType(aColumn);
    switch (columnType) {
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
      case this.COLUMN_TYPE_KEYWORD:
        if (oldSort == NHQO.SORT_BY_KEYWORD_ASCENDING)
          newSort = NHQO.SORT_BY_KEYWORD_DESCENDING;
        else if (allowTriState && oldSort == NHQO.SORT_BY_KEYWORD_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_KEYWORD_ASCENDING;

        break;
      case this.COLUMN_TYPE_DESCRIPTION:
        if (oldSort == NHQO.SORT_BY_ANNOTATION_ASCENDING &&
            oldSortingAnnotation == DESCRIPTION_ANNO) {
          newSort = NHQO.SORT_BY_ANNOTATION_DESCENDING;
          newSortingAnnotation = DESCRIPTION_ANNO;
        }
        else if (allowTriState &&
                 oldSort == NHQO.SORT_BY_ANNOTATION_DESCENDING &&
                 oldSortingAnnotation == DESCRIPTION_ANNO)
          newSort = NHQO.SORT_BY_NONE;
        else {
          newSort = NHQO.SORT_BY_ANNOTATION_ASCENDING;
          newSortingAnnotation = DESCRIPTION_ANNO;
        }

        break;
      case this.COLUMN_TYPE_DATEADDED:
        if (oldSort == NHQO.SORT_BY_DATEADDED_ASCENDING)
          newSort = NHQO.SORT_BY_DATEADDED_DESCENDING;
        else if (allowTriState &&
                 oldSort == NHQO.SORT_BY_DATEADDED_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_DATEADDED_ASCENDING;

        break;
      case this.COLUMN_TYPE_LASTMODIFIED:
        if (oldSort == NHQO.SORT_BY_LASTMODIFIED_ASCENDING)
          newSort = NHQO.SORT_BY_LASTMODIFIED_DESCENDING;
        else if (allowTriState &&
                 oldSort == NHQO.SORT_BY_LASTMODIFIED_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_LASTMODIFIED_ASCENDING;

        break;
      case this.COLUMN_TYPE_TAGS:
        if (oldSort == NHQO.SORT_BY_TAGS_ASCENDING)
          newSort = NHQO.SORT_BY_TAGS_DESCENDING;
        else if (allowTriState && oldSort == NHQO.SORT_BY_TAGS_DESCENDING)
          newSort = NHQO.SORT_BY_NONE;
        else
          newSort = NHQO.SORT_BY_TAGS_ASCENDING;

        break;
      default:
        throw Cr.NS_ERROR_INVALID_ARG;
    }
    this._result.sortingAnnotation = newSortingAnnotation;
    this._result.sortingMode = newSort;
  },

  selectionChanged: function() { },
  cycleCell: function PTV_cycleCell(aRow, aColumn) { },
  isEditable: function(aRow, aColumn) { return false; },
  isSelectable: function(aRow, aColumn) { return false; },
  setCellText: function(aRow, aColumn) { },
  performAction: function(aAction) { },
  performActionOnRow: function(aAction, aRow) { },
  performActionOnCell: function(aAction, aRow, aColumn) { }
};

function PlacesTreeView(aShowRoot, aFlatList) {
  if (aShowRoot && aFlatList)
    throw("Flat-list mode is not supported when show-root is set");

  this._tree = null;
  this._result = null;
  this._collapseDuplicates = true;
  this._showSessions = false;
  this._selection = null;
  this._visibleElements = [];
  this._showRoot = aShowRoot;
  this._flatList = aFlatList;
}
