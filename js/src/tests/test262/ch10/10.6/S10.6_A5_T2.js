











function f1(){
  for(var x in arguments){
    if (x === "length"){
      return false;
    }
  }
  return true;
}

try{
  if(!f1()){
    $ERROR("#1: A property length don't have attribute { DontEnum }");
  }
}
catch(e){
  $ERROR("#1: arguments object don't exists");
}


var f2 = function(){
  for(var x in arguments){
    if (x === "length"){
      return false;
    }
  }
  return true;
}

try{
  if(!f2()){
    $ERROR("#2: A property length don't have attribute { DontEnum }");
  }
}
catch(e){
  $ERROR("#2: arguments object don't exists");
}

