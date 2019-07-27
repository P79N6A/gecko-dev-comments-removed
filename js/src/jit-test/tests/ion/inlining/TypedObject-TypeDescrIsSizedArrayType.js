
















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var IT = new T.ArrayType(T.int32, 100);
var ix = IT.build(x => x == 0 ? 99 : x-1);  


function check(v) {
    return v.scatter(IT, ix);
}

function test() {
    var w = IT.build(x => x);
    for ( var i=0 ; i < 77 ; i++ )
	w = check(w);
    return w;
}

w = test();
assertEq(w.length, 100);
assertEq(w[0], 77);
print("Done");
