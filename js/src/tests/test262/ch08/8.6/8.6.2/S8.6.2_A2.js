











function FooObj(){};
FooObj.prototype.prop="some";


var foo= new FooObj;



if (foo.prop !== "some"){
  $ERROR('#1: function FooObj(){}; FooObj.prototype.prop="some"; var foo= new FooObj; foo.prop === "some". Actual: ' + (foo.prop));
}





foo.prop=true;

var foo__ = new FooObj;
if (foo__.prop !== "some"){
  $ERROR('#2: function FooObj(){}; FooObj.prototype.prop="some"; var foo= new FooObj; foo.prop=true; var foo__ = new FooObj; foo__.prop === "some". Actual: ' + (foo__.prop));
}



