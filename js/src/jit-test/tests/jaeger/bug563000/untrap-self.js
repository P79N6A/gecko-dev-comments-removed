
setDebug(true);
x = "notset";
function main() {
  
  untrap(main, 24);
  x = "success";
}
function failure() { x = "failure"; }


trap(main, 24, "failure()");
main();
assertEq(x, "success");
