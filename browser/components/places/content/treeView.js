



Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');

const PTV_interfaces = [Ci.nsITreeView,
                        Ci.nsINavHistoryResultObserver,
                        Ci.nsINavHistoryResultTreeViewer,
                        Ci.nsISupportsWeakReference];

function PlacesTreeView(aFlatList, aOnOpenFlatContainer, aController) {
  this._tree = null;
  this._result = null;
  this._selection = null;
  this._rootNode = null;
  this._rows = [];
  this._flatList = aFlatList;
  this._openContainerCallback = aOnOpenFlatContainer;
  this._controller = aController;
}

PlacesTreeView.prototype = {
  get wrappedJSObject() this,

  __xulStore: null,
  get _xulStore() {
    if (!this.__xulStore) {
      this.__xulStore = Cc["@mozilla.org/xul/xulstore;1"].getService(Ci.nsIXULStore);
    }
    return this.__xulStore;
  },

  __dateService: null,
  get _dateService() {
    if (!this.__dateService) {
      this.__dateService = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                           getService(Ci.nsIScriptableDateFormat);
    }
    return this.__dateService;
  },

  QueryInterface: XPCOMUtils.generateQI(PTV_interfaces),

  
  
  
  
  
  
  
  
  classInfo: XPCOMUtils.generateCI({ interfaces: PTV_interfaces }),

  


  _finishInit: function PTV__finishInit() {
    let selection = this.selection;
    if (selection)
      selection.selectEventsSuppressed = true;

    if (!this._rootNode.containerOpen) {
      
      
      this._rootNode.containerOpen = true;
    }
    else
      this.invalidateContainer(this._rootNode);

    
    this.sortingChanged(this._result.sortingMode);

    if (selection)
      selection.selectEventsSuppressed = false;
  },

  




















  _isPlainContainer: function PTV__isPlainContainer(aContainer) {
    
    if (this._controller.hasCachedLivemarkInfo(aContainer))
      return true;

    
    if (!(aContainer instanceof Ci.nsINavHistoryQueryResultNode))
      return false;

    switch (aContainer.queryOptions.resultType) {
      case Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY:
      case Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY:
      case Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY:
      case Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY:
        return false;
    }

    
    let nodeType = aContainer.type;
    return nodeType != Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER &&
           nodeType != Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT;
  },

  
























  _getRowForNode:
  function PTV__getRowForNode(aNode, aForceBuild, aParentRow, aNodeIndex) {
    if (aNode == this._rootNode)
      throw new Error("The root node is never visible");

    
    
    
    let ancestors = [x for (x of PlacesUtils.nodeAncestors(aNode))];
    if (ancestors.length == 0 ||
        ancestors[ancestors.length - 1] != this._rootNode) {
      throw new Error("Removed node passed to _getRowForNode");
    }

    
    for (let ancestor of ancestors) {
      if (!ancestor.containerOpen)
        throw new Error("Invisible node passed to _getRowForNode");
    }

    
    let parent = aNode.parent;
    let parentIsPlain = this._isPlainContainer(parent);
    if (!parentIsPlain) {
      if (parent == this._rootNode)
        return this._rows.indexOf(aNode);

      return this._rows.indexOf(aNode, aParentRow);
    }

    let row = -1;
    let useNodeIndex = typeof(aNodeIndex) == "number";
    if (parent == this._rootNode)
      row = useNodeIndex ? aNodeIndex : this._rootNode.getChildIndex(aNode);
    else if (useNodeIndex && typeof(aParentRow) == "number") {
      
      
      row = aParentRow + aNodeIndex + 1;
    }
    else {
      
      
      
      row = this._rows.indexOf(aNode, aParentRow);
      if (row == -1 && aForceBuild) {
        let parentRow = typeof(aParentRow) == "number" ? aParentRow
                                                       : this._getRowForNode(parent);
        row = parentRow + parent.getChildIndex(aNode) + 1;
      }
    }

    if (row != -1)
      this._rows[row] = aNode;

    return row;
  },

  






  _getParentByChildRow: function PTV__getParentByChildRow(aChildRow) {
    let node = this._getNodeForRow(aChildRow);
    let parent = (node === null) ? this._rootNode : node.parent;

    
    if (parent == this._rootNode)
      return [this._rootNode, -1];

    let parentRow = this._rows.lastIndexOf(parent, aChildRow - 1);
    return [parent, parentRow];
  },

  


  _getNodeForRow: function PTV__getNodeForRow(aRow) {
    if (aRow < 0) {
      return null;
    }
  
    let node = this._rows[aRow];
    if (node !== undefined)
      return node;

    
    let rowNode, row;
    for (let i = aRow - 1; i >= 0 && rowNode === undefined; i--) {
      rowNode = this._rows[i];
      row = i;
    }

    
    
    if (!rowNode)
      return this._rows[aRow] = this._rootNode.getChild(aRow);

    
    
    if (rowNode instanceof Ci.nsINavHistoryContainerResultNode)
      return this._rows[aRow] = rowNode.getChild(aRow - row - 1);

    let [parent, parentRow] = this._getParentByChildRow(row);
    return this._rows[aRow] = parent.getChild(aRow - parentRow - 1);
  },

  














  _buildVisibleSection:
  function PTV__buildVisibleSection(aContainer, aFirstChildRow, aToOpen)
  {
    
    if (!aContainer.containerOpen)
      return 0;

    
    
    
    let cc = aContainer.childCount;
    let newElements = new Array(cc);
    this._rows = this._rows.splice(0, aFirstChildRow)
                     .concat(newElements, this._rows);

    if (this._isPlainContainer(aContainer))
      return cc;

    const openLiteral = PlacesUIUtils.RDF.GetResource("http://home.netscape.com/NC-rdf#open");
    const trueLiteral = PlacesUIUtils.RDF.GetLiteral("true");
    let sortingMode = this._result.sortingMode;

    let rowsInserted = 0;
    for (let i = 0; i < cc; i++) {
      let curChild = aContainer.getChild(i);
      let curChildType = curChild.type;

      let row = aFirstChildRow + rowsInserted;

      
      if (curChildType == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
        if (sortingMode != Ci.nsINavHistoryQueryOptions.SORT_BY_NONE) {
          
          
          
          this._rows.splice(row, 1);
          continue;
        }
      }

      this._rows[row] = curChild;
      rowsInserted++;

      
      if (!this._flatList &&
          curChild instanceof Ci.nsINavHistoryContainerResultNode &&
          !this._controller.hasCachedLivemarkInfo(curChild)) {
        let uri = curChild.uri;
        let isopen = false;

        if (uri) {
          let val = this._xulStore.getValue(document.documentURI, uri, "open");
          isopen = (val == "true");
        }

        if (isopen != curChild.containerOpen)
          aToOpen.push(curChild);
        else if (curChild.containerOpen && curChild.childCount > 0)
          rowsInserted += this._buildVisibleSection(curChild, row + 1, aToOpen);
      }
    }

    return rowsInserted;
  },

  



  _countVisibleRowsForNodeAtRow:
  function PTV__countVisibleRowsForNodeAtRow(aNodeRow) {
    let node = this._rows[aNodeRow];

    
    
    if (!(node instanceof Ci.nsINavHistoryContainerResultNode))
      return 1;

    let outerLevel = node.indentLevel;
    for (let i = aNodeRow + 1; i < this._rows.length; i++) {
      let rowNode = this._rows[i];
      if (rowNode && rowNode.indentLevel <= outerLevel)
        return i - aNodeRow;
    }

    
    return this._rows.length - aNodeRow;
  },

  _getSelectedNodesInRange:
  function PTV__getSelectedNodesInRange(aFirstRow, aLastRow) {
    let selection = this.selection;
    let rc = selection.getRangeCount();
    if (rc == 0)
      return [];

    
    
    let firstVisibleRow = this._tree.getFirstVisibleRow();
    let lastVisibleRow = this._tree.getLastVisibleRow();

    let nodesInfo = [];
    for (let rangeIndex = 0; rangeIndex < rc; rangeIndex++) {
      let min = { }, max = { };
      selection.getRangeAt(rangeIndex, min, max);

      
      
      if (max.value < aFirstRow || min.value > aLastRow)
        continue;

      let firstRow = Math.max(min.value, aFirstRow);
      let lastRow = Math.min(max.value, aLastRow);
      for (let i = firstRow; i <= lastRow; i++) {
        nodesInfo.push({
          node: this._rows[i],
          oldRow: i,
          wasVisible: i >= firstVisibleRow && i <= lastVisibleRow
        });
      }
    }

    return nodesInfo;
  },

  














  _getNewRowForRemovedNode:
  function PTV__getNewRowForRemovedNode(aUpdatedContainer, aOldNode) {
    let parent = aOldNode.parent;
    if (parent) {
      
      
      
      
      let ancestors = PlacesUtils.nodeAncestors(aOldNode);
      for (let ancestor of ancestors) {
        if (!ancestor.containerOpen)
          return -1;
      }

      return this._getRowForNode(aOldNode, true);
    }

    
    
    
    
    
    let newNode = aUpdatedContainer.findNodeByDetails(aOldNode.uri,
                                                      aOldNode.time,
                                                      aOldNode.itemId,
                                                      true);
    if (!newNode)
      return -1;

    return this._getRowForNode(newNode, true);
  },

  









  _restoreSelection:
  function PTV__restoreSelection(aNodesInfo, aUpdatedContainer) {
    if (aNodesInfo.length == 0)
      return;

    let selection = this.selection;

    
    
    let scrollToRow = -1;
    for (let i = 0; i < aNodesInfo.length; i++) {
      let nodeInfo = aNodesInfo[i];
      let row = this._getNewRowForRemovedNode(aUpdatedContainer,
                                              nodeInfo.node);
      
      if (row != -1) {
        selection.rangedSelect(row, row, true);
        if (nodeInfo.wasVisible && scrollToRow == -1)
          scrollToRow = row;
      }
    }

    
    
    if (aNodesInfo.length == 1 && selection.count == 0) {
      let row = Math.min(aNodesInfo[0].oldRow, this._rows.length - 1);
      if (row != -1) {
        selection.rangedSelect(row, row, true);
        if (aNodesInfo[0].wasVisible && scrollToRow == -1)
          scrollToRow = aNodesInfo[0].oldRow;
      }
    }

    if (scrollToRow != -1)
      this._tree.ensureRowIsVisible(scrollToRow);
  },

  _convertPRTimeToString: function PTV__convertPRTimeToString(aTime) {
    const MS_PER_MINUTE = 60000;
    const MS_PER_DAY = 86400000;
    let timeMs = aTime / 1000; 

    
    
    
    
    let dateObj = new Date();
    let now = dateObj.getTime() - dateObj.getTimezoneOffset() * MS_PER_MINUTE;
    let midnight = now - (now % MS_PER_DAY);
    midnight += new Date(midnight).getTimezoneOffset() * MS_PER_MINUTE;

    let dateFormat = timeMs >= midnight ?
                      Ci.nsIScriptableDateFormat.dateFormatNone :
                      Ci.nsIScriptableDateFormat.dateFormatShort;

    let timeObj = new Date(timeMs);
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
    let columnType = aColumn.element.getAttribute("anonid") || aColumn.id;

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
        if (this._result.sortingAnnotation == PlacesUIUtils.DESCRIPTION_ANNO)
          return [this.COLUMN_TYPE_DESCRIPTION, false];
        break;
      case Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING:
        if (this._result.sortingAnnotation == PlacesUIUtils.DESCRIPTION_ANNO)
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

  
  nodeInserted: function PTV_nodeInserted(aParentNode, aNode, aNewIndex) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree || !this._result)
      return;

    
    if (PlacesUtils.nodeIsSeparator(aNode) && this.isSorted())
      return;

    let parentRow;
    if (aParentNode != this._rootNode) {
      parentRow = this._getRowForNode(aParentNode);

      
      if (aParentNode.childCount == 1)
        this._tree.invalidateRow(parentRow);
    }

    
    let row = -1;
    let cc = aParentNode.childCount;
    if (aNewIndex == 0 || this._isPlainContainer(aParentNode) || cc == 0) {
      
      
      if (aParentNode == this._rootNode)
        row = aNewIndex;
      else
        row = parentRow + aNewIndex + 1;
    }
    else {
      
      
      
      
      let separatorsAreHidden = PlacesUtils.nodeIsSeparator(aNode) &&
                                this.isSorted();
      for (let i = aNewIndex + 1; i < cc; i++) {
        let node = aParentNode.getChild(i);
        if (!separatorsAreHidden || PlacesUtils.nodeIsSeparator(node)) {
          
          
          row = this._getRowForNode(node, false, parentRow, i);
          break;
        }
      }
      if (row < 0) {
        
        
        
        let prevChild = aParentNode.getChild(aNewIndex - 1);
        let prevIndex = this._getRowForNode(prevChild, false, parentRow,
                                            aNewIndex - 1);
        row = prevIndex + this._countVisibleRowsForNodeAtRow(prevIndex);
      }
    }

    this._rows.splice(row, 0, aNode);
    this._tree.rowCountChanged(row, 1);

    if (PlacesUtils.nodeIsContainer(aNode) &&
        PlacesUtils.asContainer(aNode).containerOpen) {
      this.invalidateContainer(aNode);
    }
  },

  








  nodeRemoved: function PTV_nodeRemoved(aParentNode, aNode, aOldIndex) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree || !this._result)
      return;

    
    if (aNode == this._rootNode)
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    
    if (PlacesUtils.nodeIsSeparator(aNode) && this.isSorted())
      return;

    let parentRow = aParentNode == this._rootNode ?
                    undefined : this._getRowForNode(aParentNode, true);
    let oldRow = this._getRowForNode(aNode, true, parentRow, aOldIndex);
    if (oldRow < 0)
      throw Cr.NS_ERROR_UNEXPECTED;

    
    
    let selectNext = false;
    let selection = this.selection;
    if (selection.getRangeCount() == 1) {
      let min = { }, max = { };
      selection.getRangeAt(0, min, max);
      if (min.value == max.value &&
          this.nodeForTreeIndex(min.value) == aNode)
        selectNext = true;
    }

    
    let count = this._countVisibleRowsForNodeAtRow(oldRow);
    this._rows.splice(oldRow, count);
    this._tree.rowCountChanged(oldRow, -count);

    
    if (aParentNode != this._rootNode && !aParentNode.hasChildren) {
      let parentRow = oldRow - 1;
      this._tree.invalidateRow(parentRow);
    }

    
    if (!selectNext)
      return;

    
    let rowToSelect = Math.min(oldRow, this._rows.length - 1);
    if (rowToSelect != -1)
      this.selection.rangedSelect(rowToSelect, rowToSelect, true);
  },

  nodeMoved:
  function PTV_nodeMoved(aNode, aOldParent, aOldIndex, aNewParent, aNewIndex) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree || !this._result)
      return;

    
    if (PlacesUtils.nodeIsSeparator(aNode) && this.isSorted())
      return;

    
    
    let oldParentRow = aOldParent == this._rootNode ?
                         undefined : this._getRowForNode(aOldParent, true);
    let oldRow = this._getRowForNode(aNode, true, oldParentRow, aOldIndex);
    if (oldRow < 0)
      throw Cr.NS_ERROR_UNEXPECTED;

    
    let count = this._countVisibleRowsForNodeAtRow(oldRow);

    
    let nodesToReselect =
      this._getSelectedNodesInRange(oldRow, oldRow + count);
    if (nodesToReselect.length > 0)
      this.selection.selectEventsSuppressed = true;

    
    if (aOldParent != this._rootNode && !aOldParent.hasChildren) {
      let parentRow = oldRow - 1;
      this._tree.invalidateRow(parentRow);
    }

    
    this._rows.splice(oldRow, count);
    this._tree.rowCountChanged(oldRow, -count);

    
    this.nodeInserted(aNewParent, aNode, aNewIndex);

    
    if (nodesToReselect.length > 0) {
      this._restoreSelection(nodesToReselect, aNewParent);
      this.selection.selectEventsSuppressed = false;
    }
  },

  _invalidateCellValue: function PTV__invalidateCellValue(aNode,
                                                          aColumnType) {
    NS_ASSERT(this._result, "Got a notification but have no result!");
    if (!this._tree || !this._result)
      return;

    
    if (aNode == this._rootNode)
      return;

    let row = this._getRowForNode(aNode);
    if (row == -1)
      return;

    let column = this._findColumnByType(aColumnType);
    if (column && !column.element.hidden)
      this._tree.invalidateCell(row, column);

    
    if (aColumnType != this.COLUMN_TYPE_LASTMODIFIED) {
      let lastModifiedColumn =
        this._findColumnByType(this.COLUMN_TYPE_LASTMODIFIED);
      if (lastModifiedColumn && !lastModifiedColumn.hidden)
        this._tree.invalidateCell(row, lastModifiedColumn);
    }
  },

  _populateLivemarkContainer: function PTV__populateLivemarkContainer(aNode) {
    PlacesUtils.livemarks.getLivemark({ id: aNode.itemId })
      .then(aLivemark => {
        let placesNode = aNode;
        
        if (!placesNode.containerOpen)
          return;

        let children = aLivemark.getNodesForContainer(placesNode);
        for (let i = 0; i < children.length; i++) {
          let child = children[i];
          this.nodeInserted(placesNode, child, i);
        }
      }, Components.utils.reportError);
  },

  nodeTitleChanged: function PTV_nodeTitleChanged(aNode, aNewTitle) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_TITLE);
  },

  nodeURIChanged: function PTV_nodeURIChanged(aNode, aNewURI) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_URI);
  },

  nodeIconChanged: function PTV_nodeIconChanged(aNode) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_TITLE);
  },

  nodeHistoryDetailsChanged:
  function PTV_nodeHistoryDetailsChanged(aNode, aUpdatedVisitDate,
                                         aUpdatedVisitCount) {
    if (aNode.parent && this._controller.hasCachedLivemarkInfo(aNode.parent)) {
      
      let parentRow = this._flatList ? 0 : this._getRowForNode(aNode.parent);
      for (let i = parentRow; i < this._rows.length; i++) {
        let child = this.nodeForTreeIndex(i);
        if (child.uri == aNode.uri) {
          this._cellProperties.delete(child);
          this._invalidateCellValue(child, this.COLUMN_TYPE_TITLE);
          break;
        }
      }
      return;
    }

    this._invalidateCellValue(aNode, this.COLUMN_TYPE_DATE);
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_VISITCOUNT);
  },

  nodeTagsChanged: function PTV_nodeTagsChanged(aNode) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_TAGS);
  },

  nodeKeywordChanged: function PTV_nodeKeywordChanged(aNode, aNewKeyword) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_KEYWORD);
  },

  nodeAnnotationChanged: function PTV_nodeAnnotationChanged(aNode, aAnno) {
    if (aAnno == PlacesUIUtils.DESCRIPTION_ANNO) {
      this._invalidateCellValue(aNode, this.COLUMN_TYPE_DESCRIPTION);
    }
    else if (aAnno == PlacesUtils.LMANNO_FEEDURI) {
      PlacesUtils.livemarks.getLivemark({ id: aNode.itemId })
        .then(aLivemark => {
          this._controller.cacheLivemarkInfo(aNode, aLivemark);
          let properties = this._cellProperties.get(aNode);
          this._cellProperties.set(aNode, properties += " livemark ");

          
          this._invalidateCellValue(aNode, this.COLUMN_TYPE_TITLE);
        }, Components.utils.reportError);
    }
  },

  nodeDateAddedChanged: function PTV_nodeDateAddedChanged(aNode, aNewValue) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_DATEADDED);
  },

  nodeLastModifiedChanged:
  function PTV_nodeLastModifiedChanged(aNode, aNewValue) {
    this._invalidateCellValue(aNode, this.COLUMN_TYPE_LASTMODIFIED);
  },

  containerStateChanged:
  function PTV_containerStateChanged(aNode, aOldState, aNewState) {
    this.invalidateContainer(aNode);

    if (PlacesUtils.nodeIsFolder(aNode) ||
        (this._flatList && aNode == this._rootNode)) {
      let queryOptions = PlacesUtils.asQuery(this._rootNode).queryOptions;
      if (queryOptions.excludeItems) {
        return;
      }

      PlacesUtils.livemarks.getLivemark({ id: aNode.itemId })
        .then(aLivemark => {
          let shouldInvalidate = 
            !this._controller.hasCachedLivemarkInfo(aNode);
          this._controller.cacheLivemarkInfo(aNode, aLivemark);
          if (aNewState == Components.interfaces.nsINavHistoryContainerResultNode.STATE_OPENED) {
            aLivemark.registerForUpdates(aNode, this);
            
            aLivemark.reload();
            PlacesUtils.livemarks.reloadLivemarks();
            if (shouldInvalidate)
              this.invalidateContainer(aNode);
          }
          else {
            aLivemark.unregisterForUpdates(aNode);
          }
        }, () => undefined);
    }
  },

  invalidateContainer: function PTV_invalidateContainer(aContainer) {
    NS_ASSERT(this._result, "Need to have a result to update");
    if (!this._tree)
      return;

    let startReplacement, replaceCount;
    if (aContainer == this._rootNode) {
      startReplacement = 0;
      replaceCount = this._rows.length;

      
      if (!this._rootNode.containerOpen) {
        this._rows = [];
        if (replaceCount)
          this._tree.rowCountChanged(startReplacement, -replaceCount);

        return;
      }
    }
    else {
      
      let row = this._getRowForNode(aContainer);
      this._tree.invalidateRow(row);

      
      
      startReplacement = row + 1;
      replaceCount = this._countVisibleRowsForNodeAtRow(row) - 1;
    }

    
    let nodesToReselect =
      this._getSelectedNodesInRange(startReplacement,
                                    startReplacement + replaceCount);

    
    this.selection.selectEventsSuppressed = true;

    
    this._rows.splice(startReplacement, replaceCount);

    
    if (!aContainer.containerOpen) {
      let oldSelectionCount = this.selection.count;
      if (replaceCount)
        this._tree.rowCountChanged(startReplacement, -replaceCount);

      
      
      if (nodesToReselect.length > 0 &&
          nodesToReselect.length == oldSelectionCount) {
        this.selection.rangedSelect(startReplacement, startReplacement, true);
        this._tree.ensureRowIsVisible(startReplacement);
      }

      this.selection.selectEventsSuppressed = false;
      return;
    }

    
    this._tree.beginUpdateBatch();
    if (replaceCount)
      this._tree.rowCountChanged(startReplacement, -replaceCount);

    let toOpenElements = [];
    let elementsAddedCount = this._buildVisibleSection(aContainer,
                                                       startReplacement,
                                                       toOpenElements);
    if (elementsAddedCount)
      this._tree.rowCountChanged(startReplacement, elementsAddedCount);

    if (!this._flatList) {
      
      for (let i = 0; i < toOpenElements.length; i++) {
        let item = toOpenElements[i];
        let parent = item.parent;

        
        while (parent) {
          if (parent.uri == item.uri)
            break;
          parent = parent.parent;
        }

        
        
        if (!parent && !item.containerOpen)
          item.containerOpen = true;
      }
    }

    if (this._controller.hasCachedLivemarkInfo(aContainer)) {
      let queryOptions = PlacesUtils.asQuery(this._result.root).queryOptions;
      if (!queryOptions.excludeItems) {
        this._populateLivemarkContainer(aContainer);
      }
    }

    this._tree.endUpdateBatch();

    
    this._restoreSelection(nodesToReselect, aContainer);
    this.selection.selectEventsSuppressed = false;
  },

  _columns: [],
  _findColumnByType: function PTV__findColumnByType(aColumnType) {
    if (this._columns[aColumnType])
      return this._columns[aColumnType];

    let columns = this._tree.columns;
    let colCount = columns.count;
    for (let i = 0; i < colCount; i++) {
      let column = columns.getColumnAt(i);
      let columnType = this._getColumnType(column);
      this._columns[columnType] = column;
      if (columnType == aColumnType)
        return column;
    }

    
    
    return null;
  },

  sortingChanged: function PTV__sortingChanged(aSortingMode) {
    if (!this._tree || !this._result)
      return;

    
    window.updateCommands("sort");

    let columns = this._tree.columns;

    
    let sortedColumn = columns.getSortedColumn();
    if (sortedColumn)
      sortedColumn.element.removeAttribute("sortDirection");

    
    if (aSortingMode == Ci.nsINavHistoryQueryOptions.SORT_BY_NONE)
      return;

    let [desiredColumn, desiredIsDescending] =
      this._sortTypeToColumnType(aSortingMode);
    let colCount = columns.count;
    let column = this._findColumnByType(desiredColumn);
    if (column) {
      let sortDir = desiredIsDescending ? "descending" : "ascending";
      column.element.setAttribute("sortDirection", sortDir);
    }
  },

  _inBatchMode: false,
  batching: function PTV__batching(aToggleMode) {
    if (this._inBatchMode != aToggleMode) {
      this._inBatchMode = this.selection.selectEventsSuppressed = aToggleMode;
      if (this._inBatchMode) {
        this._tree.beginUpdateBatch();
      }
      else {
        this._tree.endUpdateBatch();
      }
    }
  },

  get result() this._result,
  set result(val) {
    if (this._result) {
      this._result.removeObserver(this);
      this._rootNode.containerOpen = false;
    }

    if (val) {
      this._result = val;
      this._rootNode = this._result.root;
      this._cellProperties = new Map();
      this._cuttingNodes = new Set();
    }
    else if (this._result) {
      delete this._result;
      delete this._rootNode;
      delete this._cellProperties;
      delete this._cuttingNodes;
    }

    
    if (this._tree && val)
      this._finishInit();

    return val;
  },

  nodeForTreeIndex: function PTV_nodeForTreeIndex(aIndex) {
    if (aIndex > this._rows.length)
      throw Cr.NS_ERROR_INVALID_ARG;

    return this._getNodeForRow(aIndex);
  },

  treeIndexForNode: function PTV_treeNodeForIndex(aNode) {
    
    try {
      return this._getRowForNode(aNode, true);
    }
    catch(ex) { }

    return Ci.nsINavHistoryResultTreeViewer.INDEX_INVISIBLE;
  },

  
  get rowCount() this._rows.length,
  get selection() this._selection,
  set selection(val) this._selection = val,

  getRowProperties: function() { return ""; },

  getCellProperties:
  function PTV_getCellProperties(aRow, aColumn) {
    
    var props = "";
    let columnType = aColumn.element.getAttribute("anonid");
    if (columnType)
      props += columnType;
    else
      columnType = aColumn.id;

    
    if (columnType == "url")
      props += " ltr";

    if (columnType != "title")
      return props;

    let node = this._getNodeForRow(aRow);

    if (this._cuttingNodes.has(node)) {
      props += " cutting";
    }

    let properties = this._cellProperties.get(node);
    if (properties === undefined) {
      properties = "";
      let itemId = node.itemId;
      let nodeType = node.type;
      if (PlacesUtils.containerTypes.indexOf(nodeType) != -1) {
        if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY) {
          properties += " query";
          if (PlacesUtils.nodeIsTagQuery(node))
            properties += " tagContainer";
          else if (PlacesUtils.nodeIsDay(node))
            properties += " dayContainer";
          else if (PlacesUtils.nodeIsHost(node))
            properties += " hostContainer";
        }
        else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER ||
                 nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT) {
          if (this._controller.hasCachedLivemarkInfo(node)) {
            properties += " livemark";
          }
          else {
            PlacesUtils.livemarks.getLivemark({ id: node.itemId })
              .then(aLivemark => {
                this._controller.cacheLivemarkInfo(node, aLivemark);
                properties += " livemark";
                
                this._invalidateCellValue(node, this.COLUMN_TYPE_TITLE);
              }, () => undefined);
          }
        }

        if (itemId != -1) {
          let queryName = PlacesUIUtils.getLeftPaneQueryNameFromId(itemId);
          if (queryName)
            properties += " OrganizerQuery_" + queryName;
        }
      }
      else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR)
        properties += " separator";
      else if (PlacesUtils.nodeIsURI(node)) {
        properties += " " + PlacesUIUtils.guessUrlSchemeForUI(node.uri);

        if (this._controller.hasCachedLivemarkInfo(node.parent)) {
          properties += " livemarkItem";
          if (node.accessCount) {
            properties += " visited";
          }
        }
      }

      this._cellProperties.set(node, properties);
    }

    return props + " " + properties;
  },

  getColumnProperties: function(aColumn) { return ""; },

  isContainer: function PTV_isContainer(aRow) {
    
    let node = this._rows[aRow];
    if (node === undefined)
      return false;

    if (PlacesUtils.nodeIsContainer(node)) {
      
      
      if (this._flatList)
        return true;

      
      if (PlacesUtils.nodeIsQuery(node)) {
        let parent = node.parent;
        if ((PlacesUtils.nodeIsQuery(parent) ||
             PlacesUtils.nodeIsFolder(parent)) &&
            !PlacesUtils.asQuery(node).hasChildren)
          return PlacesUtils.asQuery(parent).queryOptions.expandQueries;
      }
      return true;
    }
    return false;
  },

  isContainerOpen: function PTV_isContainerOpen(aRow) {
    if (this._flatList)
      return false;

    
    return this._rows[aRow].containerOpen;
  },

  isContainerEmpty: function PTV_isContainerEmpty(aRow) {
    if (this._flatList)
      return true;

    let node = this._rows[aRow];
    if (this._controller.hasCachedLivemarkInfo(node)) {
      let queryOptions = PlacesUtils.asQuery(this._result.root).queryOptions;
      return queryOptions.excludeItems;
    }

    
    return !node.hasChildren;
  },

  isSeparator: function PTV_isSeparator(aRow) {
    
    let node = this._rows[aRow];
    return node && PlacesUtils.nodeIsSeparator(node);
  },

  isSorted: function PTV_isSorted() {
    return this._result.sortingMode !=
           Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
  },

  canDrop: function PTV_canDrop(aRow, aOrientation, aDataTransfer) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    
    if (this.isSorted())
      return false;

    let ip = this._getInsertionPoint(aRow, aOrientation);
    return ip && PlacesControllerDragHelper.canDrop(ip, aDataTransfer);
  },

  _getInsertionPoint: function PTV__getInsertionPoint(index, orientation) {
    let container = this._result.root;
    let dropNearItemId = -1;
    
    
    if (index != -1) {
      let lastSelected = this.nodeForTreeIndex(index);
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
        
        container = lastSelected.parent;

        
        
        
        
        
        
        if (!container || !container.containerOpen)
          return null;

        
        
        if (PlacesControllerDragHelper.disallowInsertion(container))
          return null;

        let queryOptions = PlacesUtils.asQuery(this._result.root).queryOptions;
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
          let lsi = container.getChildIndex(lastSelected);
          index = orientation == Ci.nsITreeView.DROP_BEFORE ? lsi : lsi + 1;
        }
      }
    }

    if (PlacesControllerDragHelper.disallowInsertion(container))
      return null;

    let tagName = PlacesUtils.nodeIsTagQuery(container) ? container.title : null;
    return new InsertionPoint(PlacesUtils.getConcreteItemId(container),
                              index, orientation,
                              tagName,
                              dropNearItemId);
  },

  drop: function PTV_drop(aRow, aOrientation, aDataTransfer) {
    
    
    
    let ip = this._getInsertionPoint(aRow, aOrientation);
    if (ip) {
      PlacesControllerDragHelper.onDrop(ip, aDataTransfer)
                                .then(null, Components.utils.reportError);
    }

    PlacesControllerDragHelper.currentDropTarget = null;
  },

  getParentIndex: function PTV_getParentIndex(aRow) {
    let [parentNode, parentRow] = this._getParentByChildRow(aRow);
    return parentRow;
  },

  hasNextSibling: function PTV_hasNextSibling(aRow, aAfterIndex) {
    if (aRow == this._rows.length - 1) {
      
      return false;
    }

    let node = this._rows[aRow];
    if (node === undefined || this._isPlainContainer(node.parent)) {
      
      
      
      let nextNode = this._rows[aRow + 1];
      return (nextNode == undefined || nextNode.parent == node.parent);
    }

    let thisLevel = node.indentLevel;
    for (let i = aAfterIndex + 1; i < this._rows.length; ++i) {
      let rowNode = this._getNodeForRow(i);
      let nextLevel = rowNode.indentLevel;
      if (nextLevel == thisLevel)
        return true;
      if (nextLevel < thisLevel)
        break;
    }

    return false;
  },

  getLevel: function(aRow) this._getNodeForRow(aRow).indentLevel,

  getImageSrc: function PTV_getImageSrc(aRow, aColumn) {
    
    if (this._getColumnType(aColumn) != this.COLUMN_TYPE_TITLE)
      return "";

    let node = this._getNodeForRow(aRow);
    if (PlacesUtils.nodeIsURI(node) && node.icon)
      return PlacesUtils.getImageURLForResolution(window, node.icon);

    return node.icon;
  },

  getProgressMode: function(aRow, aColumn) { },
  getCellValue: function(aRow, aColumn) { },

  getCellText: function PTV_getCellText(aRow, aColumn) {
    let node = this._getNodeForRow(aRow);
    switch (this._getColumnType(aColumn)) {
      case this.COLUMN_TYPE_TITLE:
        
        
        
        
        if (PlacesUtils.nodeIsSeparator(node))
          return "";
        return PlacesUIUtils.getBestTitle(node, true);
      case this.COLUMN_TYPE_TAGS:
        return node.tags;
      case this.COLUMN_TYPE_URI:
        if (PlacesUtils.nodeIsURI(node))
          return node.uri;
        return "";
      case this.COLUMN_TYPE_DATE:
        let nodeTime = node.time;
        if (nodeTime == 0 || !PlacesUtils.nodeIsURI(node)) {
          
          
          
          
          
          return "";
        }

        return this._convertPRTimeToString(nodeTime);
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
                               getItemAnnotation(node.itemId, PlacesUIUtils.DESCRIPTION_ANNO);
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
    
    
    
    this.batching(false);

    let hasOldTree = this._tree != null;
    this._tree = aTree;

    if (this._result) {
      if (hasOldTree) {
        
        
        if (!aTree) {
          this._result.removeObserver(this);
          this._rootNode.containerOpen = false;
        }
      }
      if (aTree)
        this._finishInit();
    }
  },

  toggleOpenState: function PTV_toggleOpenState(aRow) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    let node = this._rows[aRow];
    if (this._flatList && this._openContainerCallback) {
      this._openContainerCallback(node);
      return;
    }

    
    if (!this._controller.hasCachedLivemarkInfo(node)) {
      let uri = node.uri;

      if (uri) {
        let docURI = document.documentURI;

        if (node.containerOpen) {
          this._xulStore.removeValue(docURI, uri, "open");
        } else {
          this._xulStore.setValue(docURI, uri, "open", "true");
        }
      }
    }

    node.containerOpen = !node.containerOpen;
  },

  cycleHeader: function PTV_cycleHeader(aColumn) {
    if (!this._result)
      throw Cr.NS_ERROR_UNEXPECTED;

    
    
    
    
    
    
    
    
    
    
    
    let allowTriState = PlacesUtils.nodeIsFolder(this._result.root);

    let oldSort = this._result.sortingMode;
    let oldSortingAnnotation = this._result.sortingAnnotation;
    let newSort;
    let newSortingAnnotation = "";
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
            oldSortingAnnotation == PlacesUIUtils.DESCRIPTION_ANNO) {
          newSort = NHQO.SORT_BY_ANNOTATION_DESCENDING;
          newSortingAnnotation = PlacesUIUtils.DESCRIPTION_ANNO;
        }
        else if (allowTriState &&
                 oldSort == NHQO.SORT_BY_ANNOTATION_DESCENDING &&
                 oldSortingAnnotation == PlacesUIUtils.DESCRIPTION_ANNO)
          newSort = NHQO.SORT_BY_NONE;
        else {
          newSort = NHQO.SORT_BY_ANNOTATION_ASCENDING;
          newSortingAnnotation = PlacesUIUtils.DESCRIPTION_ANNO;
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

    let node = this._rows[aRow];
    if (!node) {
      Cu.reportError("isEditable called for an unbuilt row.");
      return false;
    }
    let itemId = node.itemId;

    
    
    if (itemId == -1)
      return false;

    
    
    
    
    
    
    
    
    
    
    if (PlacesUtils.nodeIsSeparator(node) || PlacesUtils.isRootItem(itemId))
      return false;

    let parentId = PlacesUtils.getConcreteItemId(node.parent);
    if (parentId == PlacesUIUtils.leftPaneFolderId ||
        parentId == PlacesUIUtils.allBookmarksFolderId) {
      
      
      
      
      return false;
    }

    return true;
  },

  setCellText: function PTV_setCellText(aRow, aColumn, aText) {
    
    let node = this._rows[aRow];
    if (node.title != aText) {
      if (!PlacesUIUtils.useAsyncTransactions) {
        let txn = new PlacesEditItemTitleTransaction(node.itemId, aText);
        PlacesUtils.transactionManager.doTransaction(txn);
        return;
      }
      PlacesTransactions.EditTitle({ guid: node.bookmarkGuid, title: aText })
                        .transact().catch(Cu.reportError);
    }
  },

  toggleCutNode: function PTV_toggleCutNode(aNode, aValue) {
    let currentVal = this._cuttingNodes.has(aNode);
    if (currentVal != aValue) {
      if (aValue)
        this._cuttingNodes.add(aNode);
      else
        this._cuttingNodes.delete(aNode);

      this._invalidateCellValue(aNode, this.COLUMN_TYPE_TITLE);
    }
  },

  selectionChanged: function() { },
  cycleCell: function(aRow, aColumn) { },
  isSelectable: function(aRow, aColumn) { return false; },
  performAction: function(aAction) { },
  performActionOnRow: function(aAction, aRow) { },
  performActionOnCell: function(aAction, aRow, aColumn) { }
};
