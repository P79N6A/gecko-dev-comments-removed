var BUGNUMBER = 1145326;
var summary = 'String.prototype.normalize error when normalization form parameter is not an atom';

print(BUGNUMBER + ": " + summary);

function test() {
  assertEq("abc".normalize("NFKC".split("").join("")), "abc");
  assertEq("abc".normalize("NFKCabc".replace("abc", "")), "abc");
  assertEq("abc".normalize("N" + "F" + "K" + "C"), "abc");
}

if ("normalize" in String.prototype) {
  
  test();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
