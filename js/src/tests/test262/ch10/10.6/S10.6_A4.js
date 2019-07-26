












function f1(){
  return arguments.callee;
}

try{
  if(f1 !== f1()){
    $ERROR('#1: arguments.callee === f1');
  }
}
catch(e){
  $ERROR("#1: arguments object doesn't exists");
}


var f2 = function(){return arguments.callee;};

try{
  if(f2 !== f2()){
    $ERROR('#2: arguments.callee === f2');
  }
}
catch(e){
  $ERROR("#1: arguments object doesn't exists");
}

