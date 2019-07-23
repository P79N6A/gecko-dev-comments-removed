





































var gTestfile = 'regress-289094.js';

var BUGNUMBER = 289094;
var summary = 'argument shadowing function property special case for lambdas';
var actual = '';
var expect = 'function:function';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function fn()
{
  var o = {
    d: function(x,y) {},
    init: function() { this.d.x = function(x) {}; this.d.y = function(y) {}; }
  };
  o.init();
  actual = typeof(o.d.x) + ':' + typeof(o.d.y);
}

fn();

reportCompare(expect, actual, summary);
