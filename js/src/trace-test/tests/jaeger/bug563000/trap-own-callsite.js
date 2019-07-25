setDebug(true);
x = "notset";
function myparent(nested) {
  if (nested) {
    
    trap(myparent, 40, "failure()");
  } else {
    x = "success";   
    myparent(true);
  }
}
function failure() { x = "failure"; }

myparent(false);
assertEq(x, "success");
