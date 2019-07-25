





var watcherCount;
function watcher(id, oldval, newval) { watcherCount++; return newval; }


var setterCount;
var o = { w:2, set x(v) { setterCount++; } };





delete o.w;





o.watch('x', watcher);






Object.defineProperty(o, 'x', { value:3,
                                writable:true,
                                enumerable:true,
                                configurable:true });






watcherCount = setterCount = 0;
o.x = 3;
assertEq(watcherCount, 1);
assertEq(setterCount, 0);

reportCompare(true, true);
