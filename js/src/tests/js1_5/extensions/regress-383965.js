




































var gTestfile = 'regress-383965.js';

var BUGNUMBER = 383965;
var summary = 'getter function with toSource';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = /({get aaa :{}})|({aaa:{}})/;

getter function aaa(){};
var obj = {};
var gett = this.__lookupGetter__("aaa");
gett.__proto__ = obj;
obj.__defineGetter__("aaa", gett);
actual = obj.toSource();

reportMatch(expect, actual, summary);
