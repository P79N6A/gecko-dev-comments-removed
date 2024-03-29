


function editableTextTestRun()
{
  this.add = function add(aTest)
  {
    this.seq.push(aTest);
  }

  this.run = function run()
  {
    this.iterate();
  }

  this.index = 0;
  this.seq = new Array();

  this.iterate = function iterate()
  {
    if (this.index < this.seq.length) {
      this.seq[this.index++].startTest(this);
      return;
    }

    this.seq = null;
    SimpleTest.finish();
  }
}




function editableTextTest(aID)
{
  



  this.scheduleTest = function scheduleTest(aFunc)
  {
    
    
    var dataContainer = {
      func: aFunc,
      funcArgs: Array.slice(arguments, 1)
    };
    this.mEventQueue.push(dataContainer);

    if (!this.mEventQueueReady) {
      this.unwrapNextTest();
      this.mEventQueueReady = true;
    }
  }

  


  this.setTextContents = function setTextContents(aValue, aSkipStartOffset)
  {
    var testID = "setTextContents '" + aValue + "' for " + prettyName(aID);

    function setTextContentsInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.setTextContents(aValue);
    }

    aSkipStartOffset = aSkipStartOffset || 0;
    var insertTripple = aValue ?
      [ aSkipStartOffset, aSkipStartOffset + aValue.length, aValue ] : null;
    var oldValue = getValue(aID);
    var removeTripple = oldValue ?
      [ aSkipStartOffset, aSkipStartOffset + oldValue.length, oldValue ] : null;

    this.generateTest(aID, removeTripple, insertTripple, setTextContentsInvoke,
                      getValueChecker(aID, aValue), testID);
  }

  


  this.insertText = function insertText(aStr, aPos, aResStr, aResPos)
  {
    var testID = "insertText '" + aStr + "' at " + aPos + " for " +
      prettyName(aID);

    function insertTextInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.insertText(aStr, aPos);
    }

    var resPos = (aResPos != undefined) ? aResPos : aPos;
    this.generateTest(aID, null, [resPos, resPos + aStr.length, aStr],
                      insertTextInvoke, getValueChecker(aID, aResStr), testID);
  }

  


  this.copyText = function copyText(aStartPos, aEndPos, aClipboardStr)
  {
    var testID = "copyText from " + aStartPos + " to " + aEndPos + " for " +
      prettyName(aID);

    function copyTextInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.copyText(aStartPos, aEndPos);
    }

    this.generateTest(aID, null, null, copyTextInvoke,
                      getClipboardChecker(aID, aClipboardStr), testID);
  }

  


  this.copyNPasteText = function copyNPasteText(aStartPos, aEndPos,
                                                aPos, aResStr)
  {
    var testID = "copyText from " + aStartPos + " to " + aEndPos +
      "and pasteText at " + aPos + " for " + prettyName(aID);

    function copyNPasteTextInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.copyText(aStartPos, aEndPos);
      acc.pasteText(aPos);
    }

    this.generateTest(aID, null, [aStartPos, aEndPos, getTextFromClipboard],
                      copyNPasteInvoke, getValueChecker(aID, aResStr), testID);
  }

  


  this.cutText = function cutText(aStartPos, aEndPos, aResStr,
                                  aResStartPos, aResEndPos)
  {
    var testID = "cutText from " + aStartPos + " to " + aEndPos + " for " +
      prettyName(aID);

    function cutTextInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.cutText(aStartPos, aEndPos);
    }

    var resStartPos = (aResStartPos != undefined) ? aResStartPos : aStartPos;
    var resEndPos = (aResEndPos != undefined) ? aResEndPos : aEndPos;
    this.generateTest(aID, [resStartPos, resEndPos, getTextFromClipboard], null,
                      cutTextInvoke, getValueChecker(aID, aResStr), testID);
  }

  


  this.cutNPasteText = function copyNPasteText(aStartPos, aEndPos,
                                               aPos, aResStr)
  {
    var testID = "cutText from " + aStartPos + " to " + aEndPos +
      " and pasteText at " + aPos + " for " + prettyName(aID);

    function cutNPasteTextInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.cutText(aStartPos, aEndPos);
      acc.pasteText(aPos);
    }

    this.generateTest(aID, [aStartPos, aEndPos, getTextFromClipboard],
                      [aPos, -1, getTextFromClipboard],
                      cutNPasteTextInvoke, getValueChecker(aID, aResStr),
                      testID);
  }

  


  this.pasteText = function pasteText(aPos, aResStr)
  {
    var testID = "pasteText at " + aPos + " for " + prettyName(aID);

    function pasteTextInvoke()
    {
      var acc = getAccessible(aID, nsIAccessibleEditableText);
      acc.pasteText(aPos);
    }

    this.generateTest(aID, null, [aPos, -1, getTextFromClipboard],
                      pasteTextInvoke, getValueChecker(aID, aResStr), testID);
  }

  


  this.deleteText = function deleteText(aStartPos, aEndPos, aResStr)
  {
    var testID = "deleteText from " + aStartPos + " to " + aEndPos +
      " for " + prettyName(aID);

    var oldValue = getValue(aID).substring(aStartPos, aEndPos);
    var removeTripple = oldValue ? [aStartPos, aEndPos, oldValue] : null;

    function deleteTextInvoke()
    {
      var acc = getAccessible(aID, [nsIAccessibleEditableText]);
      acc.deleteText(aStartPos, aEndPos);
    }

    this.generateTest(aID, removeTripple, null, deleteTextInvoke,
                      getValueChecker(aID, aResStr), testID);
  }

  
  

  function getValue(aID)
  {
    var value = "";
    var elm = getNode(aID);
    if (elm instanceof Components.interfaces.nsIDOMNSEditableElement)
      return elm.value;

    if (elm instanceof Components.interfaces.nsIDOMHTMLDocument)
      return elm.body.textContent;

    return elm.textContent;
  }

  


  function getValueChecker(aID, aValue)
  {
    var checker = {
      check: function valueChecker_check()
      {
        is(getValue(aID), aValue, "Wrong value " + aValue);
      }
    };
    return checker;
  }

  function getClipboardChecker(aID, aText)
  {
    var checker = {
      check: function clipboardChecker_check()
      {
        is(getTextFromClipboard(), aText, "Wrong text in clipboard.");
      }
    };
    return checker;
  }

  function getValueNClipboardChecker(aID, aValue, aText)
  {
    var valueChecker = getValueChecker(aID, aValue);
    var clipboardChecker = getClipboardChecker(aID, aText);

    var checker = {
      check: function()
      {
        valueChecker.check();
        clipboardChecker.check();
      }
    };
    return checker;
  }

  


  this.unwrapNextTest = function unwrapNextTest()
  {
    var data = this.mEventQueue.mInvokers[this.mEventQueue.mIndex + 1];
    if (data)
      data.func.apply(this, data.funcArgs);
  }

  


  this.generateTest = function generateTest(aID, aRemoveTriple, aInsertTriple,
                                            aInvokeFunc, aChecker, aInvokerID)
  {
    var et = this;
    var invoker = {
      eventSeq: [],

      invoke: aInvokeFunc,
      finalCheck: function finalCheck()
      {
        aChecker.check();
        et.unwrapNextTest(); 
      },
      getID: function getID() { return aInvokerID; }
    };

    if (aRemoveTriple) {
      var checker = new textChangeChecker(aID, aRemoveTriple[0],
                                          aRemoveTriple[1], aRemoveTriple[2],
                                          false);
      invoker.eventSeq.push(checker);
    }

    if (aInsertTriple) {
      var checker = new textChangeChecker(aID, aInsertTriple[0],
                                          aInsertTriple[1], aInsertTriple[2],
                                          true);
      invoker.eventSeq.push(checker);
    }

    
    if (!aRemoveTriple && !aInsertTriple)
      invoker.noEventsOnAction = true;

    this.mEventQueue.mInvokers[this.mEventQueue.mIndex + 1] = invoker;
  }

  


  this.startTest = function startTest(aTestRun)
  {
    var testRunObj = aTestRun;
    var thisObj = this;
    this.mEventQueue.onFinish = function finishCallback()
    {
      
      testRunObj.iterate();

      
      
      thisObj.mEventQueue.onFinish = null;

      return DO_NOT_FINISH_TEST;
    }

    this.mEventQueue.invoke();
  }

  this.mEventQueue = new eventQueue();
  this.mEventQueueReady = false;
}

