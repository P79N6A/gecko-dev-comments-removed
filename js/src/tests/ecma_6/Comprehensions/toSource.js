




assertEq(  function () { g = (for (d of [0]) d); g.next(); }.toSource(),
         '(function () { g = (for (d of [0]) d); g.next(); })');
         

assertEq(  function () { return [for (d of [0]) d]; }.toSource(),
         '(function () { return [for (d of [0]) d]; })');

if (typeof reportCompare === "function")
  reportCompare(true, true);
