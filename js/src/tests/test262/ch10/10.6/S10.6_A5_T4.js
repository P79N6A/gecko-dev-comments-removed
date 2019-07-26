










var str = "something different";

function f1(){
  arguments.length = str;
  return arguments;
}

try{
  if(f1().length !== str){
    $ERROR("#1: A property length have attribute { ReadOnly }");
  }
}
catch(e){
  $ERROR("#1: arguments object don't exists");
}


var f2 = function(){
    arguments.length = str;
    return arguments;
  };
try{
  if(f2().length !== str){
    $ERROR("#2: A property length have attribute { ReadOnly }");
  }
}
catch(e){
  $ERROR("#2: arguments object don't exists");
}

