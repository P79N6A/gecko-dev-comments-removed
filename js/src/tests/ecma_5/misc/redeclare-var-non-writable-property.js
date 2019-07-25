





var BUGNUMBER = 539488;
var summary =
  '|var| statements for existing, read-only/permanent properties should not ' +
  'be errors';

print(BUGNUMBER + ": " + summary);





var undefined;



reportCompare(true, true);

print("All tests passed!");
