




































 
 



var viewer;
var gBundle;




const kAccessibleRetrievalCID = "@mozilla.org/accessibleRetrieval;1";

const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;
const nsIAccessible = Components.interfaces.nsIAccessible;

const nsIPropertyElement = Components.interfaces.nsIPropertyElement;




window.addEventListener("load", AccessiblePropsViewer_initialize, false);

function AccessiblePropsViewer_initialize()
{
  gBundle = document.getElementById("accessiblePropsBundle");

  viewer = new AccessiblePropsViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}



function AccessiblePropsViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);
  this.mAccService = XPCU.getService(kAccessibleRetrievalCID,
                                     nsIAccessibleRetrieval);
}

AccessiblePropsViewer.prototype =
{
  mSubject: null,
  mPane: null,
  mAccSubject: null,
  mAccService: null,

  get uid() { return "accessibleProps" },
  get pane() { return this.mPane },

  get subject() { return this.mSubject },
  set subject(aObject)
  {
    this.mSubject = aObject;
    this.updateView();
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  initialize: function initialize(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  isCommandEnabled: function isCommandEnabled(aCommand)
  {
    return false;
  },
  
  getCommand: function getCommand(aCommand)
  {
    return null;
  },

  destroy: function destroy() {},

  

  addObserver: function addObserver(aEvent, aObserver)
  {
    this.mObsMan.addObserver(aEvent, aObserver);
  },
  removeObserver: function removeObserver(aEvent, aObserver)
  {
    this.mObsMan.removeObserver(aEvent, aObserver);
  },

  
  updateView: function updateView()
  {
    this.clearView();

    try {
      this.mAccSubject = this.mSubject.getUserData("accessible");
      if (this.mAccSubject)
        XPCU.QI(this.mAccSubject, nsIAccessible);
      else
        this.mAccSubject = this.mAccService.getAccessibleFor(this.mSubject);
    } catch(e) {
      dump("Failed to get accessible object for node.");
      return;
    }

    
    var containers = document.getElementsByAttribute("prop", "*");
    for (var i = 0; i < containers.length; ++i) {
      var value = "";
      try {
        var prop = containers[i].getAttribute("prop");
        value = this[prop];
      } catch (e) {
        dump("Accessibility " + prop + " property is not available.\n");
      }

      if (value instanceof Array)
        containers[i].textContent = value.join(", ");
      else
        containers[i].textContent = value;
    }

    
    var x = { value: 0 };
    var y = { value: 0 };
    var width = {value: 0 };
    var height = {value: 0 };
    this.mAccSubject.getBounds(x, y, width, height);

    document.getElementById("bounds-x").textContent =
      gBundle.getFormattedString("accBoundsX", [x.value]);
    document.getElementById("bounds-y").textContent =
      gBundle.getFormattedString("accBoundsY", [y.value]);
    document.getElementById("bounds-width").textContent =
      gBundle.getFormattedString("accBoundsWidth", [width.value]);
    document.getElementById("bounds-height").textContent =
      gBundle.getFormattedString("accBoundsHeight", [height.value]);

    
    var attrs = this.mAccSubject.attributes;
    if (attrs) {
      var enumerate = attrs.enumerate();
      while (enumerate.hasMoreElements())
        this.addAccessibleAttribute(enumerate.getNext());
    }
  },

  addAccessibleAttribute: function addAccessibleAttribute(aElement)
  {
    var prop = XPCU.QI(aElement, nsIPropertyElement);

    var trAttrBody = document.getElementById("trAttrBody");

    var ti = document.createElement("treeitem");
    var tr = document.createElement("treerow");

    var tc = document.createElement("treecell");
    tc.setAttribute("label", prop.key);
    tr.appendChild(tc);

    tc = document.createElement("treecell");
    tc.setAttribute("label", prop.value);
    tr.appendChild(tc);

    ti.appendChild(tr);

    trAttrBody.appendChild(ti);
  },

  removeAccessibleAttributes: function removeAccessibleAttributes()
  {
    var trAttrBody = document.getElementById("trAttrBody");
    while (trAttrBody.hasChildNodes())
      trAttrBody.removeChild(trAttrBody.lastChild)
  },

  clearView: function clearView()
  {
    var containers = document.getElementsByAttribute("prop", "*");
    for (var i = 0; i < containers.length; ++i)
      containers[i].textContent = "";

    this.removeAccessibleAttributes();
  },

  get role()
  {
    return this.mAccService.getStringRole(this.mAccSubject.finalRole);
  },

  get name()
  {
    return this.mAccSubject.name;
  },

  get description()
  {
    return this.mAccSubject.description;
  },

  get value()
  {
    return this.mAccSubject.value;
  },

  get state()
  {
    var stateObj = {value: null};
    var extStateObj = {value: null};
    this.mAccSubject.getFinalState(stateObj, extStateObj);

    var list = [];

    states = this.mAccService.getStringStates(stateObj.value, extStateObj.value);

    for (var i = 0; i < states.length; i++)
      list.push(states.item(i));
    return list;
  },

  get actionNames()
  {
    var list = [];

    var count = this.mAccSubject.numActions;
    for (var i = 0; i < count; i++)
      list.push(this.mAccSubject.getActionName(i));
    return list;
  }
};
