


const EVENT_ALERT = nsIAccessibleEvent.EVENT_ALERT;
const EVENT_DOCUMENT_LOAD_COMPLETE = nsIAccessibleEvent.EVENT_DOCUMENT_LOAD_COMPLETE;
const EVENT_DOCUMENT_RELOAD = nsIAccessibleEvent.EVENT_DOCUMENT_RELOAD;
const EVENT_DOCUMENT_LOAD_STOPPED = nsIAccessibleEvent.EVENT_DOCUMENT_LOAD_STOPPED;
const EVENT_HIDE = nsIAccessibleEvent.EVENT_HIDE;
const EVENT_FOCUS = nsIAccessibleEvent.EVENT_FOCUS;
const EVENT_NAME_CHANGE = nsIAccessibleEvent.EVENT_NAME_CHANGE;
const EVENT_MENU_START = nsIAccessibleEvent.EVENT_MENU_START;
const EVENT_MENU_END = nsIAccessibleEvent.EVENT_MENU_END;
const EVENT_MENUPOPUP_START = nsIAccessibleEvent.EVENT_MENUPOPUP_START;
const EVENT_MENUPOPUP_END = nsIAccessibleEvent.EVENT_MENUPOPUP_END;
const EVENT_OBJECT_ATTRIBUTE_CHANGED = nsIAccessibleEvent.EVENT_OBJECT_ATTRIBUTE_CHANGED;
const EVENT_REORDER = nsIAccessibleEvent.EVENT_REORDER;
const EVENT_SCROLLING_START = nsIAccessibleEvent.EVENT_SCROLLING_START;
const EVENT_SELECTION = nsIAccessibleEvent.EVENT_SELECTION;
const EVENT_SELECTION_ADD = nsIAccessibleEvent.EVENT_SELECTION_ADD;
const EVENT_SELECTION_REMOVE = nsIAccessibleEvent.EVENT_SELECTION_REMOVE;
const EVENT_SELECTION_WITHIN = nsIAccessibleEvent.EVENT_SELECTION_WITHIN;
const EVENT_SHOW = nsIAccessibleEvent.EVENT_SHOW;
const EVENT_STATE_CHANGE = nsIAccessibleEvent.EVENT_STATE_CHANGE;
const EVENT_TEXT_ATTRIBUTE_CHANGED = nsIAccessibleEvent.EVENT_TEXT_ATTRIBUTE_CHANGED;
const EVENT_TEXT_CARET_MOVED = nsIAccessibleEvent.EVENT_TEXT_CARET_MOVED;
const EVENT_TEXT_INSERTED = nsIAccessibleEvent.EVENT_TEXT_INSERTED;
const EVENT_TEXT_REMOVED = nsIAccessibleEvent.EVENT_TEXT_REMOVED;
const EVENT_TEXT_SELECTION_CHANGED = nsIAccessibleEvent.EVENT_TEXT_SELECTION_CHANGED;
const EVENT_VALUE_CHANGE = nsIAccessibleEvent.EVENT_VALUE_CHANGE;
const EVENT_VIRTUALCURSOR_CHANGED = nsIAccessibleEvent.EVENT_VIRTUALCURSOR_CHANGED;

const kNotFromUserInput = 0;
const kFromUserInput = 1;




Components.utils.import("resource://gre/modules/Services.jsm");




var gA11yEventDumpID = "";




var gA11yEventDumpToConsole = false;




var gA11yEventDumpToAppConsole = false;




var gA11yEventDumpFeature = "";












