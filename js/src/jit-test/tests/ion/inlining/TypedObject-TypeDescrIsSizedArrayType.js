
















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var AT = new T.ArrayType(T.int32);
var IT = AT.dimension(100);
var ix = AT.build(100, x => x == 0 ? 99 : x-1);  


function check(v) {
    return v.scatter(IT, ix);
}

function test() {
    var w = AT.build(100, x => x);
    for ( var i=0 ; i < 77 ; i++ )
	w = check(w);
    return w;
}

w = test();
assertEq(w.length, 100);
assertEq(w[0], 77);
print("Done");
