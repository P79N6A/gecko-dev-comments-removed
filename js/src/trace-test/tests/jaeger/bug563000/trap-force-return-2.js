function main() {
  return 1;
}

trap(main, 2, "0");
assertEq(main(), 0);
