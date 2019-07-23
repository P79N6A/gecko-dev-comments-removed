















































var viewer;




const kObserverServiceCID = "@mozilla.org/observer-service;1";
const kAccessibleRetrievalCID = "@mozilla.org/accessibleRetrieval;1";

const nsIObserverService = Components.interfaces.nsIObserverService;
const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;
const nsIAccessibleEvent = Components.interfaces.nsIAccessibleEvent;
const nsIAccessNode = Components.interfaces.nsIAccessNode;




window.addEventListener("load", AccessibleEventsViewer_initialize, false);

function AccessibleEventsViewer_initialize()
{
  viewer = new AccessibleEventsViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function AccessibleEventsViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);

  this.mTree = document.getElementById("olAccessibleEvents");
  this.mOlBox = this.mTree.treeBoxObject;
}

AccessibleEventsViewer.prototype =
{
  

  mSubject: null,
  mPane: null,
  mView: null,

  

  get uid() { return "accessibleEvents"; },
  get pane() { return this.mPane; },
  get selection() { return this.mSelection; },

  get subject() { return this.mSubject; },
  set subject(aObject)
  {
    this.mView = new AccessibleEventsView(aObject);
    this.mOlBox.view = this.mView;
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  initialize: function initialize(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function destroy()
  {
    this.mView.destroy();
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
  },

  clearEventsList: function clearEventsList()
  {
    this.mView.clear();
  }
};




function AccessibleEventsView(aDocument)
{
  this.mDocument = aDocument;
  this.mEvents = [];
  this.mRowCount = 0;

  this.mAccService = XPCU.getService(kAccessibleRetrievalCID,
                                     nsIAccessibleRetrieval);

  this.mAccDocument = this.mAccService.getAccessibleFor(this.mDocument);
  this.mObserverService = XPCU.getService(kObserverServiceCID,
                                          nsIObserverService);

  this.mObserverService.addObserver(this, "accessible-event", false);
}

AccessibleEventsView.prototype = new inBaseTreeView();

AccessibleEventsView.prototype.observe =
function observe(aSubject, aTopic, aData)
{
  var event = XPCU.QI(aSubject, nsIAccessibleEvent);
  var accessible = event.accessible;
  if (!accessible)
    return;

  var accessnode = XPCU.QI(accessible, nsIAccessNode);
  var accDocument = accessnode.accessibleDocument;
  if (accDocument != this.mAccDocument)
    return;

  var type = event.eventType;
  var date = new Date();
  var node = accessnode.DOMNode;

  var eventObj = {
    event: event,
    accessnode: accessnode,
    node: node,
    nodename: node ? node.nodeName : "",
    type: this.mAccService.getStringEventType(type),
    time: date.toLocaleTimeString()
  };

  this.mEvents.unshift(eventObj);
  ++this.mRowCount;
  this.mTree.rowCountChanged(0, 1);
}

AccessibleEventsView.prototype.destroy =
function destroy()
{
  this.mObserverService.removeObserver(this, "accessible-event");
}

AccessibleEventsView.prototype.clear =
function clear()
{
  var count = this.mRowCount;
  this.mRowCount = 0;
  this.mEvents = [];
  this.mTree.rowCountChanged(0, -count);
}

AccessibleEventsView.prototype.getDOMNode =
function getDOMNode(aRow)
{
  return this.mEvents[aRow].node;
}

AccessibleEventsView.prototype.getCellText =
function getCellText(aRow, aCol)
{
  if (aCol.id == "olcEventType")
    return this.mEvents[aRow].type;
  if (aCol.id == "olcEventTime")
    return this.mEvents[aRow].time;
  if (aCol.id == "olcEventTargetNodeName")
    return this.mEvents[aRow].nodename;
  return "";
}