function waitForEvent(aEventType, aTargetOrFunc, aFunc, aContext, aArg1, aArg2)
{
  var handler = {
    handleEvent: function handleEvent(aEvent) {

      var target = aTargetOrFunc;
      if (typeof aTargetOrFunc == "function")
        target = aTargetOrFunc.call();

      if (target) {
        if (target instanceof nsIAccessible &&
            target != aEvent.accessible)
          return;

        if (target instanceof nsIDOMNode &&
            target != aEvent.DOMNode)
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





function waveOverImageMap(aImageMapID)
{
  var imageMapNode = getNode(aImageMapID);
  synthesizeMouse(imageMapNode, 10, 10, { type: "mousemove" },
                  imageMapNode.ownerDocument.defaultView);
}




function waitForImageMap(aImageMapID, aTestFunc)
{
  waveOverImageMap(aImageMapID);

  var imageMapAcc = getAccessible(aImageMapID);
  if (imageMapAcc.firstChild)
    return aTestFunc();

  waitForEvent(EVENT_REORDER, imageMapAcc, aTestFunc);
}











function registerA11yEventListener(aEventType, aEventHandler)
{
  listenA11yEvents(true);
  addA11yEventListener(aEventType, aEventHandler);
}






function unregisterA11yEventListener(aEventType, aEventHandler)
{
  removeA11yEventListener(aEventType, aEventHandler);
  listenA11yEvents(false);
}









const INVOKER_ACTION_FAILED = 1;





const DO_NOT_FINISH_TEST = 1;



















































































function eventQueue(aEventType)
{
  

  


  this.push = function eventQueue_push(aEventInvoker)
  {
    this.mInvokers.push(aEventInvoker);
  }

  


  this.invoke = function eventQueue_invoke()
  {
    listenA11yEvents(true);

    
    
    this.processNextInvokerInTimeout(true);
  }

  



  this.onFinish = function eventQueue_finish()
  {
  }

  

  


  this.processNextInvoker = function eventQueue_processNextInvoker()
  {
    
    if (this.mNextInvokerStatus == kInvokerCanceled) {
      this.mNextInvokerStatus = kInvokerNotScheduled;
      return;
    }

    this.mNextInvokerStatus = kInvokerNotScheduled;

    
    var testFailed = false;

    var invoker = this.getInvoker();
    if (invoker) {
      if ("finalCheck" in invoker)
        invoker.finalCheck();

      if (this.mScenarios && this.mScenarios.length) {
        var matchIdx = -1;
        for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
          var eventSeq = this.mScenarios[scnIdx];
          if (!this.areExpectedEventsLeft(eventSeq)) {
            for (var idx = 0; idx < eventSeq.length; idx++) {
              var checker = eventSeq[idx];
              if (checker.unexpected && checker.wasCaught ||
                  !checker.unexpected && checker.wasCaught != 1) {
                break;
              }
            }

            
            
            
            
            if (idx == eventSeq.length) {
              if (matchIdx != -1 && eventSeq.length > 0 &&
                  this.mScenarios[matchIdx].length > 0) {
                ok(false,
                   "We have a matched scenario at index " + matchIdx + " already.");
              }

              if (matchIdx == -1 || eventSeq.length > 0)
                matchIdx = scnIdx;

              
              for (var idx = 0; idx < eventSeq.length; idx++) {
                var checker = eventSeq[idx];

                var typeStr = eventQueue.getEventTypeAsString(checker);
                var msg = "Test with ID = '" + this.getEventID(checker) +
                  "' succeed. ";

                if (checker.unexpected)
                  ok(true, msg + "There's no unexpected " + typeStr + " event.");
                else
                  ok(true, msg + "Event " + typeStr + " was handled.");
              }
            }
          }
        }

        
        
        if (matchIdx == -1) {
          testFailed = true;
          for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
            var eventSeq = this.mScenarios[scnIdx];
            for (var idx = 0; idx < eventSeq.length; idx++) {
              var checker = eventSeq[idx];

              var typeStr = eventQueue.getEventTypeAsString(checker);
              var msg = "Scenario #" + scnIdx + " of test with ID = '" +
                this.getEventID(checker) + "' failed. ";

              if (checker.wasCaught > 1)
                ok(false, msg + "Dupe " + typeStr + " event.");

              if (checker.unexpected) {
                if (checker.wasCaught)
                  ok(false, msg + "There's unexpected " + typeStr + " event.");
              } else if (!checker.wasCaught) {
                ok(false, msg + typeStr + " event was missed.");
              }
            }
          }
        }
      }
    }

    this.clearEventHandler();

    
    if (testFailed || this.mIndex == this.mInvokers.length - 1) {
      listenA11yEvents(false);

      var res = this.onFinish();
      if (res != DO_NOT_FINISH_TEST)
        SimpleTest.finish();

      return;
    }

    
    invoker = this.getNextInvoker();

    
    if (!this.setEventHandler(invoker)) {
      this.processNextInvoker();
      return;
    }

    if (gLogger.isEnabled()) {
      gLogger.logToConsole("Event queue: \n  invoke: " + invoker.getID());
      gLogger.logToDOM("EQ: invoke: " + invoker.getID(), true);
    }

    var infoText = "Invoke the '" + invoker.getID() + "' test { ";
    var scnCount = this.mScenarios ? this.mScenarios.length : 0;
    for (var scnIdx = 0; scnIdx < scnCount; scnIdx++) {
      infoText += "scenario #" + scnIdx + ": ";
      var eventSeq = this.mScenarios[scnIdx];
      for (var idx = 0; idx < eventSeq.length; idx++) {
        infoText += eventSeq[idx].unexpected ? "un" : "" +
          "expected '" + eventQueue.getEventTypeAsString(eventSeq[idx]) +
          "' event; ";
      }
    }
    infoText += " }";
    info(infoText);

    if (invoker.invoke() == INVOKER_ACTION_FAILED) {
      
      this.processNextInvoker();
      return;
    }

    if (this.hasUnexpectedEventsScenario())
      this.processNextInvokerInTimeout(true);
  }

  this.processNextInvokerInTimeout =
    function eventQueue_processNextInvokerInTimeout(aUncondProcess)
  {
    this.mNextInvokerStatus = kInvokerPending;

    
    
    if (!aUncondProcess && this.areAllEventsExpected()) {
      
      var queue = this;
      SimpleTest.executeSoon(function() { queue.processNextInvoker(); });
      return;
    }

    
    window.setTimeout(function(aQueue) { aQueue.processNextInvoker(); }, 300,
                      this);
  }

  


  this.handleEvent = function eventQueue_handleEvent(aEvent)
  {
    var invoker = this.getInvoker();
    if (!invoker) 
      return;

    if (!this.mScenarios) {
      
      
      this.processNextInvoker();
      return;
    }

    if ("debugCheck" in invoker)
      invoker.debugCheck(aEvent);

    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];
      for (var idx = 0; idx < eventSeq.length; idx++) {
        var checker = eventSeq[idx];

        
        
        if (!checker.unexpected && (checker.wasCaught > 0) &&
            eventQueue.isSameEvent(checker, aEvent)) {
          checker.wasCaught++;
          continue;
        }

        
        
        if (checker.unexpected && eventQueue.compareEvents(checker, aEvent)) {
          checker.wasCaught++;
          continue;
        }

        
        
        if (!checker.unexpected && checker.unique &&
            eventQueue.compareEventTypes(checker, aEvent)) {
          var isExppected = false;
          for (var jdx = 0; jdx < eventSeq.length; jdx++) {
            isExpected = eventQueue.compareEvents(eventSeq[jdx], aEvent);
            if (isExpected)
              break;
          }

          if (!isExpected) {
            ok(false,
               "Unique type " +
               eventQueue.getEventTypeAsString(checker) + " event was handled.");
          }
        }
      }
    }

    var matchedChecker = null;
    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];

      
      var nextChecker = this.getNextExpectedEvent(eventSeq);
      if (nextChecker) {
        if (eventQueue.compareEvents(nextChecker, aEvent)) {
          matchedChecker = nextChecker;
          matchedChecker.wasCaught++;
          break;
        }
      }

      
      for (idx = 0; idx < eventSeq.length; idx++) {
        if (!eventSeq[idx].unexpected && eventSeq[idx].async) {
          if (eventQueue.compareEvents(eventSeq[idx], aEvent)) {
            matchedChecker = eventSeq[idx];
            matchedChecker.wasCaught++;
            break;
          }
        }
      }
    }

    
    if (matchedChecker) {
      if ("check" in matchedChecker)
        matchedChecker.check(aEvent);

      var invoker = this.getInvoker();
      if ("check" in invoker)
        invoker.check(aEvent);
    }

    
    eventQueue.logEvent(aEvent, matchedChecker, this.areExpectedEventsLeft(),
                        this.mNextInvokerStatus);

    
    if (!this.areExpectedEventsLeft() &&
        (this.mNextInvokerStatus == kInvokerNotScheduled)) {
      this.processNextInvokerInTimeout();
      return;
    }

    
    if ((this.mNextInvokerStatus == kInvokerPending) && matchedChecker)
      this.mNextInvokerStatus = kInvokerCanceled;
  }

  
  this.getNextExpectedEvent =
    function eventQueue_getNextExpectedEvent(aEventSeq)
  {
    if (!("idx" in aEventSeq))
      aEventSeq.idx = 0;

    while (aEventSeq.idx < aEventSeq.length &&
           (aEventSeq[aEventSeq.idx].unexpected ||
            aEventSeq[aEventSeq.idx].async ||
            aEventSeq[aEventSeq.idx].wasCaught > 0)) {
      aEventSeq.idx++;
    }

    return aEventSeq.idx != aEventSeq.length ? aEventSeq[aEventSeq.idx] : null;
  }

  this.areExpectedEventsLeft =
    function eventQueue_areExpectedEventsLeft(aScenario)
  {
    function scenarioHasUnhandledExpectedEvent(aEventSeq)
    {
      
      
      for (var idx = 0; idx < aEventSeq.length; idx++) {
        if (!aEventSeq[idx].unexpected && !aEventSeq[idx].wasCaught)
          return true;
      }

      return false;
    }

    if (aScenario)
      return scenarioHasUnhandledExpectedEvent(aScenario);

    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];
      if (scenarioHasUnhandledExpectedEvent(eventSeq))
        return true;
    }
    return false;
  }

  this.areAllEventsExpected =
    function eventQueue_areAllEventsExpected()
  {
    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];
      for (var idx = 0; idx < eventSeq.length; idx++) {
        if (eventSeq[idx].unexpected)
          return false;
      }
    }

    return true;
  }

  this.hasUnexpectedEventsScenario =
    function eventQueue_hasUnexpectedEventsScenario()
  {
    if (this.getInvoker().noEventsOnAction)
      return true;

    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];
      for (var idx = 0; idx < eventSeq.length; idx++) {
        if (!eventSeq[idx].unexpected)
          break;
      }
      if (idx == eventSeq.length)
        return true;
    }

    return false;
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
    if (!("scenarios" in aInvoker) || aInvoker.scenarios.length == 0) {
      var eventSeq = aInvoker.eventSeq;
      var unexpectedEventSeq = aInvoker.unexpectedEventSeq;
      if (!eventSeq && !unexpectedEventSeq && this.mDefEventType)
        eventSeq = [ new invokerChecker(this.mDefEventType, aInvoker.DOMNode) ];

      if (eventSeq || unexpectedEventSeq)
        defineScenario(aInvoker, eventSeq, unexpectedEventSeq);
    }

    if (aInvoker.noEventsOnAction)
      return true;

    this.mScenarios = aInvoker.scenarios;
    if (!this.mScenarios || !this.mScenarios.length) {
      ok(false, "Broken invoker '" + aInvoker.getID() + "'");
      return false;
    }

    
    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];

      if (gLogger.isEnabled()) {
        var msg = "scenario #" + scnIdx +
          ", registered events number: " + eventSeq.length;
        gLogger.logToConsole(msg);
        gLogger.logToDOM(msg, true);
      }

      
      
      if (this.mScenarios.length == 1 && eventSeq.length == 0) {
        ok(false,
           "Broken scenario #" + scnIdx + " of invoker '" + aInvoker.getID() +
           "'. No registered events");
        return false;
      }

      for (var idx = 0; idx < eventSeq.length; idx++)
        eventSeq[idx].wasCaught = 0;

      for (var idx = 0; idx < eventSeq.length; idx++) {
        if (gLogger.isEnabled()) {
          var msg = "registered";
          if (eventSeq[idx].unexpected)
            msg += " unexpected";
          if (eventSeq[idx].async)
            msg += " async";

          msg += ": event type: " +
            eventQueue.getEventTypeAsString(eventSeq[idx]) +
            ", target: " + eventQueue.getEventTargetDescr(eventSeq[idx], true);

          gLogger.logToConsole(msg);
          gLogger.logToDOM(msg, true);
        }

        var eventType = eventSeq[idx].type;
        if (typeof eventType == "string") {
          
          var target = eventSeq[idx].target;
          if (!target) {
            ok(false, "no target for DOM event!");
            return false;
          }
          var phase = eventQueue.getEventPhase(eventSeq[idx]);
          target.ownerDocument.addEventListener(eventType, this, phase);

        } else {
          
          addA11yEventListener(eventType, this);
        }
      }
    }

    return true;
  }

  this.clearEventHandler = function eventQueue_clearEventHandler()
  {
    if (!this.mScenarios)
      return;

    for (var scnIdx = 0; scnIdx < this.mScenarios.length; scnIdx++) {
      var eventSeq = this.mScenarios[scnIdx];
      for (var idx = 0; idx < eventSeq.length; idx++) {
        var eventType = eventSeq[idx].type;
        if (typeof eventType == "string") {
          
          var target = eventSeq[idx].target;
          var phase = eventQueue.getEventPhase(eventSeq[idx]);
          target.ownerDocument.removeEventListener(eventType, this, phase);

        } else {
          
          removeA11yEventListener(eventType, this);
        }
      }
    }
    this.mScenarios = null;
  }

  this.getEventID = function eventQueue_getEventID(aChecker)
  {
    if ("getID" in aChecker)
      return aChecker.getID();

    var invoker = this.getInvoker();
    return invoker.getID();
  }

  this.mDefEventType = aEventType;

  this.mInvokers = new Array();
  this.mIndex = -1;
  this.mScenarios = null;

  this.mNextInvokerStatus = kInvokerNotScheduled;
}




