setDebug(true);
function main() {
  return "failure";
}

trap(main, 3, "'success'");
assertEq(main(), "success");
