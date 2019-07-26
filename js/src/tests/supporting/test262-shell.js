










testPassesUnlessItThrows();





function $ERROR(msg)
{
  throw new Error("Test262 error: " + msg);
}





function $INCLUDE(file)
{
  load("supporting/" + file);
}




var fnGlobalObject = (function()
{
  var global = Function("return this")();
  return function fnGlobalObject() { return global; };
})();
