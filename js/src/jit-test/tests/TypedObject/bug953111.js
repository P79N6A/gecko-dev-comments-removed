




if (!this.hasOwnProperty("TypedObject"))
  quit();



var A = TypedObject.uint8.array(0);
var a = new A();
a.forEach(function(val, i) {});



var AA = TypedObject.uint8.array(2147483647).array(0);
var aa = new AA();
