


















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var AT = new T.ArrayType(T.uint32);

function check() {
    return AT.build(100, x => x+1);
}

function test() {
    var w;
    for ( var i=0 ; i < 100 ; i++ )
	w = check();
    return w;
}

var w = test();
assertEq(w.length, 100);
assertEq(w[99], 100);
print("Done");

