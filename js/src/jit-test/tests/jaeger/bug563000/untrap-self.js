
setDebug(true);
x = "notset";
function main() {
  
  untrap(main, 26);
  x = "success";
}
function failure() { x = "failure"; }


trap(main, 26, "failure()");
main();
assertEq(x, "success");
