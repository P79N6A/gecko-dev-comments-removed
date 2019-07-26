











var str = "something different";

function f1(){
  arguments.callee = str;
  return arguments;
}

try{
  if(f1().callee !== str){
    $ERROR("#1: A property callee have attribute { ReadOnly }");
  }
}
catch(e){
  $ERROR("#1: arguments object don't exists");
}


var f2 = function(){
    arguments.callee = str;
    return arguments;
  }
try{
  if(f2().callee !== str){
    $ERROR("#2: A property callee have attribute { ReadOnly }");
  }
}
catch(e){
  $ERROR("#2: arguments object don't exists");
}

