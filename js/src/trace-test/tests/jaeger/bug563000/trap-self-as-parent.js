setDebug(true);
x = "notset";

function myparent(nested) {
  if (nested) {
    
    trap(myparent, 49, "success()");
  } else {
    myparent(true);
    x = "failure";
    noop();
  }
}
function noop() { }
function success() { x = "success"; }

myparent();
assertEq(x, "success");
