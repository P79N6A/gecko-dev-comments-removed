setDebug(true);
x = "notset";
function child() {
  
  untrap(parent, 10);
  x = "success";
}
function parent() {
  x = "failure";
}

trap(parent, 10, "child()");

parent();
assertEq(x, "success");
