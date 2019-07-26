











var x = 0;

function f1(){
  var x = 1;
  function f2(){
    return x;
  };
  return f2();
}

if(!(f1() === 1)){
  $ERROR("#1: Scope chain disturbed");
}

