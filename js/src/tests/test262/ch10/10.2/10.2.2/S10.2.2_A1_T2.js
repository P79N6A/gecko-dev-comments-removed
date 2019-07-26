











var x = 0;

function f1(){
  function f2(){
    return x;
  };
  return f2();
}

if(!(f1() === 0)){
  $ERROR("#1: Scope chain disturbed");
}

