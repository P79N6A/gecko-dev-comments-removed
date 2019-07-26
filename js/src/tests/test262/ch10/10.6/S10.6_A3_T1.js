











function f1(){
  return arguments.hasOwnProperty("callee");
}
try{
  if(f1() !== true){
    $ERROR("#1: arguments object doesn't contains property 'callee'");
  }
}
catch(e){
  $ERROR("#1: arguments object doesn't exists");
}


var f2 = function(){return arguments.hasOwnProperty("callee");};
try{
  if(f2() !== true){
    $ERROR("#2: arguments object doesn't contains property 'callee'");
  }
}
catch(e){
  $ERROR("#2: arguments object doesn't exists");
}

