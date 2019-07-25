


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
const EVENT_TEXT_SELECTION_CHANGED = nsIAccessibleEvent.EVENT_TEXT_SELECTION_CHANGED;
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

    
    window.setTimeout(function(aQueue) { aQueue.processNextInvoker(); }, 100,
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
      if (this.isEventExpected(idx) && (invoker.wasCaught[idx] == true) &&
          this.isSameEvent(idx, aEvent)) {

        var msg = "Doubled event { event type: " +
          this.getEventTypeAsString(idx) + ", target: " +
          this.getEventTargetDescr(idx) + "} in test with ID = '" +
          this.getEventID(idx) + "'.";
        ok(false, msg);
      }
    }

    
    
    for (idx = 0; idx < this.mEventSeq.length; idx++) {
      if (this.isEventUnexpected(idx) && this.compareEvents(idx, aEvent))
        invoker.wasCaught[idx] = true;
    }

    
    
    var idxObj = {};
    if (!this.prepareForExpectedEvent(invoker, idxObj))
      return;

    
    var matched = false;
    idx = idxObj.value;
    if (idx < this.mEventSeq.length) {
      matched = this.compareEvents(idx, aEvent);
      if (matched)
        this.mEventSeqIdx = idx;
    }

    
    if (!matched) {
      for (idx = 0; idx < this.mEventSeq.length; idx++) {
        if (this.mEventSeq[idx].async) {
          matched = this.compareEvents(idx, aEvent);
          if (matched)
            break;
        }
      }
    }
    this.dumpEventToDOM(aEvent, idx, matched);

    if (matched) {
      this.checkEvent(idx, aEvent);
      invoker.wasCaught[idx] = true;

      this.prepareForExpectedEvent(invoker);
    }
  }

  
  this.prepareForExpectedEvent =
    function eventQueue_prepareForExpectedEvent(aInvoker, aIdxObj)
  {
    
    if (this.mEventSeqFinished)
      return false;

    
    for (var idx = this.mEventSeqIdx + 1;
         idx < this.mEventSeq.length &&
         (this.mEventSeq[idx].unexpected || this.mEventSeq[idx].async);
         idx++);

    
    
    if (idx == this.mEventSeq.length) {
      var allHandled = true;
      for (var jdx = 0; jdx < this.mEventSeq.length; jdx++) {
        if (this.isEventExpected(jdx) && !aInvoker.wasCaught[jdx])
          allHandled = false;
      }

      if (allHandled) {
        this.mEventSeqIdx = this.mEventSeq.length;
        this.mEventFinished = true;
        this.processNextInvokerInTimeout();
        return false;
      }
    }

    if (aIdxObj)
      aIdxObj.value = idx;

    return true;
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

    var len = this.mEventSeq.length;
    for (var idx = 0; idx < len; idx++) {
      var seqItem = this.mEventSeq[idx];
      
      if (!("unexpected" in this.mEventSeq[idx]))
        seqItem.unexpected = false;

      if (!("async" in this.mEventSeq[idx]))
        seqItem.async = false;

      
      
      
      
      if (("unique" in seqItem) && seqItem.unique) {
        var uniquenessChecker = {
          type: seqItem.type,
          unexpected: true,
          match: function uniquenessChecker_match(aEvent)
          {
            
            
            var matched = true;
            for (var idx = 0; idx < this.queue.mEventSeq.length; idx++) {
              if (this.queue.isEventExpected(idx) &&
                  this.queue.compareEvents(idx, aEvent)) {
                matched = false;
                break;
              }
            }
            return matched;
          },
          targetDescr: "any target different from expected events",
          queue: this
        };
        this.mEventSeq.push(uniquenessChecker);
      }
    }

    var unexpectedSeq = aInvoker.unexpectedEventSeq;
    if (unexpectedSeq) {
      for (var idx = 0; idx < unexpectedSeq.length; idx++) {
        unexpectedSeq[idx].unexpected = true;
        unexpectedSeq[idx].async = false;
      }

      this.mEventSeq = this.mEventSeq.concat(unexpectedSeq);
    }

    this.mEventSeqIdx = -1;
    this.mEventSeqFinished = false;

    
    if (this.mEventSeq) {
      aInvoker.wasCaught = new Array(this.mEventSeq.length);

      for (var idx = 0; idx < this.mEventSeq.length; idx++) {
        var eventType = this.getEventType(idx);

        if (gLogger.isEnabled()) {
          var msg = "registered";
          if (this.isEventUnexpected(idx))
            msg += " unexpected";

          msg += ": event type: " + this.getEventTypeAsString(idx) +
            ", target: " + this.getEventTargetDescr(idx, true);

          gLogger.logToConsole(msg);
          gLogger.logToDOM(msg, true);
        }

        if (typeof eventType == "string") {
          
          var target = this.getEventTarget(idx);
          if (!target) {
            ok(false, "no target for DOM event!");
            return;
          }
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

  this.getEventTargetDescr =
    function eventQueue_getEventTargetDescr(aIdx, aDontForceTarget)
  {
    var descr = this.mEventSeq[aIdx].targetDescr;
    if (descr)
      return descr;

    if (aDontForceTarget)
      return "no target description";

    var target = ("target" in this.mEventSeq[aIdx]) ?
      this.mEventSeq[aIdx].target : null;
    return prettyName(target);
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
  this.isEventExpected = function eventQueue_isEventExpected(aIdx)
  {
    return !this.mEventSeq[aIdx].unexpected;
  }

  this.compareEventTypes = function eventQueue_compareEventTypes(aIdx, aEvent)
  {
    var eventType1 = this.getEventType(aIdx);
    var eventType2 = (aEvent instanceof nsIDOMEvent) ?
      aEvent.type : aEvent.eventType;

    return eventType1 == eventType2;
  }

  this.compareEvents = function eventQueue_compareEvents(aIdx, aEvent)
  {
    if (!this.compareEventTypes(aIdx, aEvent))
      return false;

    
    
    if ("match" in this.mEventSeq[aIdx])
      return this.mEventSeq[aIdx].match(aEvent);

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

  this.isSameEvent = function eventQueue_isSameEvent(aIdx, aEvent)
  {
    
    
    
    return this.compareEvents(aIdx, aEvent) &&
      !(aEvent instanceof nsIAccessibleTextChangeEvent) &&
      !(aEvent instanceof nsIAccessibleStateChangeEvent);
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

    if (!aMatch)
      return;

    var msg = "EQ: ";
    var emphText = "matched ";

    var currType = this.getEventTypeAsString(aExpectedEventIdx);
    var currTargetDescr = this.getEventTargetDescr(aExpectedEventIdx);
    var consoleMsg = "*****\nEQ matched: " + currType + "\n*****";
    gLogger.logToConsole(consoleMsg);

    msg += " event, type: " + currType + ", target: " + currTargetDescr;

    gLogger.logToDOM(msg, true, emphText);
  }

  this.mDefEventType = aEventType;

  this.mInvokers = new Array();
  this.mIndex = -1;

  this.mEventSeq = null;
  this.mEventSeqIdx = -1;
  this.mEventSeqFinished = false;
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
                            aTargetOrFunc, aTargetFuncArg)
{
  this.__proto__ = new invokerChecker(EVENT_STATE_CHANGE, aTargetOrFunc,
                                      aTargetFuncArg);

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

    is(event.state, aState, "Wrong state of the statechange event.");
    is(event.isExtraState(), aIsExtraState,
       "Wrong extra state bit of the statechange event.");
    is(event.isEnabled(), aIsEnabled,
      "Wrong state of statechange event state");

    var state = aIsEnabled ? (aIsExtraState ? 0 : aState) : 0;
    var extraState = aIsEnabled ? (aIsExtraState ? aState : 0) : 0;
    var unxpdState = aIsEnabled ? 0 : (aIsExtraState ? 0 : aState);
    var unxpdExtraState = aIsEnabled ? 0 : (aIsExtraState ? aState : 0);
    testStates(event.accessible, state, extraState, unxpdState, unxpdExtraState);
  }
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
    is(event.isExtraState(), false,
       "Wrong extra state bit of the statechange event.");
    is(event.isEnabled(), aIsEnabled,
      "Wrong state of statechange event state");

    testStates(event.accessible,
               (aIsEnabled ? STATE_EXPANDED : STATE_COLLAPSED));
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
