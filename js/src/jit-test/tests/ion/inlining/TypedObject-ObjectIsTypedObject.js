




















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var ST = new T.StructType({x:T.int32});
var v = new ST({x:10});

function check(v) {
  return T.objectType(v);
}

function test() {
  for ( var i=0 ; i < 1000 ; i++ )
    assertEq(check(v), ST);
  return check(v);
}

print("Done");


