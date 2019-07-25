
var re = /(?:^(a)|\1(a)|(ab)){2}/;
var str = 'aab';
var actual = re.exec(str);
var expected = makeExpectedMatch(['aa', undefined, 'a', undefined], 0, str);
checkRegExpMatch(actual, expected);

if (typeof reportCompare === 'function')
    reportCompare(true, true);
