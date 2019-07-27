Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const PREFILTER_INVISIBLE = nsIAccessibleTraversalRule.PREFILTER_INVISIBLE;
const PREFILTER_ARIA_HIDDEN = nsIAccessibleTraversalRule.PREFILTER_ARIA_HIDDEN;
const PREFILTER_TRANSPARENT = nsIAccessibleTraversalRule.PREFILTER_TRANSPARENT;
const FILTER_MATCH = nsIAccessibleTraversalRule.FILTER_MATCH;
const FILTER_IGNORE = nsIAccessibleTraversalRule.FILTER_IGNORE;
const FILTER_IGNORE_SUBTREE = nsIAccessibleTraversalRule.FILTER_IGNORE_SUBTREE;
const CHAR_BOUNDARY = nsIAccessiblePivot.CHAR_BOUNDARY;
const WORD_BOUNDARY = nsIAccessiblePivot.WORD_BOUNDARY;

const NS_ERROR_NOT_IN_TREE = 0x80780026;
const NS_ERROR_INVALID_ARG = 0x80070057;







var HeadersTraversalRule =
{
  getMatchRoles: function(aRules)
  {
    aRules.value = [ROLE_HEADING];
    return aRules.value.length;
  },

  preFilter: PREFILTER_INVISIBLE,

  match: function(aAccessible)
  {
    return FILTER_MATCH;
  },

  QueryInterface: XPCOMUtils.generateQI([nsIAccessibleTraversalRule])
}




var ObjectTraversalRule =
{
  getMatchRoles: function(aRules)
  {
    aRules.value = [];
    return 0;
  },

  preFilter: PREFILTER_INVISIBLE | PREFILTER_ARIA_HIDDEN | PREFILTER_TRANSPARENT,

  match: function(aAccessible)
  {
    var rv = FILTER_IGNORE;
    var role = aAccessible.role;
    if (hasState(aAccessible, STATE_FOCUSABLE) &&
        (role != ROLE_DOCUMENT && role != ROLE_INTERNAL_FRAME))
      rv = FILTER_IGNORE_SUBTREE | FILTER_MATCH;
    else if (aAccessible.childCount == 0 &&
             role != ROLE_STATICTEXT && aAccessible.name.trim())
      rv = FILTER_MATCH;

    return rv;
  },

  QueryInterface: XPCOMUtils.generateQI([nsIAccessibleTraversalRule])
};







function VCChangedChecker(aDocAcc, aIdOrNameOrAcc, aTextOffsets, aPivotMoveMethod,
                          aIsFromUserInput)
{
  this.__proto__ = new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc);

  this.match = function VCChangedChecker_match(aEvent)
  {
    var event = null;
    try {
      event = aEvent.QueryInterface(nsIAccessibleVirtualCursorChangeEvent);
    } catch (e) {
      return false;
    }

    var expectedReason = VCChangedChecker.methodReasonMap[aPivotMoveMethod] ||
      nsIAccessiblePivot.REASON_NONE;

    return event.reason == expectedReason;
  };

  this.check = function VCChangedChecker_check(aEvent)
  {
    SimpleTest.info("VCChangedChecker_check");

    var event = null;
    try {
      event = aEvent.QueryInterface(nsIAccessibleVirtualCursorChangeEvent);
    } catch (e) {
      SimpleTest.ok(false, "Does not support correct interface: " + e);
    }

    var position = aDocAcc.virtualCursor.position;
    var idMatches = position && position.DOMNode.id == aIdOrNameOrAcc;
    var nameMatches = position && position.name == aIdOrNameOrAcc;
    var accMatches = position == aIdOrNameOrAcc;

    SimpleTest.ok(idMatches || nameMatches || accMatches, "id or name matches",
                  "expecting " + aIdOrNameOrAcc + ", got '" +
                  prettyName(position));

    SimpleTest.is(aEvent.isFromUserInput, aIsFromUserInput,
                  "Expected user input is " + aIsFromUserInput + '\n');

    if (aTextOffsets) {
      SimpleTest.is(aDocAcc.virtualCursor.startOffset, aTextOffsets[0],
                    "wrong start offset");
      SimpleTest.is(aDocAcc.virtualCursor.endOffset, aTextOffsets[1],
                    "wrong end offset");
    }

    var prevPosAndOffset = VCChangedChecker.
      getPreviousPosAndOffset(aDocAcc.virtualCursor);

    if (prevPosAndOffset) {
      SimpleTest.is(event.oldAccessible, prevPosAndOffset.position,
                    "previous position does not match");
      SimpleTest.is(event.oldStartOffset, prevPosAndOffset.startOffset,
                    "previous start offset does not match");
      SimpleTest.is(event.oldEndOffset, prevPosAndOffset.endOffset,
                    "previous end offset does not match");
    }
  };
}

