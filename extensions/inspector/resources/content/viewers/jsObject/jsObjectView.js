



















































function JSObjectView(aObject)
{
  this.mJSObject = aObject;
}

JSObjectView.prototype = 
{
  
  
  
  
  

  mTree: null,
  mProperties: [],
  mLevels: [],
  mOrder: [],
  
  get rowCount() {
    return this.mObjects.length;
  },
  








  
  getRowProperties: function(aIndex, aProperties)
  {
  },
  
  getCellProperties: function(aIndex, aCol, aProperties)
  {
  },
  
  getColumnProperties: function(aCol, aProperties)
  {
  },

  isContainer: function(aIndex)
  {
  },
  
  isContainerOpen: function(aIndex)
  {
  },
  
  isContainerEmpty: function(aIndex)
  {
  },
  
  isSeparator: function(aIndex)
  {
  },

  isSorted: function()
  {
    return false;
  },

  getParentIndex: function(aRowIndex)
  {
    return 1;
  },
  
  hasNextSibling: function(aRowIndex, aAfterIndex)
  {
    return false;
  },
  
  getLevel: function(aIndex)
  {
  },

  getImageSrc: function(aRow, aCol)
  {
  },

  getProgressMode: function(aRow, aCol)
  {
  },

  getCellValue: function(aRow, aCol)
  {
  },

  getCellText: function(aRow, aCol)
  {
    var object = null;
  
    switch (aCol.id) {
      case "olrCol1":
        return 1;
        break;
      case "olrCol2":
        return 2;
        break;    
    };
  },
  
  setTree: function(aTree)
  {
    this.mTree = aTree;
  },
  
  toggleOpenState: function(aIndex)
  {
  },
  
  cycleHeader: function(aCol)
  {
  },
  
  selectionChanged: function()
  {
  },
  
  cycleCell: function(aRow, aCol)
  {
  },
  
  isEditable: function(aRow, aCol)
  {
  },

  isSelectable: function(aRow, aCol)
  {
  },
  
  setCellValue: function(aRow, aCol, aValue)
  {
  },

  setCellText: function(aRow, aCol, aValue)
  {
  },
  
  performAction: function(aAction)
  {
  },
  
  performActionOnRow: function(aAction, aRow)
  {
  },
  
  performActionOnCell: function(aAction, aRow, aCol)
  {
  },
  
  
  
  

  
};