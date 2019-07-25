setDebug(true);
function main() {
  return 1;
}

trap(main, 1, "0");
assertEq(main(), 0);
