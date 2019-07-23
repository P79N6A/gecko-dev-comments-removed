





































gTestfile = 'regress-383255.js';

var summary = 'Do not assert: JS_UPTRDIFF(fp->sp, fp->spbase) <= depthdiff';
var BUGNUMBER = 383255;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    <x/>[0]++;
}
catch(ex)
{
    print(ex);
}

TEST(1, expect, actual)

END();
