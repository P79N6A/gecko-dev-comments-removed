












var x = 0;

var myObj = {x : "obj"};

function f1(){
  function f2(){
    with(myObj){
      return x;
    }
  };
  return f2();

  var x = 1;
}

if(!(f1() === "obj")){
  $ERROR("#1: Scope chain disturbed");
}

