
var p = /./, x = resolver({}, p), y = resolver({lastIndex: 2}, p), v;
var a = [];
for (var i = 0; i < HOTLOOP; i++)
    a[i] = x;
a[HOTLOOP] = y;
for (i = 0; i < a.length; i++)
    v = a[i].lastIndex;
assertEq(v, 2);  
