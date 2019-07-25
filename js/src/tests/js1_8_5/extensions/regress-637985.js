


var obj = {};
obj.watch(-1, function(){});
obj.unwatch("-1");  

reportCompare(0, 0, 'ok');