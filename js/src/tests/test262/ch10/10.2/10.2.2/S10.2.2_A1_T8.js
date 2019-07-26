












var x = 0;

var myObj = {x : "obj"};

function f1(){
  function f2(){
    with(myObj){
      return x;
    }
  };

  var x = 1;
  return f2();
}

if(!(f1() === "obj")){
  $ERROR("#1: Scope chain disturbed");
}

