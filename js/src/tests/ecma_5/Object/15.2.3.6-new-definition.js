




var BUGNUMBER = 430133;
var summary = 'ES5 Object.defineProperty(O, P, Attributes): new definition';

print(BUGNUMBER + ": " + summary);

load("ecma_5/Object/defineProperty-setup.js");





try
{
  new TestRunner().runNotPresentTests();
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