VCChangedChecker.prevPosAndOffset = {};

VCChangedChecker.storePreviousPosAndOffset =
  function storePreviousPosAndOffset(aPivot)
{
  VCChangedChecker.prevPosAndOffset[aPivot] =
    {position: aPivot.position,
     startOffset: aPivot.startOffset,
     endOffset: aPivot.endOffset};
};

VCChangedChecker.getPreviousPosAndOffset =
  function getPreviousPosAndOffset(aPivot)
{
  return VCChangedChecker.prevPosAndOffset[aPivot];
};

VCChangedChecker.methodReasonMap = {
  'moveNext': nsIAccessiblePivot.REASON_NEXT,
  'movePrevious': nsIAccessiblePivot.REASON_PREV,
  'moveFirst': nsIAccessiblePivot.REASON_FIRST,
  'moveLast': nsIAccessiblePivot.REASON_LAST,
  'setTextRange': nsIAccessiblePivot.REASON_TEXT,
  'moveNextByText': nsIAccessiblePivot.REASON_TEXT,
  'movePreviousByText': nsIAccessiblePivot.REASON_TEXT,
  'moveToPoint': nsIAccessiblePivot.REASON_POINT
};









function setVCRangeInvoker(aDocAcc, aTextAccessible, aTextOffsets)
{
  this.invoke = function virtualCursorChangedInvoker_invoke()
  {
    VCChangedChecker.
      storePreviousPosAndOffset(aDocAcc.virtualCursor);
    SimpleTest.info(prettyName(aTextAccessible) + " " + aTextOffsets);
    aDocAcc.virtualCursor.setTextRange(aTextAccessible,
                                       aTextOffsets[0],
                                       aTextOffsets[1]);
  };

  this.getID = function setVCRangeInvoker_getID()
  {
    return "Set offset in " + prettyName(aTextAccessible) +
      " to (" + aTextOffsets[0] + ", " + aTextOffsets[1] + ")";
  };

  this.eventSeq = [
    new VCChangedChecker(aDocAcc, aTextAccessible, aTextOffsets, "setTextRange", true)
  ];
}













function setVCPosInvoker(aDocAcc, aPivotMoveMethod, aRule, aIdOrNameOrAcc,
                         aIsFromUserInput)
{
  var expectMove = (aIdOrNameOrAcc != false);
  this.invoke = function virtualCursorChangedInvoker_invoke()
  {
    VCChangedChecker.
      storePreviousPosAndOffset(aDocAcc.virtualCursor);
    if (aPivotMoveMethod && aRule) {
      var moved = false;
      switch (aPivotMoveMethod) {
        case 'moveFirst':
        case 'moveLast':
          moved = aDocAcc.virtualCursor[aPivotMoveMethod](aRule,
            aIsFromUserInput === undefined ? true : aIsFromUserInput);
          break;
        case 'moveNext':
        case 'movePrevious':
          moved = aDocAcc.virtualCursor[aPivotMoveMethod](aRule,
            aDocAcc.virtualCursor.position, false,
            aIsFromUserInput === undefined ? true : aIsFromUserInput);
          break;
      }
      SimpleTest.is(!!moved, !!expectMove,
                    "moved pivot with " + aPivotMoveMethod +
                    " to " + aIdOrNameOrAcc);
    } else {
      aDocAcc.virtualCursor.position = getAccessible(aIdOrNameOrAcc);
    }
  };

  this.getID = function setVCPosInvoker_getID()
  {
    return "Do " + (expectMove ? "" : "no-op ") + aPivotMoveMethod;
  };

  if (expectMove) {
    this.eventSeq = [
      new VCChangedChecker(aDocAcc, aIdOrNameOrAcc, null, aPivotMoveMethod,
        aIsFromUserInput === undefined ? !!aPivotMoveMethod : aIsFromUserInput)
    ];
  } else {
    this.eventSeq = [];
    this.unexpectedEventSeq = [
      new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc)
    ];
  }
}