const kInvokerNotScheduled = 0;
const kInvokerPending = 1;
const kInvokerCanceled = 2;

eventQueue.getEventTypeAsString =
  function eventQueue_getEventTypeAsString(aEventOrChecker)
{
  if (aEventOrChecker instanceof nsIDOMEvent)
    return aEventOrChecker.type;

  if (aEventOrChecker instanceof nsIAccessibleEvent)
    return eventTypeToString(aEventOrChecker.eventType);

  return (typeof aEventOrChecker.type == "string") ?
    aEventOrChecker.type : eventTypeToString(aEventOrChecker.type);
}

eventQueue.getEventTargetDescr =
  function eventQueue_getEventTargetDescr(aEventOrChecker, aDontForceTarget)
{
  if (aEventOrChecker instanceof nsIDOMEvent)
    return prettyName(aEventOrChecker.originalTarget);

  if (aEventOrChecker instanceof nsIDOMEvent)
    return prettyName(aEventOrChecker.accessible);

  var descr = aEventOrChecker.targetDescr;
  if (descr)
    return descr;

  if (aDontForceTarget)
    return "no target description";

  var target = ("target" in aEventOrChecker) ? aEventOrChecker.target : null;
  return prettyName(target);
}

eventQueue.getEventPhase = function eventQueue_getEventPhase(aChecker)
{
  return ("phase" in aChecker) ? aChecker.phase : true;
}

