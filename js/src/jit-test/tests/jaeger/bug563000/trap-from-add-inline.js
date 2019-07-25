setDebug(true);
x = "notset";
function main() {
  
  a = { valueOf: function () { trap(main, 38, "success()"); } };
  a + "";
  x = "failure";
}
function success() { x = "success"; }

main();
assertEq(x, "success");
