


if (typeof TypedObject === "undefined")
  quit();

load(libdir + "asserts.js")

var StructType = TypedObject.StructType;
var uint32 = TypedObject.uint32;

function main(variant)
{
  var Point = new StructType({ x: uint32, y: uint32 });
  var Line = new StructType({ from: Point, to: Point });

  var buf = new ArrayBuffer(16);
  var line = new Line(buf);

  assertThrowsInstanceOf(function()
  {
    line.to = { x: 22, get y() { neuter(buf, variant); return 44; } };
  }, TypeError, "setting into a neutered buffer is bad mojo");
}

main("same-data");
main("change-data");
