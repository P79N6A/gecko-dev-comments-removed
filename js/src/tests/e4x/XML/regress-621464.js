






































var BUGNUMBER = 621464;
var summary = '<x>a</x>.replace() == <x>a</x>';

printBugNumber(BUGNUMBER);
START(summary);

var expected = <x>a</x>;
var actual = <x>a</x>.replace();

TEST(0, expected, actual);

END();
