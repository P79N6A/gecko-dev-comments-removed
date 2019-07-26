










function MyObject( val ) {
    this.value = val;
    this.valueOf = function (){ return this.value; }
}

var x = new MyObject(1);
var y = Object(x);


if (y.valueOf() !== x.valueOf()){
  $ERROR('#1: Object(obj).valueOf() === obj.valueOf(). Actual: ' + (Object(obj).valueOf()));
}


if (typeof y !== typeof x){
  $ERROR('#2: typeof Object(obj) === typeof obj. Actual: ' + (typeof Object(obj)));
}


if (y.constructor.prototype !== x.constructor.prototype){
  $ERROR('#3: Object(obj).constructor.prototype === obj.constructor.prototype. Actual: ' + (Object(obj).constructor.prototype));
}



if (y !== x){
  $ERROR('#4: Object(obj) === obj');
}

