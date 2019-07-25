





var firstSetterCount;
var o = { w:2, set x(v) { firstSetterCount++; } };





delete o.w;


var watcherCount;
function watcher(id, oldval, newval) { watcherCount++; return newval; }






o.watch('x', watcher);






var secondSetterCount;
Object.defineProperty(o, 'x', { set: function () { secondSetterCount++ } });




watcherCount = firstSetterCount = secondSetterCount = 0;
o.x = 3;
assertEq(watcherCount, 1);
assertEq(firstSetterCount, 0);
assertEq(secondSetterCount, 1);

reportCompare(true, true);
