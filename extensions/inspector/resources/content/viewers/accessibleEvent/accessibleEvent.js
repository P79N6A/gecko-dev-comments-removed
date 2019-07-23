



































 
 



var viewer;




const kAccessibleRetrievalCID = "@mozilla.org/accessibleRetrieval;1";

const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;

const nsIAccessibleEvent = Components.interfaces.nsIAccessibleEvent;
const nsIAccessibleStateChangeEvent =
  Components.interfaces.nsIAccessibleStateChangeEvent;
const nsIAccessibleTextChangeEvent =
  Components.interfaces.nsIAccessibleTextChangeEvent;
const nsIAccessibleCaretMoveEvent =
  Components.interfaces.nsIAccessibleCaretMoveEvent;




window.addEventListener("load", AccessibleEventViewer_initialize, false);

function AccessibleEventViewer_initialize()
{
  viewer = new AccessibleEventViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}



function AccessibleEventViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);
  this.mAccService = XPCU.getService(kAccessibleRetrievalCID,
                                     nsIAccessibleRetrieval);
}

AccessibleEventViewer.prototype =
{
  mSubject: null,
  mPane: null,
  mAccEventSubject: null,
  mAccService: null,

  get uid() { return "accessibleEvent"; },
  get pane() { return this.mPane; },

  get subject() { return this.mSubject; },
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

    this.mAccEventSubject = this.mSubject.getUserData("accessibleEvent");
    if (!this.mAccEventSubject)
      return;

    XPCU.QI(this.mAccEventSubject, nsIAccessibleEvent);

    var shownPropsId = "";
    if (this.mAccEventSubject instanceof nsIAccessibleStateChangeEvent)
      shownPropsId = "stateChangeEvent";
    else if (this.mAccEventSubject instanceof nsIAccessibleTextChangeEvent)
      shownPropsId = "textChangeEvent";
    else if (this.mAccEventSubject instanceof nsIAccessibleCaretMoveEvent)
      shownPropsId = "caretMoveEvent";

    var props = document.getElementsByAttribute("prop", "*");
    for (var i = 0; i < props.length; i++) {
      var propElm = props[i];
      var isActive = !propElm.hasAttribute("class") ||
                     (propElm.getAttribute("class") == shownPropsId);

      if (isActive) {
        var prop = propElm.getAttribute("prop");
        propElm.textContent = this[prop];
        propElm.parentNode.removeAttribute("hidden");
      } else {
        propElm.parentNode.setAttribute("hidden", "true");
      }
    }
  },

  clearView: function clearView()
  {
    var containers = document.getElementsByAttribute("prop", "*");
    for (var i = 0; i < containers.length; ++i)
      containers[i].textContent = "";
  },

  get isFromUserInput()
  {
    return this.mAccEventSubject.isFromUserInput;
  },

  get state()
  {
    var accessible = this.mAccEventSubject.accessible;

    var stateObj = {value: null};
    var extStateObj = {value: null};
    accessible.getFinalState(stateObj, extStateObj);

    var list = [];

    states = this.mAccService.getStringStates(stateObj.value,
                                              extStateObj.value);

    for (var i = 0; i < states.length; i++)
      list.push(states.item(i));
    return list.join();
  },

  get isEnabled()
  {
    return this.mAccEventSubject.isEnabled();
  },

  get startOffset()
  {
    return this.mAccEventSubject.start;
  },

  get length()
  {
    return this.mAccEventSubject.length;
  },

  get isInserted()
  {
    return this.mAccEventSubject.isInserted();
  },

  get modifiedText()
  {
    return this.mAccEventSubject.modifiedText;
  },

  get caretOffset()
  {
    return this.mAccEventSubject.caretOffset;
  }
};

