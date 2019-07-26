



















var T = TypedObject;
var ST = new T.StructType({x:T.int32});

function check(v) {
    return v.toSource();
}

function test() {
    for ( var i=0 ; i < 1000 ; i++ )
	check(ST);
    return check(ST);
}

print(test());


