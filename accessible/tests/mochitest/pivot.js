Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const PREFILTER_INVISIBLE = nsIAccessibleTraversalRule.PREFILTER_INVISIBLE;
const FILTER_MATCH = nsIAccessibleTraversalRule.FILTER_MATCH;
const FILTER_IGNORE = nsIAccessibleTraversalRule.FILTER_IGNORE;
const FILTER_IGNORE_SUBTREE = nsIAccessibleTraversalRule.FILTER_IGNORE_SUBTREE;







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

  preFilter: PREFILTER_INVISIBLE,

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







function virtualCursorChangedChecker(aDocAcc, aIdOrNameOrAcc, aTextOffsets)
{
  this.__proto__ = new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc);

  this.check = function virtualCursorChangedChecker_check(aEvent)
  {
    var position = aDocAcc.virtualCursor.position;
    position.QueryInterface(nsIAccessNode);

    var idMatches = position.DOMNode.id == aIdOrNameOrAcc;
    var nameMatches = position.name == aIdOrNameOrAcc;
    var accMatches = position == aIdOrNameOrAcc;

    SimpleTest.ok(idMatches || nameMatches || accMatches, "id or name matches",
                  "expecting " + aIdOrNameOrAcc + ", got '" +
                  prettyName(position));

    if (aTextOffsets) {
      SimpleTest.is(aDocAcc.virtualCursor.startOffset, aTextOffsets[0],
                    "wrong start offset");
      SimpleTest.is(aDocAcc.virtualCursor.endOffset, aTextOffsets[1],
                    "wrong end offset");
    }
  };
}









function setVirtualCursorRangeInvoker(aDocAcc, aTextAccessible, aTextOffsets)
{
  this.invoke = function virtualCursorChangedInvoker_invoke()
  {
    SimpleTest.info(prettyName(aTextAccessible) + " " + aTextOffsets);
    aDocAcc.virtualCursor.setTextRange(aTextAccessible,
                                       aTextOffsets[0],
                                       aTextOffsets[1]);
  };

  this.getID = function setVirtualCursorRangeInvoker_getID()
  {
    return "Set offset in " + prettyName(aTextAccessible) +
      " to (" + aTextOffsets[0] + ", " + aTextOffsets[1] + ")";
  }

  this.eventSeq = [
    new virtualCursorChangedChecker(aDocAcc, aTextAccessible, aTextOffsets)
  ];
}










function setVirtualCursorPosInvoker(aDocAcc, aPivotMoveMethod, aRule,
                                    aIdOrNameOrAcc)
{
  this.invoke = function virtualCursorChangedInvoker_invoke()
  {
    var moved = aDocAcc.virtualCursor[aPivotMoveMethod](aRule);
    SimpleTest.ok((aIdOrNameOrAcc && moved) || (!aIdOrNameOrAcc && !moved),
                  "moved pivot");
  };

  this.getID = function setVirtualCursorPosInvoker_getID()
  {
    return "Do " + (aIdOrNameOrAcc ? "" : "no-op ") + aPivotMoveMethod;
  }

  if (aIdOrNameOrAcc) {
    this.eventSeq = [ new virtualCursorChangedChecker(aDocAcc, aIdOrNameOrAcc) ];
  } else {
    this.eventSeq = [];
    this.unexpectedEventSeq = [
      new invokerChecker(EVENT_VIRTUALCURSOR_CHANGED, aDocAcc)
    ];
  }
}











function queueTraversalSequence(aQueue, aDocAcc, aRule, aSequence)
{
  aDocAcc.virtualCursor.position = null;

  for (var i = 0; i < aSequence.length; i++) {
    var invoker = new setVirtualCursorPosInvoker(aDocAcc, "moveNext",
                                                 aRule, aSequence[i]);
    aQueue.push(invoker);
  }

  
  aQueue.push(new setVirtualCursorPosInvoker(aDocAcc, "moveNext", aRule, null));

  for (var i = aSequence.length-2; i >= 0; i--) {
    var invoker = new setVirtualCursorPosInvoker(aDocAcc, "movePrevious",
                                                 aRule, aSequence[i])
    aQueue.push(invoker);
  }

  
  aQueue.push(new setVirtualCursorPosInvoker(aDocAcc, "movePrevious", aRule, null));

  aQueue.push(new setVirtualCursorPosInvoker(
    aDocAcc, "moveLast", aRule, aSequence[aSequence.length - 1]));

  
  aQueue.push(new setVirtualCursorPosInvoker(aDocAcc, "moveNext", aRule, null));

  aQueue.push(new setVirtualCursorPosInvoker(
    aDocAcc, "moveFirst", aRule, aSequence[0]));

  
  aQueue.push(new setVirtualCursorPosInvoker(aDocAcc, "movePrevious", aRule, null));
}




function dumpTraversalSequence(aPivot, aRule)
{
  var sequence = []
  if (aPivot.moveFirst(aRule)) {
    do {
      sequence.push("'" + prettyName(aPivot.position) + "'");
    } while (aPivot.moveNext(aRule))
  }
  SimpleTest.info("\n[" + sequence.join(", ") + "]\n");
}