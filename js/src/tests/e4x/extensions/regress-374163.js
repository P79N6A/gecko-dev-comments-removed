







































var BUGNUMBER = 374163;
var summary =
  "Set E4X xml.function::__proto__ = null shouldn't affect " +
  "xml.[[DefaultValue]]";
var actual = 'unset';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var a = <a/>;
a.function::__proto__ = null;
actual = "" + a;

TEST(1, expect, actual);
END();
