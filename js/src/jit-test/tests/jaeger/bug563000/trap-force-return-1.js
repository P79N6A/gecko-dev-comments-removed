
setDebug(true);
function main() {
  return "failure";
}

trap(main, 5, "'success'");
assertEq(main(), "success");
