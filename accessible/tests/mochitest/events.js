


const EVENT_HIDE = nsIAccessibleEvent.EVENT_HIDE;
const EVENT_SHOW = nsIAccessibleEvent.EVENT_SHOW;
const EVENT_DOCUMENT_LOAD_COMPLETE =
  nsIAccessibleEvent.EVENT_DOCUMENT_LOAD_COMPLETE;
const EVENT_FOCUS = nsIAccessibleEvent.EVENT_FOCUS;
const EVENT_NAME_CHANGE = nsIAccessibleEvent.EVENT_NAME_CHANGE;
const EVENT_SCROLLING_START = nsIAccessibleEvent.EVENT_SCROLLING_START;
const EVENT_STATE_CHANGE = nsIAccessibleEvent.EVENT_STATE_CHANGE;
const EVENT_REORDER = nsIAccessibleEvent.EVENT_REORDER;







var gA11yEventDumpID = "";












function waitForEvent(aEventType, aTarget, aFunc, aContext, aArg1, aArg2)
{
  var handler = {
    handleEvent: function handleEvent(aEvent) {

      if (aTarget) {
        if (aTarget instanceof nsIAccessible &&
            aTarget != aEvent.accessible)
          return;

        if (aTarget instanceof nsIDOMNode &&
            aTarget != aEvent.DOMNode)
          return;
      }

      unregisterA11yEventListener(aEventType, this);

      window.setTimeout(
        function ()
        {
          aFunc.call(aContext, aArg1, aArg2);
        },
        0
      );
    }
  };

  registerA11yEventListener(aEventType, handler);
}











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








const INVOKER_ACTION_FAILED = 1;





const DO_NOT_FINISH_TEST = 1;




