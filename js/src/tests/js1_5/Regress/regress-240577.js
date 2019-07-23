






var gTestfile = 'regress-240577.js';


var BUGNUMBER = 240577;
var summary = 'object.watch execution context';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var createWatcher = function ( watchlabel )
{
  var watcher = function (property, oldvalue, newvalue)
  {
    actual += watchlabel; return newvalue;
  };
  return watcher;
};

var watcher1 = createWatcher('watcher1');

var object = {property: 'value'};

object.watch('property', watcher1);

object.property = 'newvalue';

expect = 'watcher1';

reportCompare(expect, actual, summary);
