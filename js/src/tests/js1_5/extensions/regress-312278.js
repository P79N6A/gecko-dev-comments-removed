




































var gTestfile = 'regress-312278.js';

var BUGNUMBER = 312278;
var summary = 'Do not access GC-ed object in Error.prototype.toSource';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function wrapInsideWith(obj)
{
  var f;
  with (obj) {
    f = function() { }
  }
  return parent(f);
}

function customToSource()
{
  return "customToSource "+this;
}

Error.prototype.__defineGetter__('message', function() {
				   var obj = {
				     toSource: "something"
				   }
				   obj.__defineGetter__('toSource', function() {
							  gc();
							  return customToSource;
							});
				   return wrapInsideWith(obj);
				 });

printStatus(Error.prototype.toSource());

reportCompare(expect, actual, summary);
