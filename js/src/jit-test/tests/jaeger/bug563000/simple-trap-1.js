
setDebug(true);
var x = "failure";
function main() { x = "success"; }


trap(main, 10, "");
main();

assertEq(x, "success");
