



var obj = {a: 0, b: 1, c: 2};
delete obj.b;  
Object.defineProperty(obj, 'g',
                      {get: function () { return -1; }, configurable: true});
for (var i = 3; i < 20; i++)
    obj['x' + i] = i;  
for (var i = 3; i < 20; i++)
    delete obj['x' + i];  
delete obj.g;  


gc();
obj.d = 3;
obj.e = 4;

reportCompare(0, 0, 'ok');
