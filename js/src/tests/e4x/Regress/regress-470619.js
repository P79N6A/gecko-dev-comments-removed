






































var summary = 'Do not assert: regs.sp - 2 >= StackBase(fp)';
var BUGNUMBER = 470619;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    y = <x/>;
    for (var z = 0; z < 2; ++z) { +y };
}
catch(ex)
{
    print(ex + '');
}
TEST(1, expect, actual);

END();
