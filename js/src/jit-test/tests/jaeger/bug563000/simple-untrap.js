
setDebug(true);
var x = "notset";
function main() { x = "success"; }
function failure() { x = "failure"; }


trap(main, 10, "failure()");
untrap(main, 10);
main();

assertEq(x, "success");