function invokerChecker(aEventType, aTarget)
{
  this.type = aEventType;
  this.target = aTarget;
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

    
    
    this.processNextInvokerInTimeout(true);
  }

  



  this.onFinish = function eventQueue_finish()
  {
  }

  

  


  this.processNextInvoker = function eventQueue_processNextInvoker()
  {
    
    var testFailed = false;

    var invoker = this.getInvoker();
    if (invoker) {
      if ("finalCheck" in invoker)
        invoker.finalCheck();

      if (invoker.wasCaught) {
        for (var idx = 0; idx < invoker.wasCaught.length; idx++) {
          var id = this.getEventID(idx);
          var type = this.getEventType(idx);
          var unexpected = this.mEventSeq[idx].unexpected;

          var typeStr = (typeof type == "string") ?
            type : gAccRetrieval.getStringEventType(type);

          var msg = "test with ID = '" + id + "' failed. ";
          if (unexpected) {
            var wasCaught = invoker.wasCaught[idx];
            if (!testFailed)
              testFailed = wasCaught;

            ok(!wasCaught,
               msg + "There is unexpected " + typeStr + " event.");

          } else {
            var wasCaught = invoker.wasCaught[idx];
            if (!testFailed)
              testFailed = !wasCaught;

            ok(wasCaught,
               msg + "No " + typeStr + " event.");
          }
        }
      } else {
        testFailed = true;
        for (var idx = 0; idx < this.mEventSeq.length; idx++) {
          var id = this.getEventID(idx);
          ok(false,
             "test with ID = '" + id + "' failed. No events were registered.");
        }
      }
    }

    this.clearEventHandler();

    
    if (testFailed || this.mIndex == this.mInvokers.length - 1) {
      gA11yEventApplicantsCount--;
      listenA11yEvents(false);

      var res = this.onFinish();
      if (res != DO_NOT_FINISH_TEST)
        SimpleTest.finish();

      return;
    }

    
    invoker = this.getNextInvoker();

    this.setEventHandler(invoker);

    if (invoker.invoke() == INVOKER_ACTION_FAILED) {
      
      this.processNextInvoker();
      return;
    }

    if (this.areAllEventsUnexpected())
      this.processNextInvokerInTimeout(true);
  }

  this.processNextInvokerInTimeout = function eventQueue_processNextInvokerInTimeout(aUncondProcess)
  {
    if (!aUncondProcess && this.areAllEventsExpected()) {
      
      var queue = this;
      SimpleTest.executeSoon(function() { queue.processNextInvoker(); });
      return;
    }

    
    window.setTimeout(function(aQueue) { aQueue.processNextInvoker(); }, 500,
                      this);
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

    
    for (var idx = 0; idx < this.mEventSeq.length; idx++) {
      if (this.mEventSeq[idx].unexpected && this.compareEvents(idx, aEvent))
        invoker.wasCaught[idx] = true;
    }

    

    
    for (var idx = this.mEventSeqIdx + 1;
         idx < this.mEventSeq.length && this.mEventSeq[idx].unexpected; idx++);

    if (idx == this.mEventSeq.length) {
      
      this.processNextInvokerInTimeout();
      return;
    }

    var matched = this.compareEvents(idx, aEvent);
    this.dumpEventToDOM(aEvent, idx, matched);

    if (matched) {
      this.checkEvent(idx, aEvent);
      invoker.wasCaught[idx] = true;

      
      if (idx == this.mEventSeq.length - 1) {
        this.processNextInvokerInTimeout();
        return;
      }

      this.mEventSeqIdx = idx;
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
      aInvoker.eventSeq :
      [ new invokerChecker(this.mDefEventType, aInvoker.DOMNode) ];

    for (var idx = 0; idx < this.mEventSeq.length; idx++)
      this.mEventSeq[idx].unexpected = false;

    var unexpectedSeq = aInvoker.unexpectedEventSeq;
    if (unexpectedSeq) {
      for (var idx = 0; idx < unexpectedSeq.length; idx++)
        unexpectedSeq[idx].unexpected = true;

      this.mEventSeq = this.mEventSeq.concat(unexpectedSeq);
    }

    this.mEventSeqIdx = -1;

    
    if (this.mEventSeq) {
      aInvoker.wasCaught = new Array(this.mEventSeq.length);

      for (var idx = 0; idx < this.mEventSeq.length; idx++) {
        var eventType = this.getEventType(idx);
        if (typeof eventType == "string") {
          
          var target = this.getEventTarget(idx);
          var phase = this.getEventPhase(idx);
          target.ownerDocument.addEventListener(eventType, this, phase);

        } else {
          
          addA11yEventListener(eventType, this);
        }
      }
    }
  }

  this.clearEventHandler = function eventQueue_clearEventHandler()
  {
    if (this.mEventSeq) {
      for (var idx = 0; idx < this.mEventSeq.length; idx++) {
        var eventType = this.getEventType(idx);
        if (typeof eventType == "string") {
          
          var target = this.getEventTarget(idx);
          var phase = this.getEventPhase(idx);
          target.ownerDocument.removeEventListener(eventType, this, phase);

        } else {
          
          removeA11yEventListener(eventType, this);
        }
      }

      this.mEventSeq = null;
    }
  }

  this.getEventType = function eventQueue_getEventType(aIdx)
  {
    return this.mEventSeq[aIdx].type;
  }

  this.getEventTarget = function eventQueue_getEventTarget(aIdx)
  {
    return this.mEventSeq[aIdx].target;
  }

  this.getEventPhase = function eventQueue_getEventPhase(aIdx)
  {
     var eventItem = this.mEventSeq[aIdx];
    if ("phase" in eventItem)
      return eventItem.phase;

    return true;
  }

  this.getEventID = function eventQueue_getEventID(aIdx)
  {
    var eventItem = this.mEventSeq[aIdx];
    if ("getID" in eventItem)
      return eventItem.getID();
    
    var invoker = this.getInvoker();
    return invoker.getID();
  }

  this.compareEvents = function eventQueue_compareEvents(aIdx, aEvent)
  {
    var eventType1 = this.getEventType(aIdx);

    var eventType2 = (aEvent instanceof nsIDOMEvent) ?
      aEvent.type : aEvent.eventType;

    if (eventType1 != eventType2)
      return false;

    var target1 = this.getEventTarget(aIdx);
    if (target1 instanceof nsIAccessible) {
      var target2 = (aEvent instanceof nsIDOMEvent) ?
        getAccessible(aEvent.target) : aEvent.accessible;

      return target1 == target2;
    }

    
    
    var target2 = (aEvent instanceof nsIDOMEvent) ?
      aEvent.originalTarget : aEvent.DOMNode;
    return target1 == target2;
  }

  this.checkEvent = function eventQueue_checkEvent(aIdx, aEvent)
  {
    var eventItem = this.mEventSeq[aIdx];
    if ("check" in eventItem)
      eventItem.check(aEvent);

    var invoker = this.getInvoker();
    if ("check" in invoker)
      invoker.check(aEvent);
  }

  this.areAllEventsExpected = function eventQueue_areAllEventsExpected()
  {
    for (var idx = 0; idx < this.mEventSeq.length; idx++) {
      if (this.mEventSeq[idx].unexpected)
        return false;
    }

    return true;
  }

  this.areAllEventsUnexpected = function eventQueue_areAllEventsUnxpected()
  {
    for (var idx = 0; idx < this.mEventSeq.length; idx++) {
      if (!this.mEventSeq[idx].unexpected)
        return false;
    }

    return true;
  }

  this.dumpEventToDOM = function eventQueue_dumpEventToDOM(aOrigEvent,
                                                           aExpectedEventIdx,
                                                           aMatch)
  {
    if (!gA11yEventDumpID) 
      return;

    
    
    if (aOrigEvent instanceof nsIDOMEvent) {
      var info = "Event type: " + aOrigEvent.type;
      info += ". Target: " + prettyName(aOrigEvent.originalTarget);
      dumpInfoToDOM(info);
    }

    var currType = this.getEventType(aExpectedEventIdx);
    var currTarget = this.getEventTarget(aExpectedEventIdx);

    var info = "EQ: " + (aMatch ? "matched" : "expected") + " event, type: ";
    info += (typeof currType == "string") ?
      currType : eventTypeToString(currType);
    info += ". Target: " + prettyName(currTarget);

    dumpInfoToDOM(info);
  }

  this.mDefEventType = aEventType;

  this.mInvokers = new Array();
  this.mIndex = -1;

  this.mEventSeq = null;
  this.mEventSeqIdx = -1;
}






