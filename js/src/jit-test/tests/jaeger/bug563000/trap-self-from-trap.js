
setDebug(true);
x = "notset";

function doNothing() { }

function myparent(nested) {
  if (nested) {
    
    trap(myparent, 28, "success()");
    doNothing();
  } else {
    doNothing();
  }
}

trap(myparent, 41, "myparent(true)");

function success() {
  x = "success";
}

myparent(false);
assertEq(x, "success");
