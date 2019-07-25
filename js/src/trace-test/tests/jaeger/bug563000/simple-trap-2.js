setDebug(true);
var x = "notset";
function main() { x = "failure"; }
function success() { x = "success"; }


trap(main, 7, "success()");
main();

assertEq(x, "success");
