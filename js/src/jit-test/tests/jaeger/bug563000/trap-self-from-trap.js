
setDebug(true);
x = "notset";

function doNothing() { }

function myparent(nested) {
  if (nested) {
    
    trap(myparent, 30, "success()");
    doNothing();
  } else {
    doNothing();
  }
}

trap(myparent, 43, "myparent(true)");

function success() {
  x = "success";
}

myparent(false);
assertEq(x, "success");
