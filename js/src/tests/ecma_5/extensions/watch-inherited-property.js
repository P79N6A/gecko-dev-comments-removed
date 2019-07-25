





var protoSetterCount;
var proto = ({ set x(v) { protoSetterCount++; } });


var protoWatchCount;
proto.watch('x', function() { protoWatchCount++; });


function C() { }
C.prototype = proto;
var o = new C();







var oWatchCount;
o.watch('x', function() { oWatchCount++; });





protoSetterCount = protoWatchCount = oWatchCount = 0;
o.x = 1;
assertEq(protoWatchCount, 0);
assertEq(oWatchCount, 1);
assertEq(protoSetterCount, 1);

reportCompare(true, true);
