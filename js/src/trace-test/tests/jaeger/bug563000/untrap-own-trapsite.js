setDebug(true);
x = "notset";
function child() {
  
  untrap(parent, 11);
  x = "success";
}
function parent() {
  x = "failure";
}

trap(parent, 11, "child()");

parent();
assertEq(x, "success");
