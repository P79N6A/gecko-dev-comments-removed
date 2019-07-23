















































var viewer;




const kAccessibleRetrievalCID = "@mozilla.org/accessibleRetrieval;1";

const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;
const nsIAccessibleEvent = Components.interfaces.nsIAccessibleEvent;
const nsIAccessible = Components.interfaces.nsIAccessible;
const nsIAccessNode = Components.interfaces.nsIAccessNode;




window.addEventListener("load", AccessibleTreeViewer_initialize, false);

function AccessibleTreeViewer_initialize()
{
  viewer = new AccessibleTreeViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function AccessibleTreeViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);

  this.mTree = document.getElementById("olAccessibleTree");
  this.mOlBox = this.mTree.treeBoxObject;
}

AccessibleTreeViewer.prototype =
{
  

  mSubject: null,
  mPane: null,
  mView: null,

  

  get uid() { return "accessibleTree"; },
  get pane() { return this.mPane; },
  get selection() { return this.mSelection; },

  get subject() { return this.mSubject; },
  set subject(aObject)
  {
    this.mView = new inAccTreeView(aObject);
    this.mOlBox.view = this.mView;
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
    this.mView.selection.select(0);
  },

  initialize: function initialize(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function destroy()
  {
    this.mOlBox.view = null;
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
    this.mSelection = this.mView.getDOMNode(idx);
    this.mObsMan.dispatchEvent("selectionChange",
                               { selection: this.mSelection } );
  }
};




function inAccTreeView(aDocument)
{
  this.mNodes = [];

  this.mAccService = XPCU.getService(kAccessibleRetrievalCID,
                                     nsIAccessibleRetrieval);

  this.mDocument = aDocument;
  this.mAccDocument = this.mAccService.getAccessibleFor(aDocument);

  var node = this.createNode(this.mAccDocument);
  this.mNodes.push(node);
}




inAccTreeView.prototype = new inBaseTreeView();

inAccTreeView.prototype.__defineGetter__("rowCount",
function rowCount()
{
  return this.mNodes.length;
});

inAccTreeView.prototype.getCellText =
function getCellText(aRow, aCol)
{
  var node = this.rowToNode(aRow);
  if (!node)
    return "";

  var accessible = node.accessible;

  if (aCol.id == "olcRole")
    return this.mAccService.getStringRole(accessible.finalRole);

  if (aCol.id == "olcName")
    return accessible.name;

  if (aCol.id == "olcNodeName") {
    var node = this.getDOMNodeFor(accessible);
    return node ? node.nodeName : "";
  }

  return "";
}

inAccTreeView.prototype.isContainer =
function isContainer(aRow)
{
  var node = this.rowToNode(aRow);
  return node ? node.isContainer : false;
}

inAccTreeView.prototype.isContainerOpen =
function isContainerOpen(aRow)
{
  var node = this.rowToNode(aRow);
  return node ? node.isOpen : false;
}

inAccTreeView.prototype.isContainerEmpty =
function isContainerEmpty(aRow)
{
  return !this.isContainer(aRow);
}

inAccTreeView.prototype.getLevel =
function getLevel(aRow)
{
  var node = this.rowToNode(aRow);
  return node ? node.level : 0;
}

inAccTreeView.prototype.getParentIndex =
function getParentIndex(aRow)
{
  var node = this.rowToNode(aRow);
  if (!node)
    return -1;

  var checkNode = null;
  var i = aRow - 1;
  do {
    checkNode = this.rowToNode(i);
    if (!checkNode)
      return -1;

    if (checkNode == node.parent)
      return i;
    --i;
  } while (checkNode);

  return -1;
}

inAccTreeView.prototype.hasNextSibling =
function hasNextSibling(aRow, aAfterRow)
{
  var node = this.rowToNode(aRow);
  return node && (node.next != null);
}

inAccTreeView.prototype.toggleOpenState =
function toggleOpenState(aRow)
{
  var node = this.rowToNode(aRow);
  if (!node)
    return;

  var oldCount = this.rowCount;
  if (node.isOpen)
    this.collapseNode(aRow);
  else
    this.expandNode(aRow);

  this.mTree.invalidateRow(aRow);
  this.mTree.rowCountChanged(aRow + 1, this.rowCount - oldCount);
}









inAccTreeView.prototype.expandNode =
function expandNode(aRow)
{
  var node = this.rowToNode(aRow);
  if (!node)
    return;

  var kids = node.accessible.children;
  var kidCount = kids.length;

  var newNode = null;
  var prevNode = null;

  for (var i = 0; i < kidCount; ++i) {
    var accessible = kids.queryElementAt(i, nsIAccessible);
    newNode = this.createNode(accessible, node);
    this.mNodes.splice(aRow + i + 1, 0, newNode);

    if (prevNode)
      prevNode.next = newNode;
    newNode.previous = prevNode;
    prevNode = newNode;
  }

  node.isOpen = true;
}






inAccTreeView.prototype.collapseNode =
function collapseNode(aRow)
{
  var node = this.rowToNode(aRow);
  if (!node)
    return;

  var row = this.getLastDescendantOf(node, aRow);
  this.mNodes.splice(aRow + 1, row - aRow);

  node.isOpen = false;
}








inAccTreeView.prototype.createNode =
function createNode(aAccessible, aParent)
{
  var node = new inAccTreeViewNode(aAccessible);
  node.level = aParent ? aParent.level + 1 : 0;
  node.parent = aParent;
  node.isContainer = aAccessible.children.length > 0;

  return node;
}








inAccTreeView.prototype.getLastDescendantOf =
function getLastDescendantOf(aNode, aRow)
{
  var rowCount = this.rowCount;

  var row = aRow + 1;
  for (; row < rowCount; ++row) {
    if (this.mNodes[row].level <= aNode.level)
      return row - 1;
  }

  return rowCount - 1;
}






inAccTreeView.prototype.rowToNode =
function rowToNode(aRow)
{
  if (aRow < 0 || aRow >= this.rowCount)
    return null;

  return this.mNodes[aRow];
}









inAccTreeView.prototype.getDOMNodeFor =
function getDOMNodeFor(aAccessible)
{
  var accessNode = XPCU.QI(aAccessible, nsIAccessNode);
  return accessNode.DOMNode;
}







inAccTreeView.prototype.getDOMNode =
function getDOMNode(aRow)
{
  var node = this.mNodes[aRow];
  if (!node)
    return null;

  return this.getDOMNodeFor(node.accessible);
}




function inAccTreeViewNode(aAccessible)
{
  this.accessible = aAccessible;

  this.parent = null;
  this.next = null;
  this.previous = null;

  this.level = 0;
  this.isOpen = false;
  this.isContainer = false;
}