eventQueue.compareEventTypes =
  function eventQueue_compareEventTypes(aChecker, aEvent)
{
  var eventType = (aEvent instanceof nsIDOMEvent) ?
    aEvent.type : aEvent.eventType;
  return aChecker.type == eventType;
}

eventQueue.compareEvents = function eventQueue_compareEvents(aChecker, aEvent)
{
  if (!eventQueue.compareEventTypes(aChecker, aEvent))
    return false;

  
  
  if ("match" in aChecker)
    return aChecker.match(aEvent);

  var target1 = aChecker.target;
  if (target1 instanceof nsIAccessible) {
    var target2 = (aEvent instanceof nsIDOMEvent) ?
      getAccessible(aEvent.target) : aEvent.accessible;

    return target1 == target2;
  }

  
  
  var target2 = (aEvent instanceof nsIDOMEvent) ?
    aEvent.originalTarget : aEvent.DOMNode;
  return target1 == target2;
}

eventQueue.isSameEvent = function eventQueue_isSameEvent(aChecker, aEvent)
{
  
  
  
  return this.compareEvents(aChecker, aEvent) &&
    !(aEvent instanceof nsIAccessibleTextChangeEvent) &&
    !(aEvent instanceof nsIAccessibleStateChangeEvent);
}

eventQueue.logEvent = function eventQueue_logEvent(aOrigEvent, aMatchedChecker,
                                                   aAreExpectedEventsLeft,
                                                   aInvokerStatus)
{
  if (!gLogger.isEnabled()) 
    return;

  
  
  if (aOrigEvent instanceof nsIDOMEvent) {
    var info = "Event type: " + eventQueue.getEventTypeAsString(aOrigEvent);
    info += ". Target: " + eventQueue.getEventTargetDescr(aOrigEvent);
    gLogger.logToDOM(info);
  }

  var msg = "unhandled expected events: " + aAreExpectedEventsLeft +
    ", invoker status: ";
  switch (aInvokerStatus) {
    case kInvokerNotScheduled:
      msg += "not scheduled";
      break;
    case kInvokerPending:
      msg += "pending";
      break;
    case kInvokerCanceled:
      msg += "canceled";
      break;
  }

  gLogger.logToConsole(msg);
  gLogger.logToDOM(msg);

  if (!aMatchedChecker)
    return;

  var msg = "EQ: ";
  var emphText = "matched ";

  var currType = eventQueue.getEventTypeAsString(aMatchedChecker);
  var currTargetDescr = eventQueue.getEventTargetDescr(aMatchedChecker);
  var consoleMsg = "*****\nEQ matched: " + currType + "\n*****";
  gLogger.logToConsole(consoleMsg);

  msg += " event, type: " + currType + ", target: " + currTargetDescr;

  gLogger.logToDOM(msg, true, emphText);
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









function defineScenario(aInvoker, aEventSeq, aUnexpectedEventSeq)
{
  if (!("scenarios" in aInvoker))
    aInvoker.scenarios = new Array();

  
  
  if (!aEventSeq)
    aEventSeq = [];

  for (var idx = 0; idx < aEventSeq.length; idx++) {
    aEventSeq[idx].unexpected |= false;
    aEventSeq[idx].async |= false;
  }

  if (aUnexpectedEventSeq) {
    for (var idx = 0; idx < aUnexpectedEventSeq.length; idx++) {
      aUnexpectedEventSeq[idx].unexpected = true;
      aUnexpectedEventSeq[idx].async = false;
    }

    aEventSeq = aEventSeq.concat(aUnexpectedEventSeq);
  }

  aInvoker.scenarios.push(aEventSeq);
}













function synthClick(aNodeOrID, aCheckerOrEventSeq, aArgs)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq);

  this.invoke = function synthClick_invoke()
  {
    var targetNode = this.DOMNode;
    if (targetNode instanceof nsIDOMDocument) {
      targetNode =
        this.DOMNode.body ? this.DOMNode.body : this.DOMNode.documentElement;
    }

    
    if (targetNode instanceof nsIDOMHTMLElement) {
      targetNode.scrollIntoView(true);
    } else if (targetNode instanceof nsIDOMXULElement) {
      var targetAcc = getAccessible(targetNode);
      targetAcc.scrollTo(SCROLL_TYPE_ANYWHERE);
    }

    var x = 1, y = 1;
    if (aArgs && ("where" in aArgs) && aArgs.where == "right") {
      if (targetNode instanceof nsIDOMHTMLElement)
        x = targetNode.offsetWidth - 1;
      else if (targetNode instanceof nsIDOMXULElement)
        x = targetNode.boxObject.width - 1;
    }
    synthesizeMouse(targetNode, x, y, aArgs ? aArgs : {});
  }

  this.finalCheck = function synthClick_finalCheck()
  {
    
    window.top.scrollTo(0, 0);
  }

  this.getID = function synthClick_getID()
  {
    return prettyName(aNodeOrID) + " click";
  }
}




