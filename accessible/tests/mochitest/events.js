


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
const EVENT_SELECTION_ADD = nsIAccessibleEvent.EVENT_SELECTION_ADD;
const EVENT_SELECTION_WITHIN = nsIAccessibleEvent.EVENT_SELECTION_WITHIN;
const EVENT_SHOW = nsIAccessibleEvent.EVENT_SHOW;
const EVENT_STATE_CHANGE = nsIAccessibleEvent.EVENT_STATE_CHANGE;
const EVENT_TEXT_ATTRIBUTE_CHANGED = nsIAccessibleEvent.EVENT_TEXT_ATTRIBUTE_CHANGED;
const EVENT_TEXT_CARET_MOVED = nsIAccessibleEvent.EVENT_TEXT_CARET_MOVED;
const EVENT_TEXT_INSERTED = nsIAccessibleEvent.EVENT_TEXT_INSERTED;
const EVENT_TEXT_REMOVED = nsIAccessibleEvent.EVENT_TEXT_REMOVED;
const EVENT_VALUE_CHANGE = nsIAccessibleEvent.EVENT_VALUE_CHANGE;







var gA11yEventDumpID = "";




var gA11yEventDumpToConsole = false;




var gA11yEventDumpToAppConsole = false;












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

          var typeStr = this.getEventTypeAsString(idx);

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
      listenA11yEvents(false);

      var res = this.onFinish();
      if (res != DO_NOT_FINISH_TEST)
        SimpleTest.finish();

      return;
    }

    
    invoker = this.getNextInvoker();

    if (gLogger.isEnabled()) {
      gLogger.logToConsole("Event queue: \n  invoke: " + invoker.getID());
      gLogger.logToDOM("EQ: invoke: " + invoker.getID(), true);
    }

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

    
    var idx = 0;
    for (; idx < this.mEventSeq.length; idx++) {
      if (!this.isEventUnexpected(idx) && (invoker.wasCaught[idx] == true) &&
          this.isAlreadyCaught(idx, aEvent)) {

        var msg = "Doubled event { event type: " +
          this.getEventTypeAsString(idx) + ", target: " +
          prettyName(this.getEventTarget(idx)) + "} in test with ID = '" +
          this.getEventID(idx) + "'.";
        ok(false, msg);
      }
    }

    
    for (idx = 0; idx < this.mEventSeq.length; idx++) {
      if (this.isEventUnexpected(idx) && this.compareEvents(idx, aEvent))
        invoker.wasCaught[idx] = true;
    }

    
    if (this.mEventSeqIdx == this.mEventSeq.length)
      return;

    
    for (idx = this.mEventSeqIdx + 1;
         idx < this.mEventSeq.length && this.mEventSeq[idx].unexpected;
         idx++);

    
    
    if (idx == this.mEventSeq.length) {
      this.mEventSeqIdx = idx;
      this.processNextInvokerInTimeout();
      return;
    }

    
    var matched = this.compareEvents(idx, aEvent);
    this.dumpEventToDOM(aEvent, idx, matched);

    if (matched) {
      this.checkEvent(idx, aEvent);
      invoker.wasCaught[idx] = true;
      this.mEventSeqIdx = idx;

      
      while (++idx < this.mEventSeq.length && this.mEventSeq[idx].unexpected);

      
      
      
      if (idx == this.mEventSeq.length) {
        this.mEventSeqIdx = idx;
        this.processNextInvokerInTimeout();
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

        if (gLogger.isEnabled()) {
          var msg = "registered";
          if (this.isEventUnexpected(idx))
            msg += " unexpected";

          msg += ": event type: " + this.getEventTypeAsString(idx) +
            ", target: " + this.getEventTargetDescr(idx);

          gLogger.logToConsole(msg);
          gLogger.logToDOM(msg, true);
        }

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

  this.getEventTypeAsString = function eventQueue_getEventTypeAsString(aIdx)
  {
    var type = this.mEventSeq[aIdx].type;
    return (typeof type == "string") ? type : eventTypeToString(type);
  }

  this.getEventTarget = function eventQueue_getEventTarget(aIdx)
  {
    return this.mEventSeq[aIdx].target;
  }

  this.getEventTargetDescr = function eventQueue_getEventTargetDescr(aIdx)
  {
    var descr = this.mEventSeq[aIdx].targetDescr;
    return descr ? descr : "no target description";
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

  this.isEventUnexpected = function eventQueue_isEventUnexpected(aIdx)
  {
    return this.mEventSeq[aIdx].unexpected;
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

  this.isAlreadyCaught = function eventQueue_isAlreadyCaught(aIdx, aEvent)
  {
    
    
    
    return this.compareEvents(aIdx, aEvent) &&
      !(aEvent instanceof nsIAccessibleTextChangeEvent);
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
    if (!gLogger.isEnabled()) 
      return;

    
    
    if (aOrigEvent instanceof nsIDOMEvent) {
      var info = "Event type: " + aOrigEvent.type;
      info += ". Target: " + prettyName(aOrigEvent.originalTarget);
      gLogger.logToDOM(info);
    }

    var currType = this.getEventTypeAsString(aExpectedEventIdx);
    var currTarget = this.getEventTarget(aExpectedEventIdx);

    var msg = "EQ: ";
    var emphText = "";
    if (aMatch) {
      emphText = "matched ";

      var consoleMsg = "*****\nEQ matched: " + currType + "\n*****";
      gLogger.logToConsole(consoleMsg);

    } else {
      msg += "expected";
    }
    msg += " event, type: " + currType + ", target: " + prettyName(currTarget);

    gLogger.logToDOM(msg, true, emphText);
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




















function synthClick(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq, aEventType);

  this.invoke = function synthClick_invoke()
  {
    
    if (this.DOMNode instanceof nsIDOMNSHTMLElement)
      this.DOMNode.scrollIntoView(true);

    synthesizeMouse(this.DOMNode, 1, 1, {});
  }

  this.getID = function synthClick_getID()
  {
    return prettyName(aNodeOrID) + " click"; 
  }
}




function synthMouseMove(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq, aEventType);

  this.invoke = function synthMouseMove_invoke()
  {
    synthesizeMouse(this.DOMNode, 1, 1, { type: "mousemove" });
    synthesizeMouse(this.DOMNode, 2, 2, { type: "mousemove" });
  }

  this.getID = function synthMouseMove_getID()
  {
    return prettyName(aNodeOrID) + " mouse move"; 
  }
}




function synthKey(aNodeOrID, aKey, aArgs, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq, aEventType);

  this.invoke = function synthKey_invoke()
  {
    synthesizeKey(this.mKey, this.mArgs);
  }

  this.getID = function synthKey_getID()
  {
    return prettyName(aNodeOrID) + " '" + this.mKey + "' key"; 
  }

  this.mKey = aKey;
  this.mArgs = aArgs ? aArgs : {};
}




function synthTab(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_TAB", { shiftKey: false },
                                aCheckerOrEventSeq, aEventType);

  this.getID = function synthTab_getID() 
  { 
    return prettyName(aNodeOrID) + " tab";
  }
}




function synthShiftTab(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_TAB", { shiftKey: true },
                                aCheckerOrEventSeq, aEventType);

  this.getID = function synthTabTest_getID() 
  { 
    return prettyName(aNodeOrID) + " shift tab";
  }
}




