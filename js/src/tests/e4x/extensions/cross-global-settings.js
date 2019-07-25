




var gTestfile = 'cross-global-settings.js';

var BUGNUMBER = 631135;
var summary =
  "Look at the callee's global.XML.* setting values, not the caller's";

print(BUGNUMBER + ": " + summary);





var otherGlobal = newGlobal("same-compartment");
otherGlobal.XML.prettyPrinting = true;
otherGlobal.XML.prettyIndent = 2;
otherGlobal.indent = "  ";

XML.prettyPrinting = true;
XML.prettyIndent = 16;

var indent = "    " +
             "    " +
             "    " +
             "    ";

var str = otherGlobal.XML("<a><b/><c/></a>").toXMLString();

assertEq(str.indexOf(indent), -1);
assertEq(str.indexOf(otherGlobal.indent) > 0, true);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
