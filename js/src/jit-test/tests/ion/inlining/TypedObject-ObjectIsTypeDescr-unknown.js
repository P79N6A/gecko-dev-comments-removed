



















var T = TypedObject;
var ST = new T.StructType({x:T.int32});

function check(v) {
    return v.toSource();
}

function test() {
    var fake = { toSource: ST.toSource };
    var a = [ ST, fake ];
    for ( var i=0 ; i < 1000 ; i++ )
	try { check(a[i%2]); } catch (e) {}
    try { return check(a[0]); } catch (e) { return "Thrown" }
}

print(test());
