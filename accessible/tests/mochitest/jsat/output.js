const Cu = Components.utils;
const PREF_UTTERANCE_ORDER = "accessibility.accessfu.utterance";

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import("resource://gre/modules/accessibility/OutputGenerator.jsm", this);














function testContextOutput(expected, aAccOrElmOrID, aOldAccOrElmOrID, aGenerator) {
  aOldAccOrElmOrID = aOldAccOrElmOrID || "root";
  var accessible = getAccessible(aAccOrElmOrID);
  var oldAccessible = getAccessible(aOldAccOrElmOrID);
  var context = new PivotContext(accessible, oldAccessible);
  var output = aGenerator.genForContext(context).output;

  isDeeply(output, expected,
           "Context output is correct for " + aAccOrElmOrID +
           " (output: " + output.join(", ") + ") ==" +
           " (expected: " + expected.join(", ") + ")");
}









function testObjectOutput(aAccOrElmOrID, aGenerator) {
  var accessible = getAccessible(aAccOrElmOrID);
  var context = new PivotContext(accessible);
  var output = aGenerator.genForObject(accessible, context);
  var outputOrder;
  try {
    outputOrder = SpecialPowers.getIntPref(PREF_UTTERANCE_ORDER);
  } catch (ex) {
    
    outputOrder = 0;
  }
  var expectedNameIndex = outputOrder === 0 ? output.length - 1 : 0;
  var nameIndex = output.indexOf(accessible.name);

  if (nameIndex > -1) {
    ok(output.indexOf(accessible.name) === expectedNameIndex,
      "Object output is correct for " + aAccOrElmOrID);
  }
}










function testOutput(expected, aAccOrElmOrID, aOldAccOrElmOrID, aOutputKind) {
  var generator;
  if (aOutputKind === 1) {
    generator = UtteranceGenerator;
  } else {
    generator = BrailleGenerator;
  }
  testContextOutput(expected, aAccOrElmOrID, aOldAccOrElmOrID, generator);
  
  
  if (aOldAccOrElmOrID) {
    return;
  }
  testObjectOutput(aAccOrElmOrID, generator);
}
