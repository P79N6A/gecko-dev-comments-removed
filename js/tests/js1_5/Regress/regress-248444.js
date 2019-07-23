





































var bug = 248444;
var summary = 'toString/toSource of RegExp should escape slashes';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var re;
expect = '/[^\\/]+$/';

status = summary + ' ' + inSection(1);
re = /[^\/]+$/;
actual = re.toString();
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2);
re = RegExp("[^\\\/]+$");
actual = re.toString();
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3);
re = RegExp("[^\\/]+$");
actual = re.toString();
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(4);
re = RegExp("[^\/]+$");
actual = re.toString();
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(5);
re = RegExp("[^/]+$");
actual = re.toString();
reportCompare(expect, actual, status);


