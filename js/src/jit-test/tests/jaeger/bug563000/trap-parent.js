
setDebug(true);
x = "notset";
function child() {
  
  trap(parent, 26, "success()");
}
function parent() {
  child();
  x = "failure";
}
function success() {
  x = "success";
}

parent()
assertEq(x, "success");
