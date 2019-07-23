












function registerA11yEventListener(aEventType, aEventHandler)
{
  listenA11yEvents(true);

  gA11yEventApplicantsCount++;
  addA11yEventListener(aEventType, aEventHandler);
}






function unregisterA11yEventListener(aEventType, aEventHandler)
{
  removeA11yEventListener(aEventType, aEventHandler);

  gA11yEventApplicantsCount--;
  listenA11yEvents(false);
}












































function eventQueue(aEventType)
{
  

  


  this.push = function eventQueue_push(aEventInvoker)
  {
    this.mInvokers.push(aEventInvoker);
  }

  


  this.invoke = function eventQueue_invoke()
  {
    listenA11yEvents(true);
    gA11yEventApplicantsCount++;

    
    
    window.setTimeout(function(aQueue) { aQueue.processNextInvoker(); }, 500,
                      this);
  }

  

  


  this.processNextInvoker = function eventQueue_processNextInvoker()
  {
    
    var testFailed = false;

    var invoker = this.getInvoker();
    if (invoker) {
      var id = invoker.getID();

      if (invoker.wasCaught) {
        for (var jdx = 0; jdx < invoker.wasCaught.length; jdx++) {
          var seq = this.mEventSeq;
          var type = seq[jdx][0];
          var typeStr = gAccRetrieval.getStringEventType(type);

          var msg = "test with ID = '" + id + "' failed. ";
          if (invoker.doNotExpectEvents) {
            var wasCaught = invoker.wasCaught[jdx];
            if (!testFailed)
              testFailed = wasCaught;

            ok(!wasCaught,
               msg + "There is unexpected " + typeStr + " event.");

          } else {
            var wasCaught = invoker.wasCaught[jdx];
            if (!testFailed)
              testFailed = !wasCaught;

            ok(wasCaught,
               msg + "No " + typeStr + " event.");
          }
        }
      } else {
        testFailed = true;
        ok(false,
           "test with ID = '" + id + "' failed. No events were registered.");
      }
    }

    this.clearEventHandler();

    
    if (testFailed || this.mIndex == this.mInvokers.length - 1) {
      gA11yEventApplicantsCount--;
      listenA11yEvents(false);

      SimpleTest.finish();
      return;
    }

    
    invoker = this.getNextInvoker();

    this.setEventHandler(invoker);

    invoker.invoke();

    if (invoker.doNotExpectEvents) {
      
      window.setTimeout(function(aQueue) { aQueue.processNextInvoker(); }, 500,
                        this);
    }
  }

  


  this.handleEvent = function eventQueue_handleEvent(aEvent)
  {
    var invoker = this.getInvoker();
    if (!invoker) 
      return;

    if (!this.mEventSeq) {
      
      
      this.processNextInvoker();
      return;
    }

    if ("debugCheck" in invoker)
      invoker.debugCheck(aEvent);

    if (invoker.doNotExpectEvents) {
      
      
      for (var idx = 0; idx < this.mEventSeq.length; idx++) {
        if (aEvent.eventType == this.mEventSeq[idx][0] &&
            aEvent.DOMNode == this.mEventSeq[idx][1]) {
          invoker.wasCaught[idx] = true;
        }
      }
    } else {
      
      var idx = this.mEventSeqIdx + 1;

      if (aEvent.eventType == this.mEventSeq[idx][0] &&
          aEvent.DOMNode == this.mEventSeq[idx][1]) {

        if ("check" in invoker)
          invoker.check(aEvent);

        invoker.wasCaught[idx] = true;

        if (idx == this.mEventSeq.length - 1) {
          
          var queue = this;
          SimpleTest.executeSoon(function() { queue.processNextInvoker(); });
          return;
        }

        this.mEventSeqIdx = idx;
      }
    }
  }

  
  this.getInvoker = function eventQueue_getInvoker()
  {
    return this.mInvokers[this.mIndex];
  }

  this.getNextInvoker = function eventQueue_getNextInvoker()
  {
    return this.mInvokers[++this.mIndex];
  }

  this.setEventHandler = function eventQueue_setEventHandler(aInvoker)
  {
    this.mEventSeq = ("eventSeq" in aInvoker) ?
      aInvoker.eventSeq : [[this.mDefEventType, aInvoker.DOMNode]];
    this.mEventSeqIdx = -1;

    if (this.mEventSeq) {
      aInvoker.wasCaught = new Array(this.mEventSeq.length);
      
      for (var idx = 0; idx < this.mEventSeq.length; idx++)
        addA11yEventListener(this.mEventSeq[idx][0], this);
    }
  }

  this.clearEventHandler = function eventQueue_clearEventHandler()
  {
    if (this.mEventSeq) {
      for (var idx = 0; idx < this.mEventSeq.length; idx++)
        removeA11yEventListener(this.mEventSeq[idx][0], this);
      
      this.mEventSeq = null;
    }
  }

  this.mDefEventType = aEventType;

  this.mInvokers = new Array();
  this.mIndex = -1;

  this.mEventSeq = null;
  this.mEventSeqIdx = -1;
}









var gObserverService = null;

var gA11yEventListeners = {};
var gA11yEventApplicantsCount = 0;
var gA11yEventDumpID = ""; 

var gA11yEventObserver =
{
  observe: function observe(aSubject, aTopic, aData)
  {
    if (aTopic != "accessible-event")
      return;

    var event = aSubject.QueryInterface(nsIAccessibleEvent);
    var listenersArray = gA11yEventListeners[event.eventType];

    if (gA11yEventDumpID) { 
      var dumpElm = document.getElementById(gA11yEventDumpID);

      var target = event.DOMNode;

      var parent = target;
      while (parent && parent != dumpElm)
        parent = parent.parentNode;

      if (parent != dumpElm) {
        var type = gAccRetrieval.getStringEventType(event.eventType);
        var info = "event type: " + type + ", target: " + target.localName;
        if (listenersArray)
          info += ", registered listeners count is " + listenersArray.length;

        var div = document.createElement("div");      
        div.textContent = info;

        dumpElm.appendChild(div);
      }
    }

    if (!listenersArray)
      return;

    for (var index = 0; index < listenersArray.length; index++)
      listenersArray[index].handleEvent(event);
  }
};

function listenA11yEvents(aStartToListen)
{
  if (aStartToListen && !gObserverService) {
    gObserverService = Components.classes["@mozilla.org/observer-service;1"].
      getService(nsIObserverService);
    
    gObserverService.addObserver(gA11yEventObserver, "accessible-event",
                                 false);
  } else if (!gA11yEventApplicantsCount) {
    gObserverService.removeObserver(gA11yEventObserver,
                                    "accessible-event");
    gObserverService = null;
  }
}

function addA11yEventListener(aEventType, aEventHandler)
{
  if (!(aEventType in gA11yEventListeners))
    gA11yEventListeners[aEventType] = new Array();
  
  gA11yEventListeners[aEventType].push(aEventHandler);
}

function removeA11yEventListener(aEventType, aEventHandler)
{
  var listenersArray = gA11yEventListeners[aEventType];
  if (!listenersArray)
    return false;

  var index = listenersArray.indexOf(aEventHandler);
  if (index == -1)
    return false;

  listenersArray.splice(index, 1);
  
  if (!listenersArray.length) {
    gA11yEventListeners[aEventType] = null;
    delete gA11yEventListeners[aEventType];
  }

  return true;
}
