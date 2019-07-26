
quit();






if (!this.hasOwnProperty("TypedObject"))
  quit();

var {StructType, uint32, storage} = TypedObject;
var S = new StructType({f: uint32, g: uint32});
var A = S.array(10);

function readFrom(a) {
  return a[2].f + a[2].g;
}

function main() {
  var a = new A();
  a[2].f = 22;
  a[2].g = 44;

  for (var i = 0; i < 10; i++)
    assertEq(readFrom(a), 66);

  neuter(storage(a).buffer);

  for (var i = 0; i < 10; i++) {
    var ok = false;

    try {
      readFrom(a);
    } catch (e) {
      ok = e instanceof TypeError;
    }

    assertEq(ok, true);
  }
}

main();