function setVCTextInvoker(aDocAcc, aPivotMoveMethod, aBoundary, aTextOffsets,
                          aIdOrNameOrAcc, aIsFromUserInput)
{
  var expectMove = (aIdOrNameOrAcc != false);
  this.invoke = function virtualCursorChangedInvoker_invoke()
  {
    VCChangedChecker.storePreviousPosAndOffset(aDocAcc.virtualCursor);
    SimpleTest.info(aDocAcc.virtualCursor.position);
    var moved = aDocAcc.virtualCursor[aPivotMoveMethod](aBoundary,
      aIsFromUserInput === undefined ? true : false);
    SimpleTest.is(!!moved, !!expectMove,
                  "moved pivot by text with " + aPivotMoveMethod +
                  " to " + aIdOrNameOrAcc);
  };

  this.getID = function setVCPosInvoker_getID()
  {
    return "Do " + (expectMove ? "" : "no-op ") + aPivotMoveMethod + " in " +
      prettyName(aIdOrNameOrAcc) + ", " + boundaryToString(aBoundary) +
      ", [" + aTextOffsets + "]";
  };

  if (expectMove) {
    this.eventSeq = [
      new VCChangedChecker(aDocAcc, aIdOrNameOrAcc, aTextOffsets, aPivotMoveMethod,
        aIsFromUserInput === undefined ? true : aIsFromUserInput)
    ];
  } else {
    this.eventSeq = [];
    this.unexpectedEventSeq = [
      new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc)
    ];
  }
}















function moveVCCoordInvoker(aDocAcc, aX, aY, aIgnoreNoMatch,
                            aRule, aIdOrNameOrAcc)
{
  var expectMove = (aIdOrNameOrAcc != false);
  this.invoke = function virtualCursorChangedInvoker_invoke()
  {
    VCChangedChecker.
      storePreviousPosAndOffset(aDocAcc.virtualCursor);
    var moved = aDocAcc.virtualCursor.moveToPoint(aRule, aX, aY,
                                                  aIgnoreNoMatch);
    SimpleTest.ok((expectMove && moved) || (!expectMove && !moved),
                  "moved pivot");
  };

  this.getID = function setVCPosInvoker_getID()
  {
    return "Do " + (expectMove ? "" : "no-op ") + "moveToPoint " + aIdOrNameOrAcc;
  };

  if (expectMove) {
    this.eventSeq = [
      new VCChangedChecker(aDocAcc, aIdOrNameOrAcc, null, 'moveToPoint', true)
    ];
  } else {
    this.eventSeq = [];
    this.unexpectedEventSeq = [
      new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc)
    ];
  }
}








function setModalRootInvoker(aDocAcc, aModalRootAcc, aExpectedResult)
{
  this.invoke = function setModalRootInvoker_invoke()
  {
    var errorResult = 0;
    try {
      aDocAcc.virtualCursor.modalRoot = aModalRootAcc;
    } catch (x) {
      SimpleTest.ok(
        x.result, "Unexpected exception when changing modal root: " + x);
      errorResult = x.result;
    }

    SimpleTest.is(errorResult, aExpectedResult,
                  "Did not get expected result when changing modalRoot");
  };

  this.getID = function setModalRootInvoker_getID()
  {
    return "Set modalRoot to " + prettyName(aModalRootAcc);
  };

  this.eventSeq = [];
  this.unexpectedEventSeq = [
    new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc)
  ];
}













