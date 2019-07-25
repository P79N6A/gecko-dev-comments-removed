setDebug(true);
x = "notset";
function main() {
  
  untrap(main, 23);
  x = "success";
}
function failure() { x = "failure"; }


trap(main, 23, "failure()");
main();
assertEq(x, "success");
