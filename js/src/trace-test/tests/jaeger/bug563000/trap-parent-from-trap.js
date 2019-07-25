setDebug(true);
x = "notset";

function child() {
  x = "failure1";
  
  trap(parent, 10, "success()");
}

function parent() {
  x = "failure2";
}

trap(parent, 0, "child()");

function success() {
  x = "success";
}

parent();
assertEq(x, "success");
