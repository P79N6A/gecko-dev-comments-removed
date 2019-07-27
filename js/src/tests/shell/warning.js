

var BUGNUMBER = 1170716;
var summary = 'Add js shell functions to get last warning';

print(BUGNUMBER + ": " + summary);



enableLastWarning();

eval(`({}).__proto__ = {};`);

var warning = getLastWarning();
assertEq(warning !== null, true);
assertEq(warning.name, "None");
assertEq(warning.message.includes("mutating"), true);
assertEq(warning.lineNumber, 1);
assertEq(warning.columnNumber, 1);



clearLastWarning();
warning = getLastWarning();
assertEq(warning, null);



options("strict");
eval(`var a; if (a=0) {}`);

warning = getLastWarning();
assertEq(warning !== null, true);
assertEq(warning.name, "SyntaxError");
assertEq(warning.message.includes("equality"), true);
assertEq(warning.lineNumber, 1);
assertEq(warning.columnNumber, 14);



disableLastWarning();

eval(`var a; if (a=0) {}`);

enableLastWarning();
warning = getLastWarning();
assertEq(warning, null);

disableLastWarning();

if (typeof reportCompare === "function")
  reportCompare(true, true);
