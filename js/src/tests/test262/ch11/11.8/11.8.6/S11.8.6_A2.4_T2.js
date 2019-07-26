










var x = function () { throw "x"; };
var y = function () { throw "y"; };
try {
   x() instanceof y();
   $ERROR('#1.1: var x = function () { throw "x"; }; var y = function () { throw "y"; }; x() instanceof y() throw "x". Actual: ' + (x() instanceof y()));
} catch (e) {
   if (e === "y") {
     $ERROR('#1.2: First expression is evaluated first, and then second expression');
   } else {
     if (e !== "x") {
       $ERROR('#1.3: var x = function () { throw "x"; }; var y = function () { throw "y"; }; x() instanceof y() throw "x". Actual: ' + (e));
     }
   }
}

