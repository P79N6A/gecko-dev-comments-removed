



var PART = 2, PARTS = 4;


var BUGNUMBER = 430133;
var summary =
  'ES5 Object.defineProperty(O, P, Attributes): redefinition ' +
  PART + ' of ' + PARTS;

print(BUGNUMBER + ": " + summary);

load("defineProperty-setup.js");





try
{
  new TestRunner().runPropertyPresentTestsFraction(PART, PARTS);
}
catch (e)
{
  throw "Error thrown during testing: " + e +
          " at line " + e.lineNumber + "\n" +
        (e.stack
          ? "Stack: " + e.stack.split("\n").slice(2).join("\n") + "\n"
          : "");
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete!");
