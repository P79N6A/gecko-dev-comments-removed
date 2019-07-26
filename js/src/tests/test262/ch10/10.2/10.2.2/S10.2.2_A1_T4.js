











var x = 0;

function f1(){
  function f2(){
    return x;
  };

  var x = 1;
  return f2();
}

if(!(f1() === 1)){
  $ERROR("#1: Scope chain disturbed");
}

