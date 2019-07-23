







































gTestfile = 'regress-277935.js';

START('XML("") should create empty text node');
printBugNumber(277935);



var expect = 'PASS1,PASS2';
var actual = '';

var msg =  <foo><bar><s/></bar></foo>;

try {
    eval('msg..s = 0');
    SHOULD_THROW(1);
} catch (e) {
    actual += 'PASS1';
}

try {
    eval('msg..s += 0');
    SHOULD_THROW(2);
} catch (e) {
    actual += ',PASS2';
}

TEST(1, expect, actual);
END();