function synthMouseMove(aID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthAction(aID, aCheckerOrEventSeq);

  this.invoke = function synthMouseMove_invoke()
  {
    synthesizeMouse(this.DOMNode, 1, 1, { type: "mousemove" });
    synthesizeMouse(this.DOMNode, 2, 2, { type: "mousemove" });
  }

  this.getID = function synthMouseMove_getID()
  {
    return prettyName(aID) + " mouse move";
  }
}




function synthKey(aNodeOrID, aKey, aArgs, aCheckerOrEventSeq)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq);

  this.invoke = function synthKey_invoke()
  {
    synthesizeKey(this.mKey, this.mArgs, this.mWindow);
  }

  this.getID = function synthKey_getID()
  {
    var key = this.mKey;
    switch (this.mKey) {
      case "VK_TAB":
        key = "tab";
        break;
      case "VK_DOWN":
        key = "down";
        break;
      case "VK_UP":
        key = "up";
        break;
      case "VK_LEFT":
        key = "left";
        break;
      case "VK_RIGHT":
        key = "right";
        break;
      case "VK_HOME":
        key = "home";
        break;
      case "VK_ESCAPE":
        key = "escape";
        break;
      case "VK_RETURN":
        key = "enter";
        break;
    }
    if (aArgs) {
      if (aArgs.shiftKey)
        key += " shift";
      if (aArgs.ctrlKey)
        key += " ctrl";
      if (aArgs.altKey)
        key += " alt";
    }
    return prettyName(aNodeOrID) + " '" + key + " ' key";
  }

  this.mKey = aKey;
  this.mArgs = aArgs ? aArgs : {};
  this.mWindow = aArgs ? aArgs.window : null;
}




function synthTab(aNodeOrID, aCheckerOrEventSeq, aWindow)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_TAB",
                                { shiftKey: false, window: aWindow },
                                aCheckerOrEventSeq);
}




function synthShiftTab(aNodeOrID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_TAB", { shiftKey: true },
                                aCheckerOrEventSeq);
}




function synthEscapeKey(aNodeOrID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_ESCAPE", null,
                                aCheckerOrEventSeq);
}




function synthDownKey(aNodeOrID, aCheckerOrEventSeq, aArgs)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_DOWN", aArgs,
                                aCheckerOrEventSeq);
}




function synthUpKey(aNodeOrID, aCheckerOrEventSeq, aArgs)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_UP", aArgs,
                                aCheckerOrEventSeq);
}




function synthRightKey(aNodeOrID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_RIGHT", null, aCheckerOrEventSeq);
}




function synthHomeKey(aNodeOrID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_HOME", null, aCheckerOrEventSeq);
}




function synthEnterKey(aID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthKey(aID, "VK_RETURN", null, aCheckerOrEventSeq);
}




function synthOpenComboboxKey(aID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthDownKey(aID, aCheckerOrEventSeq, { altKey: true });

  this.getID = function synthOpenComboboxKey_getID()
  {
    return "open combobox (atl + down arrow) " + prettyName(aID);
  }
}




function synthFocus(aNodeOrID, aCheckerOrEventSeq)
{
  var checkerOfEventSeq =
    aCheckerOrEventSeq ? aCheckerOrEventSeq : new focusChecker(aNodeOrID);
  this.__proto__ = new synthAction(aNodeOrID, checkerOfEventSeq);

  this.invoke = function synthFocus_invoke()
  {
    if (this.DOMNode instanceof Components.interfaces.nsIDOMNSEditableElement &&
        this.DOMNode.editor ||
        this.DOMNode instanceof Components.interfaces.nsIDOMXULTextBoxElement) {
      this.DOMNode.selectionStart = this.DOMNode.selectionEnd = this.DOMNode.value.length;
    }
    this.DOMNode.focus();
  }

  this.getID = function synthFocus_getID() 
  { 
    return prettyName(aNodeOrID) + " focus";
  }
}




function synthFocusOnFrame(aNodeOrID, aCheckerOrEventSeq)
{
  var frameDoc = getNode(aNodeOrID).contentDocument;
  var checkerOrEventSeq =
    aCheckerOrEventSeq ? aCheckerOrEventSeq : new focusChecker(frameDoc);
  this.__proto__ = new synthAction(frameDoc, checkerOrEventSeq);

  this.invoke = function synthFocus_invoke()
  {
    this.DOMNode.body.focus();
  }

  this.getID = function synthFocus_getID() 
  { 
    return prettyName(aNodeOrID) + " frame document focus";
  }
}




