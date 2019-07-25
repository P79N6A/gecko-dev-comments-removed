
setDebug(true);
x = "notset";
function main() {
  
  untrap(main, 27);
  x = "success";
}
function failure() { x = "failure"; }


trap(main, 27, "failure()");
main();
assertEq(x, "success");
