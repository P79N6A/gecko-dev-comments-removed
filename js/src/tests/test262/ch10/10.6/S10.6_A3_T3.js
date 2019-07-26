












function f1(){
  return (delete arguments.callee);
}

try{
  if(!f1()){
    $ERROR("#1: A property callee have attribute { DontDelete }");
  }
}
catch(e){
  $ERROR("#1: arguments object don't exists");
}


var f2 = function(){
  return (delete arguments.callee);
}

try{
  if(!f2()){
    $ERROR("#2: A property callee have attribute { DontDelete }");
  }
}
catch(e){
  $ERROR("#2: arguments object don't exists");
}

