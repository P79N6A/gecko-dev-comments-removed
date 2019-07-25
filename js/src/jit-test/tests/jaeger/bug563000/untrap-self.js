
setDebug(true);
x = "notset";
function main() {
  
  untrap(main, 22);
  x = "success";
}
function failure() { x = "failure"; }


trap(main, 22, "failure()");
main();
assertEq(x, "success");
