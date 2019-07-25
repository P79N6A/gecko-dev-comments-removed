


var obj = {set x(v) {}};
obj.watch("x", function() { delete obj.x; });
obj.x = "hi";  

reportCompare(0, 0, 'ok');
