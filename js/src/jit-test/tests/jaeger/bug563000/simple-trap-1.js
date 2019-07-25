setDebug(true);
var x = "failure";
function main() { x = "success"; }


trap(main, 11, "");
main();

assertEq(x, "success");
