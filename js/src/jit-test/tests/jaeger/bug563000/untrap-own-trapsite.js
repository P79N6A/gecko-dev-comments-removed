
setDebug(true);
x = "notset";
function child() {
  
  untrap(parent, 16);
  x = "success";
}
function parent() {
  x = "failure";
}

trap(parent, 16, "child()");

parent();
assertEq(x, "success");