function queueTraversalSequence(aQueue, aDocAcc, aRule, aModalRoot, aSequence)
{
  aDocAcc.virtualCursor.position = null;

  
  aQueue.push(new setModalRootInvoker(aDocAcc, aModalRoot, 0));

  aQueue.push(new setVCPosInvoker(aDocAcc, "moveFirst", aRule, aSequence[0]));

  for (var i = 1; i < aSequence.length; i++) {
    var invoker =
      new setVCPosInvoker(aDocAcc, "moveNext", aRule, aSequence[i]);
    aQueue.push(invoker);
  }

  
  aQueue.push(new setVCPosInvoker(aDocAcc, "moveNext", aRule, false));

  for (var i = aSequence.length-2; i >= 0; i--) {
    var invoker =
      new setVCPosInvoker(aDocAcc, "movePrevious", aRule, aSequence[i]);
    aQueue.push(invoker);
  }

  
  aQueue.push(new setVCPosInvoker(aDocAcc, "movePrevious", aRule, false));

  aQueue.push(new setVCPosInvoker(aDocAcc, "moveLast", aRule,
                                  aSequence[aSequence.length - 1]));

  
  aQueue.push(new setVCPosInvoker(aDocAcc, "moveNext", aRule, false));

  
  aQueue.push(new setVCPosInvoker(aDocAcc, "moveFirst", aRule, aSequence[0], false));

  
  aQueue.push(new setVCPosInvoker(aDocAcc, "movePrevious", aRule, false));

  
  aQueue.push(new setModalRootInvoker(aDocAcc, null, 0));
}




function removeVCPositionChecker(aDocAcc, aHiddenParentAcc)
{
  this.__proto__ = new invokerChecker(EVENT_REORDER, aHiddenParentAcc);

  this.check = function removeVCPositionChecker_check(aEvent) {
    var errorResult = 0;
    try {
      aDocAcc.virtualCursor.moveNext(ObjectTraversalRule);
    } catch (x) {
      errorResult = x.result;
    }
    SimpleTest.is(
      errorResult, NS_ERROR_NOT_IN_TREE,
      "Expecting NOT_IN_TREE error when moving pivot from invalid position.");
  };
}








function removeVCPositionInvoker(aDocAcc, aPosNode)
{
  this.accessible = getAccessible(aPosNode);
  this.invoke = function removeVCPositionInvoker_invoke()
  {
    aDocAcc.virtualCursor.position = this.accessible;
    aPosNode.parentNode.removeChild(aPosNode);
  };

  this.getID = function removeVCPositionInvoker_getID()
  {
    return "Bring virtual cursor to accessible, and remove its DOM node.";
  };

  this.eventSeq = [
    new removeVCPositionChecker(aDocAcc, this.accessible.parent)
  ];
}





function removeVCRootChecker(aPivot)
{
  this.__proto__ = new invokerChecker(EVENT_REORDER, aPivot.root.parent);

  this.check = function removeVCRootChecker_check(aEvent) {
    var errorResult = 0;
    try {
      aPivot.moveLast(ObjectTraversalRule);
    } catch (x) {
      errorResult = x.result;
    }
    SimpleTest.is(
      errorResult, NS_ERROR_NOT_IN_TREE,
      "Expecting NOT_IN_TREE error when moving pivot from invalid position.");
  };
}








function removeVCRootInvoker(aRootNode)
{
  this.pivot = gAccRetrieval.createAccessiblePivot(getAccessible(aRootNode));
  this.invoke = function removeVCRootInvoker_invoke()
  {
    this.pivot.position = this.pivot.root.firstChild;
    aRootNode.parentNode.removeChild(aRootNode);
  };

  this.getID = function removeVCRootInvoker_getID()
  {
    return "Remove root of pivot from tree.";
  };

  this.eventSeq = [
    new removeVCRootChecker(this.pivot)
  ];
}




function dumpTraversalSequence(aPivot, aRule)
{
  var sequence = [];
  if (aPivot.moveFirst(aRule)) {
    do {
      sequence.push("'" + prettyName(aPivot.position) + "'");
    } while (aPivot.moveNext(aRule))
  }
  SimpleTest.info("\n[" + sequence.join(", ") + "]\n");
}
