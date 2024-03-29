


const RELATION_CONTROLLED_BY = nsIAccessibleRelation.RELATION_CONTROLLED_BY;
const RELATION_CONTROLLER_FOR = nsIAccessibleRelation.RELATION_CONTROLLER_FOR;
const RELATION_DEFAULT_BUTTON = nsIAccessibleRelation.RELATION_DEFAULT_BUTTON;
const RELATION_DESCRIBED_BY = nsIAccessibleRelation.RELATION_DESCRIBED_BY;
const RELATION_DESCRIPTION_FOR = nsIAccessibleRelation.RELATION_DESCRIPTION_FOR;
const RELATION_EMBEDDED_BY = nsIAccessibleRelation.RELATION_EMBEDDED_BY;
const RELATION_EMBEDS = nsIAccessibleRelation.RELATION_EMBEDS;
const RELATION_FLOWS_FROM = nsIAccessibleRelation.RELATION_FLOWS_FROM;
const RELATION_FLOWS_TO = nsIAccessibleRelation.RELATION_FLOWS_TO;
const RELATION_LABEL_FOR = nsIAccessibleRelation.RELATION_LABEL_FOR;
const RELATION_LABELLED_BY = nsIAccessibleRelation.RELATION_LABELLED_BY;
const RELATION_MEMBER_OF = nsIAccessibleRelation.RELATION_MEMBER_OF;
const RELATION_NODE_CHILD_OF = nsIAccessibleRelation.RELATION_NODE_CHILD_OF;
const RELATION_NODE_PARENT_OF = nsIAccessibleRelation.RELATION_NODE_PARENT_OF;
const RELATION_PARENT_WINDOW_OF = nsIAccessibleRelation.RELATION_PARENT_WINDOW_OF;
const RELATION_POPUP_FOR = nsIAccessibleRelation.RELATION_POPUP_FOR;
const RELATION_SUBWINDOW_OF = nsIAccessibleRelation.RELATION_SUBWINDOW_OF;
const RELATION_CONTAINING_DOCUMENT = nsIAccessibleRelation.RELATION_CONTAINING_DOCUMENT;
const RELATION_CONTAINING_TAB_PANE = nsIAccessibleRelation.RELATION_CONTAINING_TAB_PANE;
const RELATION_CONTAINING_APPLICATION = nsIAccessibleRelation.RELATION_CONTAINING_APPLICATION;













function testRelation(aIdentifier, aRelType, aRelatedIdentifiers)
{
  var relation = getRelationByType(aIdentifier, aRelType);

  var relDescr = getRelationErrorMsg(aIdentifier, aRelType);
  var relDescrStart = getRelationErrorMsg(aIdentifier, aRelType, true);

  if (!relation || !relation.targetsCount) {
    if (!aRelatedIdentifiers) {
      ok(true, "No" + relDescr);
      return;
    }

    var msg = relDescrStart + "has no expected targets: '" +
      prettyName(aRelatedIdentifiers) + "'";

    ok(false, msg);
    return;

  } else if (!aRelatedIdentifiers) {
    ok(false, "There are unexpected targets of " + relDescr);
    return;
  }

  var relatedIds = (aRelatedIdentifiers instanceof Array) ?
  aRelatedIdentifiers : [aRelatedIdentifiers];

  var targets = [];
   for (var idx = 0; idx < relatedIds.length; idx++)
     targets.push(getAccessible(relatedIds[idx]));

  if (targets.length != relatedIds.length)
    return;

  var actualTargets = relation.getTargets();

  
  for (var idx = 0; idx < targets.length; idx++) {
    var isFound = false;
    var enumerate = actualTargets.enumerate();
    while (enumerate.hasMoreElements()) {
      var relatedAcc = enumerate.getNext().QueryInterface(nsIAccessible);
      if (targets[idx] == relatedAcc) {
        isFound = true;
        break;
      }
    }

    ok(isFound, prettyName(relatedIds[idx]) + " is not a target of" + relDescr);
  }

  
  var enumerate = actualTargets.enumerate();
  while (enumerate.hasMoreElements()) {
    var relatedAcc = enumerate.getNext().QueryInterface(nsIAccessible);
    for (var idx = 0; idx < targets.length && relatedAcc != targets[idx]; idx++);

    if (idx == targets.length)
      ok(false, "There is unexpected target" + prettyName(relatedAcc) + "of" + relDescr);
  }
}











function testAbsentRelation(aIdentifier, aRelType, aUnrelatedIdentifiers)
{
  var relation = getRelationByType(aIdentifier, aRelType);

  var relDescr = getRelationErrorMsg(aIdentifier, aRelType);
  var relDescrStart = getRelationErrorMsg(aIdentifier, aRelType, true);

  if (!aUnrelatedIdentifiers) {
    ok(false, "No identifiers given for unrelated accessibles.");
    return;
  }

  if (!relation || !relation.targetsCount) {
    ok(true, "No relations exist.");
    return;
  }

  var relatedIds = (aUnrelatedIdentifiers instanceof Array) ?
    aUnrelatedIdentifiers : [aUnrelatedIdentifiers];

  var targets = [];
  for (var idx = 0; idx < relatedIds.length; idx++)
    targets.push(getAccessible(relatedIds[idx]));

  if (targets.length != relatedIds.length)
    return;

  var actualTargets = relation.getTargets();

  
  for (var idx = 0; idx < targets.length; idx++) {
    var notFound = true;
    var enumerate = actualTargets.enumerate();
    while (enumerate.hasMoreElements()) {
      var relatedAcc = enumerate.getNext().QueryInterface(nsIAccessible);
      if (targets[idx] == relatedAcc) {
        notFound = false;
        break;
      }
    }

    ok(notFound, prettyName(relatedIds[idx]) + " is a target of " + relDescr);
  }
}








function getRelationByType(aIdentifier, aRelType)
{
  var acc = getAccessible(aIdentifier);
  if (!acc)
    return;

  var relation = null;
  try {
    relation = acc.getRelationByType(aRelType);
  } catch (e) {
    ok(false, "Can't get" + getRelationErrorMsg(aIdentifier, aRelType));
  }

  return relation;
}




function getRelationErrorMsg(aIdentifier, aRelType, aIsStartSentence)
{
  var relStr = relationTypeToString(aRelType);
  var msg = aIsStartSentence ? "Relation of '" : " relation of '";
  msg += relStr + "' type for '" + prettyName(aIdentifier) + "'";
  msg += aIsStartSentence ? " " : ".";

  return msg;
}