function sequence()
{
  













  this.append = function sequence_append(aProcessor, aEventType, aTarget,
                                         aItemID)
  {
    var item = new sequenceItem(aProcessor, aEventType, aTarget, aItemID);
    this.items.push(item);
  }

  


  this.processNext = function sequence_processNext()
  {
    this.idx++;
    if (this.idx >= this.items.length) {
      ok(false, "End of sequence: nothing to process!");
      SimpleTest.finish();
      return;
    }

    this.items[this.idx].startProcess();
  }

  this.items = new Array();
  this.idx = -1;
}










var gObserverService = null;

var gA11yEventListeners = {};
var gA11yEventApplicantsCount = 0;

var gA11yEventObserver =
{
  observe: function observe(aSubject, aTopic, aData)
  {
    if (aTopic != "accessible-event")
      return;

    var event = aSubject.QueryInterface(nsIAccessibleEvent);
    var listenersArray = gA11yEventListeners[event.eventType];

    if (gA11yEventDumpID) { 
      var target = event.DOMNode;
      var dumpElm = document.getElementById(gA11yEventDumpID);

      var parent = target;
      while (parent && parent != dumpElm)
        parent = parent.parentNode;

      if (parent != dumpElm) {
        var type = eventTypeToString(event.eventType);
        var info = "Event type: " + type;
        info += ". Target: " + prettyName(event.accessible);

        if (listenersArray)
          info += ". Listeners count: " + listenersArray.length;

        dumpInfoToDOM(info);
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








function dumpInfoToDOM(aInfo, aDumpNode)
{
  var dumpID = gA11yEventDumpID ? gA11yEventDumpID : aDumpNode;
  if (!dumpID)
    return;

  var dumpElm = document.getElementById(dumpID);

  var containerTagName = document instanceof nsIDOMHTMLDocument ?
    "div" : "description";
  var container = document.createElement(containerTagName);

  container.textContent = aInfo;
  dumpElm.appendChild(container);
}








function sequenceItem(aProcessor, aEventType, aTarget, aItemID)
{
  
  
  this.startProcess = function sequenceItem_startProcess()
  {
    this.queue.invoke();
  }
  
  var item = this;
  
  this.queue = new eventQueue();
  this.queue.onFinish = function()
  {
    aProcessor.onProcessed();
    return DO_NOT_FINISH_TEST;
  }
  
  var invoker = {
    invoke: function invoker_invoke() {
      return aProcessor.process();
    },
    getID: function invoker_getID()
    {
      return aItemID;
    },
    eventSeq: [ new invokerChecker(aEventType, aTarget) ]
  };
  
  this.queue.push(invoker);
}
