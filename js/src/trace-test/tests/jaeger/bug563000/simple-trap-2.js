setDebug(true);
var x = "notset";
function main() { x = "failure"; }
function success() { x = "success"; }


trap(main, 6, "success()");
main();

assertEq(x, "success");
