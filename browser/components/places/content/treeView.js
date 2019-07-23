






































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

    var qoInt = Ci.nsINavHistoryQueryOptions;
    var options = asQuery(this._result.root).queryOptions;

    
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

    this._showSessions = true;
  },

  SESSION_STATUS_NONE: 0,
  SESSION_STATUS_START: 1,
  SESSION_STATUS_CONTINUE: 2,
  _getRowSessionStatus: function PTV__getRowSessionStatus(aRow) {
    var node = this._visibleElements[aRow].node;
    if (!PlacesUtils.nodeIsVisit(node) || asVisit(node).sessionId == 0)
      return this.SESSION_STATUS_NONE;

    if (aRow == 0)
      return this.SESSION_STATUS_START;

    var previousNode = this._visibleElements[aRow - 1].node;
    if (!PlacesUtils.nodeIsVisit(previousNode) ||
        node.sessionId != asVisit(previousNode).sessionId)
      return this.SESSION_STATUS_START;

    return this.SESSION_STATUS_CONTINUE;
  },

  




  _buildVisibleList: function PTV__buildVisibleList() {
    var selection = this.selection;
    if (selection)
      selection.selectEventsSuppressed = true;

    if (this._result) {
      
      for (var i = 0; i < this._visibleElements.length; i++) {
        this._visibleElements[i].node.viewIndex = -1;
      }
    }

    var rootNode = this._result.root;
    if (rootNode && this._tree) {
      this._computeShowSessions();

      asContainer(rootNode);
      if (this._showRoot) {
        
        this._visibleElements.push(
          { node: this._result.root, properties: null });
        this._tree.rowCountChanged(0, 1);
        this._result.root.viewIndex = 0;
      }
      else if (!rootNode.containerOpen) {
        
        
        rootNode.containerOpen = true;
      }
      else
        this.invalidateContainer(rootNode);
    }
    if (selection)
      selection.selectEventsSuppressed = false;
  },

  











  _buildVisibleSection:
  function PTV__buildVisibleSection(aContainer, aVisible, aToOpen, aVisibleStartIndex)
  {
    if (!aContainer.containerOpen)
      return;  

    const openLiteral = PlacesUIUtils.RDF.GetResource("http://home.netscape.com/NC-rdf#open");
    const trueLiteral = PlacesUIUtils.RDF.GetLiteral("true");

    var cc = aContainer.childCount;
    for (var i=0; i < cc; i++) {
      var curChild = aContainer.getChild(i);
      var curChildType = curChild.type;

      
      if (curChildType == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
        if (this._result.sortingMode !=
            Ci.nsINavHistoryQueryOptions.SORT_BY_NONE) {
          curChild.viewIndex = -1;
          continue;
        }
      }

      
      curChild.viewIndex = aVisibleStartIndex + aVisible.length;
      aVisible.push({ node: curChild, properties: null });

      
      if (!this._flatList && PlacesUtils.containerTypes.indexOf(curChildType) != -1) {
        asContainer(curChild);

        var resource = this._getResourceForNode(curChild);
        var isopen = resource != null &&
                     PlacesUIUtils.localStore.HasAssertion(resource, openLiteral,
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
      if (this._visibleElements[i].node.indentLevel <= outerLevel)
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

      NS_ASSERT(this._visibleElements[aContainer.viewIndex].node == aContainer,
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
      
      
      if (max.value < startReplacement || min.value > lastIndex)
        continue;
      
      
      var firstIndex = Math.max(min.value, startReplacement);
      for (var nodeIndex = firstIndex; nodeIndex <= lastIndex; nodeIndex++)
        previouslySelectedNodes.push(
          { node: this._visibleElements[nodeIndex].node, oldIndex: nodeIndex });
    }

    
    for (var i = 0; i < replaceCount; i++)
      this._visibleElements[startReplacement + i].node.viewIndex = -1;

    
    var newElements = [];
    var toOpenElements = [];
    this._buildVisibleSection(aContainer,
                              newElements, toOpenElements, startReplacement);

    
    this._visibleElements =
      this._visibleElements.slice(0, startReplacement).concat(newElements)
          .concat(this._visibleElements.slice(startReplacement + replaceCount,
                                              this._visibleElements.length));

    
    
    if (replaceCount != newElements.length) {
      for (var i = startReplacement + newElements.length;
           i < this._visibleElements.length; i ++) {
        this._visibleElements[i].node.viewIndex = i;
      }
    }

    
    selection.selectEventsSuppressed = true;
    this._tree.beginUpdateBatch();

    if (replaceCount)
      this._tree.rowCountChanged(startReplacement, -replaceCount);
    if (newElements.length)
      this._tree.rowCountChanged(startReplacement, newElements.length);

    if (!this._flatList) {
      
      for (var i = 0; i < toOpenElements.length; i++) {
        var item = toOpenElements[i];
        var parent = item.parent;
        
        while (parent) {
          if (parent.uri == item.uri)
            break;
          parent = parent.parent;
        }
        
        
        if (!parent && !item.containerOpen)
          item.containerOpen = true;
      }
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
    }
    selection.selectEventsSuppressed = false;
  },

  _convertPRTimeToString: function PTV__convertPRTimeToString(aTime) {
    var timeInMilliseconds = aTime / 1000; 

    
    
    
    
    
    var dateObj = new Date();
    var timeZoneOffsetInMs = dateObj.getTimezoneOffset() * 60000;
    var now = dateObj.getTime() - timeZoneOffsetInMs;
    var midnight = now - (now % (86400000));

    var dateFormat = timeInMilliseconds - timeZoneOffsetInMs >= midnight ?
                      Ci.nsIScriptableDateFormat.dateFormatNone :
                      Ci.nsIScriptableDateFormat.dateFormatShort;

    var timeObj = new Date(timeInMilliseconds);
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
    var columnType = aColumn.element.getAttribute("anonid") || aColumn.id;

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
      }
    }

    aItem.viewIndex = newViewIndex;
    this._visibleElements.splice(newViewIndex, 0, 
                                 { node: aItem, properties: null });
    for (var i = newViewIndex + 1;
         i < this._visibleElements.length; i ++) {
      this._visibleElements[i].node.viewIndex = i;
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

    if (oldViewIndex > this._visibleElements.length)
      throw("Trying to remove an item with an invalid viewIndex");

    this._visibleElements.splice(oldViewIndex, count);
    for (var i = oldViewIndex; i < this._visibleElements.length; i++)
      this._visibleElements[i].node.viewIndex = i;

    this._tree.rowCountChanged(oldViewIndex, -count);

    
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

    
    if (!selectNext)
      return;
    
    if (this._visibleElements.length > oldViewIndex)
      selection.rangedSelect(oldViewIndex, oldViewIndex, true);    
    else if (this._visibleElements.length > 0) {
      
      selection.rangedSelect(this._visibleElements.length - 1,
                             this._visibleElements.length - 1, true);
    }
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
        nodesToSelect.push(this._visibleElements[nodeIndex].node);
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
        viewIndex < this._visibleElements.length) {
      this._visibleElements[viewIndex].node = aNewItem;
      this._visibleElements[viewIndex].properties = null;
    }
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
      if (this._result)
        this._result.root.containerOpen = false;
      this._result = val;
      this._finishInit();
    }
    return val;
  },

  nodeForTreeIndex: function PTV_nodeForTreeIndex(aIndex) {
    if (aIndex > this._visibleElements.length)
      throw Cr.NS_ERROR_INVALID_ARG;

    return this._visibleElements[aIndex].node;
  },

  treeIndexForNode: function PTV_treeNodeForIndex(aNode) {
    var viewIndex = aNode.viewIndex;
    if (viewIndex < 0)
      return Ci.nsINavHistoryResultTreeViewer.INDEX_INVISIBLE;

    NS_ASSERT(this._visibleElements[viewIndex].node == aNode,
              "Node's visible index and array out of sync");
    return viewIndex;
  },

  _getResourceForNode: function PTV_getResourceForNode(aNode)
  {
    var uri = aNode.uri;
    NS_ASSERT(uri, "if there is no uri, we can't persist the open state");
    return uri ? PlacesUIUtils.RDF.GetResource(uri) : null;
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

    
    if (columnType == "url")
      aProperties.AppendElement(this._getAtomFor("ltr"));

    if (columnType != "title")
      return;

    var node = this._visibleElements[aRow].node;
    var properties = this._visibleElements[aRow].properties;

    if (!properties) {
      properties = new Array();
      var itemId = node.itemId;
      var nodeType = node.type;
      if (PlacesUtils.containerTypes.indexOf(nodeType) != -1) {
        if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY) {
          properties.push(this._getAtomFor("query"));
          if (PlacesUtils.nodeIsTagQuery(node))
            properties.push(this._getAtomFor("tagContainer"));
          else if (PlacesUtils.nodeIsDay(node))
            properties.push(this._getAtomFor("dayContainer"));
          else if (PlacesUtils.nodeIsHost(node))
            properties.push(this._getAtomFor("hostContainer"));
        }
        else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER ||
                 nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT) {
          if (PlacesUtils.nodeIsLivemarkContainer(node))
            properties.push(this._getAtomFor("livemark"));
        }

        if (itemId != -1) {
          var queryName = PlacesUIUtils.getLeftPaneQueryNameFromId(itemId);
          if (queryName)
            properties.push(this._getAtomFor("OrganizerQuery_" + queryName));
        }
      }
      else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR)
        properties.push(this._getAtomFor("separator"));
      else if (PlacesUtils.nodeIsURI(node)) {
        properties.push(this._getAtomFor(PlacesUIUtils.guessUrlSchemeForUI(node.uri)));
        if (itemId != -1) {
          if (PlacesUtils.nodeIsLivemarkContainer(node.parent))
            properties.push(this._getAtomFor("livemarkItem"));
        }
      }

      this._visibleElements[aRow].properties = properties;
    }
    for (var i = 0; i < properties.length; i++)
      aProperties.AppendElement(properties[i]);
  },

  getColumnProperties: function(aColumn, aProperties) { },

  isContainer: function PTV_isContainer(aRow) {
    this._ensureValidRow(aRow);

    var node = this._visibleElements[aRow].node;
    if (PlacesUtils.nodeIsContainer(node)) {
      
      if (!node.parent)
        return true;

      
      
      if (this._flatList)
        return true;

      
      if (PlacesUtils.nodeIsQuery(node)) {
        var parent = node.parent;
        if((PlacesUtils.nodeIsQuery(parent) ||
            PlacesUtils.nodeIsFolder(parent)) &&
           !node.hasChildren)
          return asQuery(parent).queryOptions.expandQueries;
      }
      return true;
    }
    return false;
  },

  isContainerOpen: function PTV_isContainerOpen(aRow) {
    if (this._flatList)
      return false;

    this._ensureValidRow(aRow);
    if (!PlacesUtils.nodeIsContainer(this._visibleElements[aRow].node))
      throw Cr.NS_ERROR_INVALID_ARG;

    return this._visibleElements[aRow].node.containerOpen;
  },

  isContainerEmpty: function PTV_isContainerEmpty(aRow) {
    if (this._flatList)
      return true;

    this._ensureValidRow(aRow);

    if (!PlacesUtils.nodeIsContainer(this._visibleElements[aRow].node))
      throw Cr.NS_ERROR_INVALID_ARG;

    return !this._visibleElements[aRow].node.hasChildren;
  },

  isSeparator: function PTV_isSeparator(aRow) {
    this._ensureValidRow(aRow);
    return PlacesUtils.nodeIsSeparator(this._visibleElements[aRow].node);
  },

  isSorted: function PTV_isSorted() {
    return this._result.sortingMode !=
           Components.interfaces.nsINavHistoryQueryOptions.SORT_BY_NONE;
  },

  canDrop: function PTV_canDrop(aRow, aOrientation) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    
    if (this.isSorted())
      return false;

    var ip = this._getInsertionPoint(aRow, aOrientation);
    return ip && PlacesControllerDragHelper.canDrop(ip);
  },

  _getInsertionPoint: function PTV__getInsertionPoint(index, orientation) {
    var container = this._result.root;
    var dropNearItemId = -1;
    
    
    if (index != -1) {
      var lastSelected = this.nodeForTreeIndex(index);
      if (this.isContainer(index) && orientation == Ci.nsITreeView.DROP_ON) {
        
        
        container = lastSelected;
        index = -1;
      }
      else if (lastSelected.containerOpen &&
               orientation == Ci.nsITreeView.DROP_AFTER &&
               lastSelected.hasChildren) {
        
        
        container = lastSelected;
        orientation = Ci.nsITreeView.DROP_ON;
        index = 0;
      }
      else {
        
        
        
        container = lastSelected.parent || container;

        
        
        if (PlacesControllerDragHelper.disallowInsertion(container))
          return null;

        var queryOptions = asQuery(this._result.root).queryOptions;
        if (queryOptions.sortingMode !=
              Ci.nsINavHistoryQueryOptions.SORT_BY_NONE) {
          
          index = -1;
        }
        else if (queryOptions.excludeItems ||
                 queryOptions.excludeQueries ||
                 queryOptions.excludeReadOnlyFolders) {
          
          
          
          index = -1;
          dropNearItemId = lastSelected.itemId;
        }
        else {
          var lsi = PlacesUtils.getIndexOfNode(lastSelected);
          index = orientation == Ci.nsITreeView.DROP_BEFORE ? lsi : lsi + 1;
        }
      }
    }

    if (PlacesControllerDragHelper.disallowInsertion(container))
      return null;

    return new InsertionPoint(PlacesUtils.getConcreteItemId(container),
                              index, orientation,
                              PlacesUtils.nodeIsTagQuery(container),
                              dropNearItemId);
  },

  drop: function PTV_drop(aRow, aOrientation) {
    
    
    
    var ip = this._getInsertionPoint(aRow, aOrientation);
    if (!ip)
      return;
    PlacesControllerDragHelper.onDrop(ip);
  },

  getParentIndex: function PTV_getParentIndex(aRow) {
    this._ensureValidRow(aRow);
    var parent = this._visibleElements[aRow].node.parent;
    if (!parent || parent.viewIndex < 0)
      return -1;

    return parent.viewIndex;
  },

  hasNextSibling: function PTV_hasNextSibling(aRow, aAfterIndex) {
    this._ensureValidRow(aRow);
    if (aRow == this._visibleElements.length -1) {
      
      return false;
    }

    var thisLevel = this._visibleElements[aRow].node.indentLevel;
    for (var i = aAfterIndex + 1; i < this._visibleElements.length; ++i) {
      var nextLevel = this._visibleElements[i].node.indentLevel;
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
      return this._visibleElements[aRow].node.indentLevel + 1;

    return this._visibleElements[aRow].node.indentLevel;
  },

  getImageSrc: function PTV_getImageSrc(aRow, aColumn) {
    this._ensureValidRow(aRow);

    
    if (this._getColumnType(aColumn) != this.COLUMN_TYPE_TITLE)
      return "";

    var node = this._visibleElements[aRow].node;
    var icon = node.icon;
    if (icon)
      return icon.spec;
    return "";
  },

  getProgressMode: function(aRow, aColumn) { },
  getCellValue: function(aRow, aColumn) { },

  getCellText: function PTV_getCellText(aRow, aColumn) {
    this._ensureValidRow(aRow);

    var node = this._visibleElements[aRow].node;
    var columnType = this._getColumnType(aColumn);
    switch (columnType) {
      case this.COLUMN_TYPE_TITLE:
        
        
        
        
        if (PlacesUtils.nodeIsSeparator(node))
          return "";
        return PlacesUIUtils.getBestTitle(node);
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
        if (node.itemId != -1) {
          try {
            return PlacesUtils.annotations.
                               getItemAnnotation(node.itemId, DESCRIPTION_ANNO);
          }
          catch (ex) {  }
        }
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

    var node = this._visibleElements[aRow].node;
    if (!PlacesUtils.nodeIsContainer(node))
      return; 

    if (this._flatList && this._openContainerCallback) {
      this._openContainerCallback(node);
      return;
    }

    var resource = this._getResourceForNode(node);
    if (resource) {
      const openLiteral = PlacesUIUtils.RDF.GetResource("http://home.netscape.com/NC-rdf#open");
      const trueLiteral = PlacesUIUtils.RDF.GetLiteral("true");

      if (node.containerOpen)
        PlacesUIUtils.localStore.Unassert(resource, openLiteral, trueLiteral);
      else
        PlacesUIUtils.localStore.Assert(resource, openLiteral, trueLiteral, true);
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

  isEditable: function PTV_isEditable(aRow, aColumn) {
    
    if (aColumn.index != 0)
      return false;

    var node = this.nodeForTreeIndex(aRow);
    if (!PlacesUtils.nodeIsReadOnly(node) &&
        (PlacesUtils.nodeIsFolder(node) ||
         (PlacesUtils.nodeIsBookmark(node) &&
          !PlacesUtils.nodeIsLivemarkItem(node))))
      return true;

    return false;
  },

  setCellText: function PTV_setCellText(aRow, aColumn, aText) {
    
    var node = this.nodeForTreeIndex(aRow);
    if (node.title != aText) {
      var txn = PlacesUIUtils.ptm.editItemTitle(node.itemId, aText);
      PlacesUIUtils.ptm.doTransaction(txn);
    }
  },

  selectionChanged: function() { },
  cycleCell: function PTV_cycleCell(aRow, aColumn) { },
  isSelectable: function(aRow, aColumn) { return false; },
  performAction: function(aAction) { },
  performActionOnRow: function(aAction, aRow) { },
  performActionOnCell: function(aAction, aRow, aColumn) { }
};

function PlacesTreeView(aShowRoot, aFlatList, aOnOpenFlatContainer) {
  if (aShowRoot && aFlatList)
    throw("Flat-list mode is not supported when show-root is set");

  this._tree = null;
  this._result = null;
  this._showSessions = false;
  this._selection = null;
  this._visibleElements = [];
  this._showRoot = aShowRoot;
  this._flatList = aFlatList;
  this._openContainerCallback = aOnOpenFlatContainer;
}
