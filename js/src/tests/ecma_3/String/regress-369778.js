


var BUGNUMBER = 369778;
var summary =
  "RegExpStatics::makeMatch should make an undefined value when the last " +
  "match had an undefined capture.";

print(BUGNUMBER + ": " + summary);





var expected = undefined;
var actual;

'x'.replace(/x(.)?/g, function(m, group) { actual = group; })

print("expected: " + expected)
print("actual: " + actual)

assertEq(expected, actual)



if (typeof reportCompare === "function")
  reportCompare(true, true);

