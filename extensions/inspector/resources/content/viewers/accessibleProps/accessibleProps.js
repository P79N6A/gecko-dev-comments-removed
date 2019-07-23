




































 
 



var viewer;




window.addEventListener("load", AccessiblePropsViewer_initialize, false);

function AccessiblePropsViewer_initialize()
{
  viewer = new AccessiblePropsViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}



function AccessiblePropsViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);
  this.mAccService = Components.classes["@mozilla.org/accessibleRetrieval;1"]
                      .getService(Components.interfaces.nsIAccessibleRetrieval);
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

    var attrs = this.mAccSubject.attributes;
    if (attrs) {
      var enumerate = attrs.enumerate();
      while (enumerate.hasMoreElements())
        this.addAccessibleAttribute(enumerate.getNext());
    }
  },

  addAccessibleAttribute: function addAccessibleAttribute(aElement)
  {
    var prop = aElement.QueryInterface(Components.interfaces.nsIPropertyElement);

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
