












var __obj={};
if (!Object.prototype.isPrototypeOf(__obj)){
  $ERROR('#1: Native ECMAScript objects have an internal property called [[Prototype]]. ');
};







var protoObj={};

function FooObj(){};




var obj__= new FooObj;

if (!Object.prototype.isPrototypeOf(obj__)){
  $ERROR('#2.1: protoObj={}; function FooObj(){}; var obj__= new FooObj; Object.prototype.isPrototypeOf(obj__) === true. Actual: ' + (Object.prototype.isPrototypeOf(obj__)));
};

if (!FooObj.prototype.isPrototypeOf(obj__)){
  $ERROR('#2.2: protoObj={}; function FooObj(){}; var obj__= new FooObj; FooObj.prototype.isPrototypeOf(obj__) === true. Actual: ' + (FooObj.prototype.isPrototypeOf(obj__)));
};

if (protoObj.isPrototypeOf(obj__)){
  $ERROR('#2.3: protoObj={}; function FooObj(){}; var obj__= new FooObj; protoObj.isPrototypeOf(obj__) === false. Actual: ' + (protoObj.isPrototypeOf(obj__)));
};

FooObj.prototype=protoObj;

if (protoObj.isPrototypeOf(obj__)){
  $ERROR('#2.4: protoObj={}; function FooObj(){}; var obj__= new FooObj; FooObj.prototype=protoObj; protoObj.isPrototypeOf(obj__) === false. Actual: ' + (protoObj.isPrototypeOf(obj__)));
};







var __foo=new FooObj;

if (!Object.prototype.isPrototypeOf(__foo)){
  $ERROR('#3.1: protoObj={}; function FooObj(){}; var obj__= new FooObj; FooObj.prototype=protoObj; var __foo=new FooObj; Object.prototype.isPrototypeOf(__foo) === true. Actual: ' + (Object.prototype.isPrototypeOf(__foo)));
};

if (!FooObj.prototype.isPrototypeOf(__foo)){
  $ERROR('#3.2: protoObj={}; function FooObj(){}; var obj__= new FooObj; FooObj.prototype=protoObj; var __foo=new FooObj; FooObj.prototype.isPrototypeOf(__foo) === true. Actual: ' + (FooObj.prototype.isPrototypeOf(__foo)));
};

if (!protoObj.isPrototypeOf(__foo)){
  $ERROR('#3.3: protoObj={}; function FooObj(){}; var obj__= new FooObj; FooObj.prototype=protoObj; var __foo=new FooObj; protoObj.isPrototypeOf(__foo) === true. Actual: ' + (protoObj.isPrototypeOf(__foo)));
};



