const Cu = Components.utils;
const PREF_UTTERANCE_ORDER = "accessibility.accessfu.utterance";

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import("resource://gre/modules/accessibility/UtteranceGenerator.jsm",
  this);












function testContextUtterance(expected, aAccOrElmOrID, aOldAccOrElmOrID) {
  aOldAccOrElmOrID = aOldAccOrElmOrID || "root";
  var accessible = getAccessible(aAccOrElmOrID);
  var oldAccessible = getAccessible(aOldAccOrElmOrID);
  var context = new PivotContext(accessible, oldAccessible);
  var utterance = UtteranceGenerator.genForContext(context);
  isDeeply(utterance, expected,
    "Context utterance is correct for " + aAccOrElmOrID);
}







function testObjectUtterance(aAccOrElmOrID) {
  var accessible = getAccessible(aAccOrElmOrID);
  var utterance = UtteranceGenerator.genForObject(accessible);
  var utteranceOrder;
  try {
    utteranceOrder = SpecialPowers.getIntPref(PREF_UTTERANCE_ORDER);
  } catch (ex) {
    
    utteranceOrder = 0;
  }
  var expectedNameIndex = utteranceOrder === 0 ? utterance.length - 1 : 0;
  var nameIndex = utterance.indexOf(accessible.name);

  if (nameIndex > -1) {
    ok(utterance.indexOf(accessible.name) === expectedNameIndex,
      "Object utterance is correct for " + aAccOrElmOrID);
  }
}









function testUtterance(expected, aAccOrElmOrID, aOldAccOrElmOrID) {
  testContextUtterance(expected, aAccOrElmOrID, aOldAccOrElmOrID);
  
  
  if (aOldAccOrElmOrID) {
    return;
  }
  testObjectUtterance(aAccOrElmOrID);
}