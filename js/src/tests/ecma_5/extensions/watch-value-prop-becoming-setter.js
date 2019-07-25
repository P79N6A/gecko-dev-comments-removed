





var watcherCount;
function watcher(id, old, newval) {
    watcherCount++;
    return newval; 
}


var o = { w:2, x:3 };





o.watch('x', watcher);





delete o.w;




var setterCount;
o.__defineSetter__('x', function() { setterCount++; });





watcherCount = setterCount = 0;
o.x = 4;
assertEq(watcherCount, 1);
assertEq(setterCount, 1);

reportCompare(true, true);
