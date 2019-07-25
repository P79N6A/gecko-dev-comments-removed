var obj = {s: ""};
var name = "s";
for (var i = 0; i <= RECORDLOOP + 5; i++)
    if (i > RECORDLOOP)
        obj[name]++;  
assertEq(obj.s, 5);

