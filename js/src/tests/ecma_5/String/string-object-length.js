





var BUGNUMBER = 650621;
var summary = 'String object length test';

print(BUGNUMBER + ": " + summary);





assertEq(raisesException(InternalError)('for (args = "" ;;) args+=new String(args)+1'), true);

reportCompare(true, true);

