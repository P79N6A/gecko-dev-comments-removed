




































var bug = 307456;
var summary = 'Do not Freeze with RegExp';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var data='<!---<->---->\n\n<><>--!!!<><><><><><>\n!!<>\n\n<>\n<><><><>!\n\n\n\n--\n--\n--\n\n--\n--\n\n\n-------\n--\n--\n\n\n--\n\n\n\n----\n\n\n\n--\n\n\n-\n\n\n-\n\n-\n\n-\n\n-\n-\n\n----\n\n-\n\n\n\n\n-\n\n\n\n\n\n\n\n\n-----\n\n\n-\n------\n-------\n\n----\n\n\n\n!\n\n\n\n\n\n\n\n!!!\n\n\n--------\n\n\n\n-\n\n\n-\n--\n\n----\n\n\n\n\n\n-\n\n\n----\n\n\n\n\n\n--------\n!\n\n\n\n\n-\n---\n--\n\n----\n\n-\n\n-\n\n-\n\n\n\n-----\n\n\n\n-\n\n\n-\n\n\n--\n-\n\n\n-\n\n----\n\n---\n\n---\n\n----\n\n\n\n---\n\n-++\n\n-------<>\n\n-!\n\n--\n\n----!-\n\n\n\n';

printStatus(data);
data=data.replace(RegExp('<!--(\\n[^\\n]|[^-]|-[^-]|--[^>])*-->', 'g'), '');
printStatus(data);

reportCompare(expect, actual, summary);
