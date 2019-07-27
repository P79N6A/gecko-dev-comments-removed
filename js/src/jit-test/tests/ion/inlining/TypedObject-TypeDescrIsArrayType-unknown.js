
























if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var AT = new T.ArrayType(T.int32, 100);

function check(v) {
    return v.map(x => x+1);
}

function test() {
    var w1 = AT.build(x => x+1);
    var w2 = Array.build(100, x => x+1);
    w2.map = w1.map;
    var a = [ w1, w2 ];
    for ( var i=0 ; i < 2000 ; i++ )
	try { a[i%2] = check(a[i%2]); } catch (e) { assertEq( i%2, 1 ); }
    return a[0];
}

var w = test();
assertEq(w.length, 100);
assertEq(w[99], 1100);
print("Done");
