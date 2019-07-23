















































var viewer;
var gAccService = null;




const kAccessibleRetrievalCID = "@mozilla.org/accessibleRetrieval;1";

const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;
const nsIAccessibleRelation = Components.interfaces.nsIAccessibleRelation;
const nsIAccessNode = Components.interfaces.nsIAccessNode;
const nsIAccessible = Components.interfaces.nsIAccessible;




window.addEventListener("load", AccessibleRelationsViewer_initialize, false);

function AccessibleRelationsViewer_initialize()
{
  gAccService = XPCU.getService(kAccessibleRetrievalCID,
                                nsIAccessibleRetrieval);

  viewer = new AccessibleRelationsViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function AccessibleRelationsViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);

  this.mTree = document.getElementById("olAccessibleRelations");
  this.mTreeBox = this.mTree.treeBoxObject;

  this.mTargetsTree = document.getElementById("olAccessibleTargets");
  this.mTargetsTreeBox = this.mTargetsTree.treeBoxObject;
}

AccessibleRelationsViewer.prototype =
{
  
  

  mSubject: null,
  mPane: null,
  mView: null,
  mTargetsView: null,

  
  

  get uid() { return "accessibleRelations"; },
  get pane() { return this.mPane; },
  get selection() { return this.mSelection; },

  get subject() { return this.mSubject; },
  set subject(aObject)
  {
    this.mView = new AccessibleRelationsView(aObject);
    this.mTreeBox.view = this.mView;
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  initialize: function initialize(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function destroy()
  {
    this.mTreeBox.view = null;
    this.mTargetsTreeBox.view = null;
  },

  isCommandEnabled: function isCommandEnabled(aCommand)
  {
    return false;
  },

  getCommand: function getCommand(aCommand)
  {
    return null;
  },

  
  

  addObserver: function addObserver(aEvent, aObserver)
  {
    this.mObsMan.addObserver(aEvent, aObserver);
  },
  removeObserver: function removeObserver(aEvent, aObserver)
  {
    this.mObsMan.removeObserver(aEvent, aObserver);
  },

  
  

  onItemSelected: function onItemSelected()
  {
    var idx = this.mTree.currentIndex;
    var relation = this.mView.getRelationObject(idx);
    this.mTargetsView = new AccessibleTargetsView(relation);
    this.mTargetsTreeBox.view = this.mTargetsView;
  },

  cmdInspectInNewView: function cmdInspectInNewView()
  {
    var idx = this.mTargetsTree.currentIndex;
    if (idx >= 0) {
      var node = this.mTargetsView.getDOMNode(idx);
      if (node)
        inspectObject(node);
    }
  }
};




function AccessibleRelationsView(aNode)
{
  this.mNode = aNode;

  this.mAccessible = aNode.getUserData("accessible");
  if (this.mAccessible)
    XPCU.QI(this.mAccessible, nsIAccessible);
  else
    this.mAccessible = gAccService.getAccessibleFor(aNode);

  this.mRelations = this.mAccessible.getRelations();
}

AccessibleRelationsView.prototype = new inBaseTreeView();

AccessibleRelationsView.prototype.__defineGetter__("rowCount",
function rowCount()
{
  return this.mRelations.length;
});

AccessibleRelationsView.prototype.getRelationObject =
function getRelationObject(aRow)
{
  return this.mRelations.queryElementAt(aRow, nsIAccessibleRelation);
}

AccessibleRelationsView.prototype.getCellText =
function getCellText(aRow, aCol)
{
  if (aCol.id == "olcRelationType") {
    var relation = this.getRelationObject(aRow);
    if (relation)
      return gAccService.getStringRelationType(relation.relationType);
  }

  return "";
}




function AccessibleTargetsView(aRelation)
{
  this.mRelation = aRelation;
  this.mTargets = this.mRelation.getTargets();
}




AccessibleTargetsView.prototype = new inBaseTreeView();

AccessibleTargetsView.prototype.__defineGetter__("rowCount",
function rowCount()
{
  return this.mTargets.length;
});

AccessibleTargetsView.prototype.getCellText =
function getCellText(aRow, aCol)
{
  if (aCol.id == "olcRole") {
    var accessible = this.getAccessible(aRow);
    if (accessible)
      return gAccService.getStringRole(accessible.finalRole);
  } else if (aCol.id == "olcNodeName") {
    var node = this.getDOMNode(aRow);
    if (node)
      return node.nodeName;
  }

  return "";
}




AccessibleTargetsView.prototype.getAccessible =
function getAccessible(aRow)
{
  return this.mTargets.queryElementAt(aRow, nsIAccessible);
}

AccessibleTargetsView.prototype.getDOMNode =
function getDOMNode(aRow)
{
  var accessNode = this.mTargets.queryElementAt(aRow, nsIAccessNode);
  if (!accessNode)
    return null;

  var DOMNode = accessNode.DOMNode;
  DOMNode.setUserData("accessible", accessNode, null);
  return DOMNode;
}

