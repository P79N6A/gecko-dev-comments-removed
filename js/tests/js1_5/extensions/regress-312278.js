




































var bug = 312278;
var summary = 'Do no access GC-ed object in Error.prototype.toSource';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);
  
function wrapInsideWith(obj)
{
  var f;
  with (obj) {
    f = function() { }
  }
  return f.__parent__;
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
