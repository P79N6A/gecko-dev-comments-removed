




































gTestfile = '11.4.7-02.js';







var SECTION = "11.4.7";
var VERSION = "ECMA";
var TITLE   = "Unary - Operator";
var BUGNUMBER = "432881";

startTest();

test_negation(0, -0.0);
test_negation(-0.0, 0);
test_negation(1, -1);
test_negation(1.0/0.0, -1.0/0.0);
test_negation(-1.0/0.0, 1.0/0.0);


test_negation(1073741824, -1073741824);
test_negation(-1073741824, 1073741824);


test_negation(1073741823, -1073741823);
test_negation(-1073741823, 1073741823);


test_negation(1073741824, -1073741824);
test_negation(-1073741824, 1073741824);


test_negation(1073741823, -1073741823);
test_negation(-1073741823, 1073741823);


test_negation(2147483648, -2147483648);
test_negation(-2147483648, 2147483648);


test_negation(2147483647, -2147483647);
test_negation(-2147483647, 2147483647);

function test_negation(value, expected)
{
    var actual = -value;
    reportCompare(expected, actual, '-(' + value + ') == ' + expected);
} 
