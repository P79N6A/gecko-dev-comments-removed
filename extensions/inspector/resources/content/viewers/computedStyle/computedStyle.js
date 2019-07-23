















































var viewer;





window.addEventListener("load", ComputedStyleViewer_initialize, false);

function ComputedStyleViewer_initialize()
{
  viewer = new ComputedStyleViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function ComputedStyleViewer()
{
  this.mObsMan = new ObserverManager(this);
  this.mURL = window.location;
  
  this.mTree = document.getElementById("olStyles");
}


ComputedStyleViewer.prototype = 
{
  
  
  
  mSubject: null,
  mPane: null,

  
  

  get uid() { return "computedStyle" },
  get pane() { return this.mPane },

  get subject() { return this.mSubject },
  set subject(aObject) 
  {
    this.mTreeView = new ComputedStyleView(aObject);
    this.mTree.view = this.mTreeView;
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  initialize: function(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function()
  {
    
    
    
    this.mTree.view = null;
  },

  isCommandEnabled: function(aCommand)
  {
    if (aCommand == "cmdEditCopy") {
      return this.mTree.view.selection.count > 0;
    }
    return false;
  },
  
  getCommand: function(aCommand)
  {
    if (aCommand == "cmdEditCopy") {
      return new cmdEditCopy(this.mTreeView.getSelectedRowObjects());
    }
    return null;
  },

  
  

  addObserver: function(aEvent, aObserver) { this.mObsMan.addObserver(aEvent, aObserver); },
  removeObserver: function(aEvent, aObserver) { this.mObsMan.removeObserver(aEvent, aObserver); },

  
  

  onItemSelected: function()
  {
    
    viewer.pane.panelset.updateAllCommands();
  }
};




function ComputedStyleView(aObject)
{
  var view = aObject.ownerDocument.defaultView;
  this.mStyleList = view.getComputedStyle(aObject, "");
  this.mRowCount = this.mStyleList.length;
}

ComputedStyleView.prototype = new inBaseTreeView();

ComputedStyleView.prototype.getCellText = 
function getCellText(aRow, aCol) 
{
  var prop = this.mStyleList.item(aRow);
  if (aCol.id == "olcStyleName") {
    return prop;
  } else if (aCol.id == "olcStyleValue") {
    return this.mStyleList.getPropertyValue(prop);
  }
  
  return null;
}







ComputedStyleView.prototype.getRowObjectFromIndex = 
function getRowObjectFromIndex(aIndex)
{
  var prop = this.mStyleList.item(aIndex);
  return new CSSDeclaration(prop, this.mStyleList.getPropertyValue(prop));
}
