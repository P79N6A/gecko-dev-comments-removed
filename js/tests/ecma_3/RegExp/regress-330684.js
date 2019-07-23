




































var gTestfile = 'regress-330684.js';

var BUGNUMBER = 330684;
var summary = 'Do not hang on RegExp';
var actual = 'Do not hang on RegExp';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var re = /^(?:(?:%[0-9A-Fa-f]{2})*[!\$&'\*-;=\?-Z_a-z]*)+$/;
var url = "http://tw.yimg.com/a/tw/wenchuan/cam_240x400_381615_030806_2.swf?clickTAG=javascript:VRECopenWindow(1)";

printStatus(re.test(url));

reportCompare(expect, actual, summary);
