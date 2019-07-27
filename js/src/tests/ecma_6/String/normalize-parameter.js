var BUGNUMBER = 918987;
var summary = 'String.prototype.normalize - passing wrong parameter';

print(BUGNUMBER + ": " + summary);

function test() {
  assertThrowsInstanceOf(() => "abc".normalize("NFE"), RangeError,
                         "String.prototype.normalize should raise RangeError on invalid form");

  assertEq("".normalize(), "");
}

if ("normalize" in String.prototype) {
  
  test();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
