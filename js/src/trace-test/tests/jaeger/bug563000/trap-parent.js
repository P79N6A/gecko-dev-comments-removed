setDebug(true);
x = "notset";
function child() {
  
  trap(parent, 17, "success()");
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