function synthDownKey(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_DOWN", null, aCheckerOrEventSeq,
                                aEventType);

  this.getID = function synthDownKey_getID()
  {
    return prettyName(aNodeOrID) + " key down";
  }
}




function synthRightKey(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_RIGHT", null, aCheckerOrEventSeq,
                                aEventType);

  this.getID = function synthRightKey_getID()
  {
    return prettyName(aNodeOrID) + " key right";
  }
}




function synthHomeKey(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthKey(aNodeOrID, "VK_HOME", null, aCheckerOrEventSeq,
                                aEventType);
  
  this.getID = function synthHomeKey_getID()
  {
    return prettyName(aNodeOrID) + " key home";
  }
}




function synthFocus(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq, aEventType);

  this.invoke = function synthFocus_invoke()
  {
    if (this.DOMNode instanceof Components.interfaces.nsIDOMNSEditableElement ||
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




function synthFocusOnFrame(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthAction(getNode(aNodeOrID).contentDocument,
                                   aCheckerOrEventSeq, aEventType);
  
  this.invoke = function synthFocus_invoke()
  {
    this.DOMNode.body.focus();
  }
  
  this.getID = function synthFocus_getID() 
  { 
    return prettyName(aNodeOrID) + " frame document focus";
  }
}




function synthSelectAll(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.__proto__ = new synthAction(aNodeOrID, aCheckerOrEventSeq, aEventType);

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








function invokerChecker(aEventType, aTargetOrFunc, aTargetFuncArg)
{
  this.type = aEventType;

  this.__defineGetter__("target", invokerChecker_targetGetter);
  this.__defineSetter__("target", invokerChecker_targetSetter);

  
  function invokerChecker_targetGetter()
  {
    if (typeof this.mTarget == "function")
      return this.mTarget.call(null, this.mTargetFuncArg);

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




function textChangeChecker(aID, aStart, aEnd, aTextOrFunc, aIsInserted)
{
  this.target = getNode(aID);
  this.type = aIsInserted ? EVENT_TEXT_INSERTED : EVENT_TEXT_REMOVED;

  this.check = function textChangeChecker_check(aEvent)
  {
    aEvent.QueryInterface(nsIAccessibleTextChangeEvent);

    var modifiedText = (typeof aTextOrFunc == "function") ?
      aTextOrFunc() : aTextOrFunc;
    var modifiedTextLen = (aEnd == -1) ? modifiedText.length : aEnd - aStart;

    is(aEvent.start, aStart, "Wrong start offset for " + prettyName(aID));
    is(aEvent.length, modifiedTextLen, "Wrong length for " + prettyName(aID));
    var changeInfo = (aIsInserted ? "inserted" : "removed");
    is(aEvent.isInserted(), aIsInserted,
       "Text was " + changeInfo + " for " + prettyName(aID));
    is(aEvent.modifiedText, modifiedText,
       "Wrong " + changeInfo + " text for " + prettyName(aID));
  }
}




function caretMoveChecker(aCaretOffset)
{
  this.check = function caretMoveChecker_check(aEvent)
  {
    is(aEvent.QueryInterface(nsIAccessibleCaretMoveEvent).caretOffset,
       aCaretOffset,
       "Wrong caret offset for " + prettyName(aEvent.accessible));
  }
}









var gA11yEventListeners = {};
var gA11yEventApplicantsCount = 0;

var gA11yEventObserver =
{
  
  
  observerService :
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(nsIObserverService),

  observe: function observe(aSubject, aTopic, aData)
  {
    if (aTopic != "accessible-event")
      return;

    var event;
    try {
      event = aSubject.QueryInterface(nsIAccessibleEvent);
    } catch (ex) {
      
      
      this.observerService.removeObserver(this, "accessible-event");
      
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
            ", " + (event.isInserted() ? "inserted" : "removed") +
            " text: " + event.modifiedText;
        }

        info += ". Target: " + prettyName(event.accessible);

        if (listenersArray)
          info += ". Listeners count: " + listenersArray.length;

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
      gA11yEventObserver.observerService
                        .addObserver(gA11yEventObserver, "accessible-event", false);
  } else {
    
    
    if (--gA11yEventApplicantsCount <= 0)
      gA11yEventObserver.observerService
                        .removeObserver(gA11yEventObserver, "accessible-event");
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
      consoleService.logStringMessage("events: " + aMsg);
  },

  consoleService: Components.classes["@mozilla.org/consoleservice;1"].
    getService(Components.interfaces.nsIConsoleService)
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







function synthAction(aNodeOrID, aCheckerOrEventSeq, aEventType)
{
  this.DOMNode = getNode(aNodeOrID);

  this.checker = null;
  if (aCheckerOrEventSeq) {
    if (aCheckerOrEventSeq instanceof Array) {
      this.eventSeq = aCheckerOrEventSeq;
    } else {
      this.checker = aCheckerOrEventSeq;
      this.checker.target = this.DOMNode;
    }
  }

  if (aEventType)
    this.eventSeq = [ new invokerChecker(aEventType, this.DOMNode) ];

  this.check = function synthAction_check(aEvent)
  {
    if (this.checker)
      this.checker.check(aEvent);
  }

  this.getID = function synthAction_getID() { return aNodeOrID + " action"; }
}
