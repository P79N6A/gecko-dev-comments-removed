







if (!this.hasOwnProperty("TypedObject"))
  quit();

var {StructType,uint8,float32} = TypedObject;
var RgbColor2 = new StructType({r: uint8, g: float32, b: uint8});
RgbColor2.prototype.add = function(c) {
  this.g += c;
  this.b += c;
};
var gray = new RgbColor2({r: 129, g: 128, b: 127});
gray.add(1);
gray.add(2);

