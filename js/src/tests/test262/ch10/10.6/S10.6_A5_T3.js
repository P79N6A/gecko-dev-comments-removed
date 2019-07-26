











function f1(){
  return (delete arguments.length); 
}

try{
  if(!f1()){
    $ERROR("#1: A property length have attribute { DontDelete }");
  }
}
catch(e){
  $ERROR("#1: arguments object don't exists");
}


var f2 = function(){
  return (delete arguments.length); 
}

try{
  if(!f2()){
    $ERROR("#2: A property length have attribute { DontDelete }");
  }
}
catch(e){
  $ERROR("#2: arguments object don't exists");
}

