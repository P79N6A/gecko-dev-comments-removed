




































var gTestfile = 'regress-381304.js';

var BUGNUMBER = 381304;
var summary = 'getter/setter with keywords';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj;

  print('1');

  obj = {
    set inn(value) {this.for = value;}, 
    get inn() {return this.for;}
  };

  expect = '({get inn() {return this.for;}, set inn(value) {this.for = value;}})';
  actual = obj.toSource();
  compareSource(expect, actual, summary + ': 1');

  print('2');

  obj = {
    set in(value) {this.for = value;}, 
    get in() {return this.for;}
  };

  expect = '( { in getter : ( function ( ) { return this . for ; } ) , in setter : ( function ( value ) { this . for = value ; } ) } )';
  actual = obj.toSource();
  compareSource(expect, actual, summary + ': 2');

  print('3');

  obj = {
    get in(value) {this.for = value;}, 
    set in() {return this.for;}
  };

  expect = '( { in getter : ( function ( value ) { this . for = value ; } ) , in setter : ( function ( ) { return this . for ; } ) } ) ';
  actual = obj.toSource();
  compareSource(expect, actual, summary + ': 3');

  print('4');

  obj = {
    set inn(value) {this.for = value;}, 
    get in() {return this.for;}
  };

  expect = '( { set inn ( value ) { this . for = value ; } , in getter : ( function ( ) { return this . for ; } ) } )';
  actual = obj.toSource();
  compareSource(expect, actual, summary + ': 4');

  print('5');

  obj = {
    set in(value) {this.for = value;}, 
    get inn() {return this.for;}
  };

  expect = ' ( { in setter : ( function ( value ) { this . for = value ; } ) , get inn ( ) { return this . for ; } } ) ';
  actual = obj.toSource();
  compareSource(expect, actual, summary + ': 5');
  exitFunc ('test');
}
