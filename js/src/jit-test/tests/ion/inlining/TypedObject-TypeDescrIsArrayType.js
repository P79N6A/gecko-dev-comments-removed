




















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var AT = new T.ArrayType(T.int32);

function check(v) {
    return v.map(x => x+1);
}

function test() {
    var w = new AT(100);
    for ( var i=0 ; i < 1000 ; i++ )
	w = check(w);
    return w;
}

var w = test();
assertEq(w.length, 100);
assertEq(w[99], 1000);
print("Done");

