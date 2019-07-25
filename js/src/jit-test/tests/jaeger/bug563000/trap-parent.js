
setDebug(true);
x = "notset";
function child() {
  
  trap(parent, 27, "success()");
}
function parent() {
  child();
  x = "failure";
}
function success() {
  x = "success";
}

dis(parent);
parent()
assertEq(x, "success");