function changeCurrentItem(aID, aItemID)
{
  this.eventSeq = [ new nofocusChecker() ];

  this.invoke = function changeCurrentItem_invoke()
  {
    var controlNode = getNode(aID);
    var itemNode = getNode(aItemID);

    
    if (controlNode.localName == "input") {
      if (controlNode.checked)
        this.reportError();

      controlNode.checked = true;
      return;
    }

    if (controlNode.localName == "select") {
      if (controlNode.selectedIndex == itemNode.index)
        this.reportError();

      controlNode.selectedIndex = itemNode.index;
      return;
    }

    
    if (controlNode.localName == "tree") {
      if (controlNode.currentIndex == aItemID)
        this.reportError();

      controlNode.currentIndex = aItemID;
      return;
    }

    if (controlNode.localName == "menulist") {
      if (controlNode.selectedItem == itemNode)
        this.reportError();

      controlNode.selectedItem = itemNode;
      return;
    }

    if (controlNode.currentItem == itemNode)
      ok(false, "Error in test: proposed current item is already current" + prettyName(aID));

    controlNode.currentItem = itemNode;
  }

  this.getID = function changeCurrentItem_getID()
  {
    return "current item change for " + prettyName(aID);
  }

  this.reportError = function changeCurrentItem_reportError()
  {
    ok(false,
       "Error in test: proposed current item '" + aItemID + "' is already current");
  }
}




function toggleTopMenu(aID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthKey(aID, "VK_ALT", null,
                                aCheckerOrEventSeq);

  this.getID = function toggleTopMenu_getID()
  {
    return "toggle top menu on " + prettyName(aID);
  }
}




function synthContextMenu(aID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthClick(aID, aCheckerOrEventSeq,
                                  { button: 0, type: "contextmenu" });

  this.getID = function synthContextMenu_getID()
  {
    return "context menu on " + prettyName(aID);
  }
}




function openCombobox(aComboboxID)
{
  this.eventSeq = [
    new stateChangeChecker(STATE_EXPANDED, false, true, aComboboxID)
  ];

  this.invoke = function openCombobox_invoke()
  {
    getNode(aComboboxID).focus();
    synthesizeKey("VK_DOWN", { altKey: true });
  }

  this.getID = function openCombobox_getID()
  {
    return "open combobox " + prettyName(aComboboxID);
  }
}




function closeCombobox(aComboboxID)
{
  this.eventSeq = [
    new stateChangeChecker(STATE_EXPANDED, false, false, aComboboxID)
  ];

  this.invoke = function closeCombobox_invoke()
  {
    synthesizeKey("VK_ESCAPE", { });
  }

  this.getID = function closeCombobox_getID()
  {
    return "close combobox " + prettyName(aComboboxID);
  }
}





function synthSelectAll(aNodeOrID, aCheckerOrEventSeq)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq);

  this.invoke = function synthSelectAll_invoke()
  {
    if (this.DOMNode instanceof Components.interfaces.nsIDOMHTMLInputElement ||
        this.DOMNode instanceof Components.interfaces.nsIDOMXULTextBoxElement) {
      this.DOMNode.select();

    } else {
      window.getSelection().selectAllChildren(this.DOMNode);
    }
  }

  this.getID = function synthSelectAll_getID()
  {
    return aNodeOrID + " selectall";
  }
}




function moveCaretToDOMPoint(aID, aNode, aOffset, aExpectedOffset,
                             aFocusTargetID)
{
  this.target = getAccessible(aID, [nsIAccessibleText]);
  this.focus = aFocusTargetID ? getAccessible(aFocusTargetID) : null;
  this.focusNode = this.focus ? this.focus.DOMNode : null;

  this.invoke = function moveCaretToDOMPoint_invoke()
  {
    if (this.focusNode)
      this.focusNode.focus();

    window.getSelection().getRangeAt(0).setStart(aNode, aOffset);
  }

  this.getID = function moveCaretToDOMPoint_getID()
  {
   return "Set caret on " + prettyName(aID) + " at point: " +
     prettyName(aNode) + " node with offset " + aOffset;
  }

  this.eventSeq = [
    new caretMoveChecker(aExpectedOffset, this.target)
  ];

  if (this.focus)
    this.eventSeq.push(new asyncInvokerChecker(EVENT_FOCUS, this.focus));
}




function setCaretOffset(aID, aOffset, aFocusTargetID)
{
  this.target = getAccessible(aID, [nsIAccessibleText]);
  this.offset = aOffset == -1 ? this.target.characterCount: aOffset;
  this.focus = aFocusTargetID ? getAccessible(aFocusTargetID) : null;

  this.invoke = function setCaretOffset_invoke()
  {
    this.target.caretOffset = this.offset;
  }

  this.getID = function setCaretOffset_getID()
  {
   return "Set caretOffset on " + prettyName(aID) + " at " + this.offset;
  }

  this.eventSeq = [
    new caretMoveChecker(this.offset, this.target)
  ];

  if (this.focus)
    this.eventSeq.push(new asyncInvokerChecker(EVENT_FOCUS, this.focus));
}








function invokerChecker(aEventType, aTargetOrFunc, aTargetFuncArg, aIsAsync)
{
  this.type = aEventType;
  this.async = aIsAsync;

  this.__defineGetter__("target", invokerChecker_targetGetter);
  this.__defineSetter__("target", invokerChecker_targetSetter);

  
  function invokerChecker_targetGetter()
  {
    if (typeof this.mTarget == "function")
      return this.mTarget.call(null, this.mTargetFuncArg);
    if (typeof this.mTarget == "string")
      return getNode(this.mTarget);

    return this.mTarget;
  }

  function invokerChecker_targetSetter(aValue)
  {
    this.mTarget = aValue;
    return this.mTarget;
  }

  this.__defineGetter__("targetDescr", invokerChecker_targetDescrGetter);

  function invokerChecker_targetDescrGetter()
  {
    if (typeof this.mTarget == "function")
      return this.mTarget.name + ", arg: " + this.mTargetFuncArg;

    return prettyName(this.mTarget);
  }

  this.mTarget = aTargetOrFunc;
  this.mTargetFuncArg = aTargetFuncArg;
}




