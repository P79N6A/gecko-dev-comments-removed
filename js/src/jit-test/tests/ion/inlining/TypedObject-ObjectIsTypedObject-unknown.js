






















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var ST1 = new T.StructType({x:T.int32});
var v1 = new ST1({x:10});

function check(v) {
    return T.objectType(v);
}

function test() {
    var v2 = { tag: "Hello, world!" };
    var a = [ v1, v2 ];
    for ( var i=0 ; i < 1000 ; i++ )
	assertEq(check(a[i%2]), (i%2) == 0 ? ST1 : T.Object);
    return check(a[i%2]);
}

print("Done");


