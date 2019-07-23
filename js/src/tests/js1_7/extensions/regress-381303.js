




































var gTestfile = 'regress-381303.js';


var BUGNUMBER = 381303;
var summary = 'object toSource when a property has both a getter and a setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj = {set inn(value) {this.for = value;}, get inn() {return this.for;}};
  expect = '( { ' + 
    'get inn() {return this.for;}' + 
    ', ' + 
    'set inn(value) {this.for = value;}' + 
    '})';
  actual = obj.toSource();

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