function asyncInvokerChecker(aEventType, aTargetOrFunc, aTargetFuncArg)
{
  this.__proto__ = new invokerChecker(aEventType, aTargetOrFunc,
                                      aTargetFuncArg, true);
}

function focusChecker(aTargetOrFunc, aTargetFuncArg)
{
  this.__proto__ = new invokerChecker(EVENT_FOCUS, aTargetOrFunc,
                                      aTargetFuncArg, false);

  this.unique = true; 

  this.check = function focusChecker_check(aEvent)
  {
    testStates(aEvent.accessible, STATE_FOCUSED);
  }
}

function nofocusChecker(aID)
{
  this.__proto__ = new focusChecker(aID);
  this.unexpected = true;
}





function textChangeChecker(aID, aStart, aEnd, aTextOrFunc, aIsInserted, aFromUser)
{
  this.target = getNode(aID);
  this.type = aIsInserted ? EVENT_TEXT_INSERTED : EVENT_TEXT_REMOVED;
  this.startOffset = aStart;
  this.endOffset = aEnd;
  this.textOrFunc = aTextOrFunc;

  this.check = function textChangeChecker_check(aEvent)
  {
    aEvent.QueryInterface(nsIAccessibleTextChangeEvent);

    var modifiedText = (typeof this.textOrFunc == "function") ?
      this.textOrFunc() : this.textOrFunc;
    var modifiedTextLen =
      (this.endOffset == -1) ? modifiedText.length : aEnd - aStart;

    is(aEvent.start, this.startOffset,
       "Wrong start offset for " + prettyName(aID));
    is(aEvent.length, modifiedTextLen, "Wrong length for " + prettyName(aID));
    var changeInfo = (aIsInserted ? "inserted" : "removed");
    is(aEvent.isInserted, aIsInserted,
       "Text was " + changeInfo + " for " + prettyName(aID));
    is(aEvent.modifiedText, modifiedText,
       "Wrong " + changeInfo + " text for " + prettyName(aID));
    if (typeof aFromUser != "undefined")
      is(aEvent.isFromUserInput, aFromUser,
         "wrong value of isFromUserInput() for " + prettyName(aID));
  }
}




function caretMoveChecker(aCaretOffset, aTargetOrFunc, aTargetFuncArg)
{
  this.__proto__ = new invokerChecker(EVENT_TEXT_CARET_MOVED,
                                      aTargetOrFunc, aTargetFuncArg);

  this.check = function caretMoveChecker_check(aEvent)
  {
    is(aEvent.QueryInterface(nsIAccessibleCaretMoveEvent).caretOffset,
       aCaretOffset,
       "Wrong caret offset for " + prettyName(aEvent.accessible));
  }
}




function stateChangeChecker(aState, aIsExtraState, aIsEnabled,
                            aTargetOrFunc, aTargetFuncArg, aIsAsync,
                            aSkipCurrentStateCheck)
{
  this.__proto__ = new invokerChecker(EVENT_STATE_CHANGE, aTargetOrFunc,
                                      aTargetFuncArg, aIsAsync);

  this.check = function stateChangeChecker_check(aEvent)
  {
    var event = null;
    try {
      var event = aEvent.QueryInterface(nsIAccessibleStateChangeEvent);
    } catch (e) {
      ok(false, "State change event was expected");
    }

    if (!event)
      return;

    is(event.isExtraState, aIsExtraState,
       "Wrong extra state bit of the statechange event.");
    isState(event.state, aState, aIsExtraState,
            "Wrong state of the statechange event.");
    is(event.isEnabled, aIsEnabled,
      "Wrong state of statechange event state");

    if (aSkipCurrentStateCheck) {
      todo(false, "State checking was skipped!");
      return;
    }

    var state = aIsEnabled ? (aIsExtraState ? 0 : aState) : 0;
    var extraState = aIsEnabled ? (aIsExtraState ? aState : 0) : 0;
    var unxpdState = aIsEnabled ? 0 : (aIsExtraState ? 0 : aState);
    var unxpdExtraState = aIsEnabled ? 0 : (aIsExtraState ? aState : 0);
    testStates(event.accessible, state, extraState, unxpdState, unxpdExtraState);
  }

  this.match = function stateChangeChecker_match(aEvent)
  {
    if (aEvent instanceof nsIAccessibleStateChangeEvent) {
      var scEvent = aEvent.QueryInterface(nsIAccessibleStateChangeEvent);
      return aEvent.accessible = this.target && scEvent.state == aState;
    }
    return false;
  }
}

function asyncStateChangeChecker(aState, aIsExtraState, aIsEnabled,
                                 aTargetOrFunc, aTargetFuncArg)
{
  this.__proto__ = new stateChangeChecker(aState, aIsExtraState, aIsEnabled,
                                          aTargetOrFunc, aTargetFuncArg, true);
}




function expandedStateChecker(aIsEnabled, aTargetOrFunc, aTargetFuncArg)
{
  this.__proto__ = new invokerChecker(EVENT_STATE_CHANGE, aTargetOrFunc,
                                      aTargetFuncArg);

  this.check = function expandedStateChecker_check(aEvent)
  {
    var event = null;
    try {
      var event = aEvent.QueryInterface(nsIAccessibleStateChangeEvent);
    } catch (e) {
      ok(false, "State change event was expected");
    }

    if (!event)
      return;

    is(event.state, STATE_EXPANDED, "Wrong state of the statechange event.");
    is(event.isExtraState, false,
       "Wrong extra state bit of the statechange event.");
    is(event.isEnabled, aIsEnabled,
      "Wrong state of statechange event state");

    testStates(event.accessible,
               (aIsEnabled ? STATE_EXPANDED : STATE_COLLAPSED));
  }
}









