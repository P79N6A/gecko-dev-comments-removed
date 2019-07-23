




































var gTestfile = 'regress-294195-01.js';

var BUGNUMBER = 294195;
var summary = 'Do not crash during String replace when accessing methods on backreferences';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var s = 'some text sample';


var result = s.replace(new RegExp('(^|\\s)(text)'), (new String('$1')));
result = result.substr(0, 1);
reportCompare(expect, actual, inSection(1) + ' ' + summary);


result = s.replace(new RegExp('(^|\\s)(text)'),
                   (new String('$1')).substr(0, 1));
reportCompare(expect, actual, inSection(2) + ' ' + summary);
