setDebug(true);
x = "notset";

function doNothing() { }

function myparent(nested) {
  if (nested) {
    
    trap(myparent, 25, "success()");
    doNothing();
  } else {
    doNothing();
  }
}

trap(myparent, 36, "myparent(true)");

function success() {
  x = "success";
}

myparent(false);
assertEq(x, "success");
