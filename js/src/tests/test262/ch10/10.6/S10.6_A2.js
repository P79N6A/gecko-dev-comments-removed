












function f1(){
  return arguments.constructor.prototype;
}
try{
  if(f1() !== Object.prototype){
    $ERROR('#1: arguments.constructor.prototype === Object.prototype');
  }
}
catch(e){
  $ERROR("#1: arguments doesn't exists");
}


var f2 = function(){return arguments.constructor.prototype;};
try{
  if(f2() !== Object.prototype){
    $ERROR('#2: arguments.constructor.prototype === Object.prototype');
  }
}
catch(e){
  $ERROR("#2: arguments doesn't exists");
}

