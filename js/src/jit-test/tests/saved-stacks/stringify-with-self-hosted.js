


const map = (function () {
  return [3].map(n => saveStack()).pop();
}());

assertEq(map.toString().contains("@self-hosted:"), false);
