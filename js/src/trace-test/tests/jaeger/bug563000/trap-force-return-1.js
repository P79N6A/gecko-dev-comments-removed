function main() {
  return "failure";
}

trap(main, 4, "'success'");
assertEq(main(), "success");
