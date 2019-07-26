


function f(foo)
{
  var x;
  eval("__defineGetter__(\"y\", function ()x)");
}
f("");
try {
((function(){ throw y })())
} catch(exc1) {}
