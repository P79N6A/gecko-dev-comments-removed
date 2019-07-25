var x = "failure";
function main() { x = "success"; }


trap(main, 8, "");
main();

assertEq(x, "success");