var gA11yEventListeners = {};
var gA11yEventApplicantsCount = 0;

var gA11yEventObserver =
{
  observe: function observe(aSubject, aTopic, aData)
  {
    if (aTopic != "accessible-event")
      return;

    var event;
    try {
      event = aSubject.QueryInterface(nsIAccessibleEvent);
    } catch (ex) {
      
      
      Services.obs.removeObserver(this, "accessible-event");
      
      throw "[accessible/events.js, gA11yEventObserver.observe] This is expected if a previous test has been aborted... Initial exception was: [ " + ex + " ]";
    }
    var listenersArray = gA11yEventListeners[event.eventType];

    var eventFromDumpArea = false;
    if (gLogger.isEnabled()) { 
      eventFromDumpArea = true;

      var target = event.DOMNode;
      var dumpElm = gA11yEventDumpID ?
        document.getElementById(gA11yEventDumpID) : null;

      if (dumpElm) {
        var parent = target;
        while (parent && parent != dumpElm)
          parent = parent.parentNode;
      }

      if (!dumpElm || parent != dumpElm) {
        var type = eventTypeToString(event.eventType);
        var info = "Event type: " + type;

        if (event instanceof nsIAccessibleTextChangeEvent) {
          info += ", start: " + event.start + ", length: " + event.length +
            ", " + (event.isInserted ? "inserted" : "removed") +
            " text: " + event.modifiedText;
        }

        info += ". Target: " + prettyName(event.accessible);

        if (listenersArray)
          info += ". Listeners count: " + listenersArray.length;

        if (gLogger.hasFeature("parentchain:" + type)) {
          info += "\nParent chain:\n";
          var acc = event.accessible;
          while (acc) {
            info += "  " + prettyName(acc) + "\n";
            acc = acc.parent;
          }
        }

        eventFromDumpArea = false;
        gLogger.log(info);
      }
    }

    
    if (!listenersArray || eventFromDumpArea)
      return;

    for (var index = 0; index < listenersArray.length; index++)
      listenersArray[index].handleEvent(event);
  }
};

function listenA11yEvents(aStartToListen)
{
  if (aStartToListen) {
    
    if (!(gA11yEventApplicantsCount++))
      Services.obs.addObserver(gA11yEventObserver, "accessible-event", false);
  } else {
    
    
    if (--gA11yEventApplicantsCount <= 0)
      Services.obs.removeObserver(gA11yEventObserver, "accessible-event");
  }
}

function addA11yEventListener(aEventType, aEventHandler)
{
  if (!(aEventType in gA11yEventListeners))
    gA11yEventListeners[aEventType] = new Array();

  var listenersArray = gA11yEventListeners[aEventType];
  var index = listenersArray.indexOf(aEventHandler);
  if (index == -1)
    listenersArray.push(aEventHandler);
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




var gLogger =
{
  


  isEnabled: function debugOutput_isEnabled()
  {
    return gA11yEventDumpID || gA11yEventDumpToConsole ||
      gA11yEventDumpToAppConsole;
  },

  


  log: function logger_log(aMsg)
  {
    this.logToConsole(aMsg);
    this.logToAppConsole(aMsg);
    this.logToDOM(aMsg);
  },

  







  logToDOM: function logger_logToDOM(aMsg, aHasIndent, aPreEmphText)
  {
    if (gA11yEventDumpID == "")
      return;

    var dumpElm = document.getElementById(gA11yEventDumpID);
    if (!dumpElm) {
      ok(false,
         "No dump element '" + gA11yEventDumpID + "' within the document!");
      return;
    }

    var containerTagName = document instanceof nsIDOMHTMLDocument ?
      "div" : "description";

    var container = document.createElement(containerTagName);
    if (aHasIndent)
      container.setAttribute("style", "padding-left: 10px;");

    if (aPreEmphText) {
      var inlineTagName = document instanceof nsIDOMHTMLDocument ?
        "span" : "description";
      var emphElm = document.createElement(inlineTagName);
      emphElm.setAttribute("style", "color: blue;");
      emphElm.textContent = aPreEmphText;

      container.appendChild(emphElm);
    }

    var textNode = document.createTextNode(aMsg);
    container.appendChild(textNode);

    dumpElm.appendChild(container);
  },

  


  logToConsole: function logger_logToConsole(aMsg)
  {
    if (gA11yEventDumpToConsole)
      dump("\n" + aMsg + "\n");
  },

  


  logToAppConsole: function logger_logToAppConsole(aMsg)
  {
    if (gA11yEventDumpToAppConsole)
      Services.console.logStringMessage("events: " + aMsg);
  },

  


  hasFeature: function logger_hasFeature(aFeature)
  {
    var startIdx = gA11yEventDumpFeature.indexOf(aFeature);
    if (startIdx == - 1)
      return false;

    var endIdx = startIdx + aFeature.length;
    return endIdx == gA11yEventDumpFeature.length ||
      gA11yEventDumpFeature[endIdx] == ";";
  }
};








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







function synthAction(aNodeOrID, aCheckerOrEventSeq)
{
  this.DOMNode = getNode(aNodeOrID);

  if (aCheckerOrEventSeq) {
    if (aCheckerOrEventSeq instanceof Array) {
      this.eventSeq = aCheckerOrEventSeq;
    } else {
      this.eventSeq = [ aCheckerOrEventSeq ];
    }
  }

  this.getID = function synthAction_getID()
    { return prettyName(aNodeOrID) + " action"; }
}
