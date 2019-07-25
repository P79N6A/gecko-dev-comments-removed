
setDebug(true);
x = "notset";

function doNothing() { }

function myparent(nested) {
  if (nested) {
    
    trap(myparent, 33, "success()");
    doNothing();
  } else {
    doNothing();
  }
}

trap(myparent, 47, "myparent(true)");

function success() {
  x = "success";
}

myparent(false);
assertEq(x, "success");
