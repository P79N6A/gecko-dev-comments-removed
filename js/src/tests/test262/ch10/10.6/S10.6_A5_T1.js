











function f1(){
  return arguments.hasOwnProperty("length");
}
try{
  if(f1() !== true){
    $ERROR("#1: arguments object doesn't contains property 'length'");
  }
}
catch(e){
  $ERROR("#1: arguments object doesn't exists");
}


var f2 = function(){return arguments.hasOwnProperty("length");};
try{
  if(f2() !== true){
    $ERROR("#2: arguments object doesn't contains property 'length'");
  }
}
catch(e){
  $ERROR("#2: arguments object doesn't exists");
}

