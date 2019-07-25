
setDebug(true);
x = "notset";
function main() {
  
  a = { valueOf: function () { trap(main, 57, "success()"); } };
  b = "";
  eval();
  a + b;
  x = "failure";
}
function success() { x = "success"; }

main();
assertEq(x, "success");
