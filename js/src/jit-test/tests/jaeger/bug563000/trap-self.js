
setDebug(true);
x = "notset";
function main() {
  
  trap(main, 42, "success()");
  x = "failure";
}
function success() { x = "success"; }

main();
assertEq(x, "success");
