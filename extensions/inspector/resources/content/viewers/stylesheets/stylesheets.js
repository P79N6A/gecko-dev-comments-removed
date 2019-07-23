














































var viewer;





window.addEventListener("load", StylesheetsViewer_initialize, false);

function StylesheetsViewer_initialize()
{
  viewer = new StylesheetsViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function StylesheetsViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);

  this.mTree = document.getElementById("olStyleSheets");
  this.mOlBox = this.mTree.treeBoxObject;
}

StylesheetsViewer.prototype = 
{
  
  
  
  mSubject: null,
  mPane: null,
  mView: null,
  
  
  

  get uid() { return "stylesheets"; },
  get pane() { return this.mPane; },
  get selection() { return this.mSelection; },

  get subject() { return this.mSubject; },
  set subject(aObject) 
  {
    this.mView = new StyleSheetsView(aObject);
    this.mOlBox.view = this.mView;
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
    this.mView.selection.select(0);
  },

  initialize: function(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function()
  {
    this.mOlBox.view = null;
  },

  isCommandEnabled: function(aCommand)
  {
    return false;
  },
  
  getCommand: function(aCommand)
  {
    return null;
  },
  
  
  

  addObserver: function(aEvent, aObserver) { this.mObsMan.addObserver(aEvent, aObserver); },
  removeObserver: function(aEvent, aObserver) { this.mObsMan.removeObserver(aEvent, aObserver); },

  
  
  
  onItemSelected: function()
  {
    var idx = this.mTree.currentIndex;
    this.mSelection = this.mView.getSheet(idx);
    this.mObsMan.dispatchEvent("selectionChange", { selection: this.mSelection } );
  }

};




function StyleSheetsView(aDocument)
{
  this.mDocument = aDocument;
  this.mSheets = [];
  this.mLevels = [];
  this.mOpen = [];
  this.mChildCount = [];
  this.mRowCount = 0;
      
  var ss = aDocument.styleSheets;
  for (var i = 0; i < ss.length; ++i)
    this.insertSheet(ss[i], 0, -1);
}

StyleSheetsView.prototype = new inBaseTreeView();

StyleSheetsView.prototype.getSheet = 
function(aRow)
{
  return this.mSheets[aRow];
}

StyleSheetsView.prototype.insertSheet = 
function(aSheet, aLevel, aRow)
{
  var row = aRow < 0 ? this.mSheets.length : aRow;
  
  this.mSheets[row] = aSheet;
  this.mLevels[row] = aLevel;
  this.mOpen[row] = false;
  
  var count = 0;
  var rules = aSheet.cssRules;
  for (var i = 0; i < rules.length; ++i) {
    if (rules[i].type == CSSRule.IMPORT_RULE)
      ++count;
  }
  this.mChildCount[row] = count;
  ++this.mRowCount;
}

StyleSheetsView.prototype.shiftDataDown = 
function(aRow, aDiff)
{
  for (var i = this.mRowCount+aDiff-1; i >= aRow ; --i) {
    this.mSheets[i] = this.mSheets[i-aDiff];
    this.mLevels[i] = this.mLevels[i-aDiff];
    this.mOpen[i] = this.mOpen[i-aDiff];
    this.mChildCount[i] = this.mChildCount[i-aDiff];
  }
}

StyleSheetsView.prototype.shiftDataUp = 
function(aRow, aDiff)
{
  for (var i = aRow; i < this.mRowCount; ++i) {
    this.mSheets[i] = this.mSheets[i+aDiff];
    this.mLevels[i] = this.mLevels[i+aDiff];
    this.mOpen[i] = this.mOpen[i+aDiff];
    this.mChildCount[i] = this.mChildCount[i+aDiff];
  }
}

StyleSheetsView.prototype.getCellText = 
function(aRow, aCol) 
{
  if (aCol.id == "olcHref")
    return this.mSheets[aRow].href;
  else if (aCol.id == "olcRules")
    return this.mSheets[aRow].cssRules.length;
  return "";
}

StyleSheetsView.prototype.getLevel = 
function(aRow) 
{
  return this.mLevels[aRow];
}

StyleSheetsView.prototype.isContainer = 
function(aRow) 
{
  return this.mChildCount[aRow] > 0;
}

StyleSheetsView.prototype.isContainerEmpty = 
function(aRow) 
{
  return !this.isContainer(aRow);
}

StyleSheetsView.prototype.getParentIndex = 
function(aRow) 
{
  var baseLevel = this.mLevels[aRow];
  for (var i = aRow-1; i >= 0; --i) {
    if (this.mLevels[i] < baseLevel)
      return i;
  }
  return -1;
}

StyleSheetsView.prototype.hasNextSibling = 
function(aRow) 
{
  var baseLevel = this.mLevels[aRow];
  for (var i = aRow+1; i < this.mRowCount; ++i) {
    if (this.mLevels[i] < baseLevel)
      break;
    if (this.mLevels[i] == baseLevel)
      return true;
  }
  return false;
}

StyleSheetsView.prototype.isContainerOpen = 
function(aRow) 
{
  return this.mOpen[aRow];
}

StyleSheetsView.prototype.toggleOpenState = 
function(aRow) 
{
  var oldRowCount = this.mRowCount;
  if (this.mOpen[aRow]) {
    var baseLevel = this.mLevels[aRow];
    var count = 0;
    for (var i = aRow+1; i < this.mRowCount; ++i) {
      if (this.mLevels[i] <= baseLevel)
        break;
      ++count;
    }
    this.shiftDataUp(aRow+1, count);
    this.mRowCount -= count;
  } else {
    this.shiftDataDown(aRow+1, this.mChildCount[aRow]);
    
    var rules = this.mSheets[aRow].cssRules;
    for (changeCount = 0; changeCount < rules.length; ++changeCount) {
      if (rules[changeCount].type == CSSRule.IMPORT_RULE)
        this.insertSheet(rules[changeCount].styleSheet, this.mLevels[aRow]+1, aRow+changeCount+1);
    }
  }
  
  this.mOpen[aRow] = !this.mOpen[aRow];
  this.mTree.rowCountChanged(aRow+1, this.mRowCount - oldRowCount);
}

