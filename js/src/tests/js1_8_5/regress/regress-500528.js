







var a = {x: 'a'},
    b1 = Object.create(a),
    c1 = Object.create(b1),
    b2 = Object.create(a),
    c2 = Object.create(b2);

b2.x = 'b';  

var s = '';
for each (var obj in [c1, c2])
    s += obj.x;
assertEq(s, 'ab');

print(" PASSED! Property cache soundness: objects with the same shape but different prototypes.");
