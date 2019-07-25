
setDebug(true);
x = "notset";
function main() {
  
  untrap(main, 28);
  x = "success";
}
function failure() { x = "failure"; }


trap(main, 28, "failure()");
main();
assertEq(x, "success");
