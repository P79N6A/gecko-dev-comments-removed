











var x = 0;

function f1(){
  function f2(){
    return x;
  };
  return f2();
  
  var x = 1;
}

if(!(f1() === undefined)){
  $ERROR("#1: Scope chain disturbed");
}


