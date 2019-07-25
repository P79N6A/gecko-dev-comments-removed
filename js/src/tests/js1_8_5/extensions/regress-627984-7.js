



var obj = {m: function () {}};
obj.watch("m", function () { throw 'FAIL'; });
var f = obj.m;  

reportCompare(0, 0, 'ok');
