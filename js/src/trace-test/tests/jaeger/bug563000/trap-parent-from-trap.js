x = "notset";

function child() {
  x = "failure1";
  
  trap(parent, 11, "success()");
}

function parent() {
  x = "failure2";
}

trap(parent, 1, "child()");

function success() {
  x = "success";
}

parent();
assertEq(x, "success");
