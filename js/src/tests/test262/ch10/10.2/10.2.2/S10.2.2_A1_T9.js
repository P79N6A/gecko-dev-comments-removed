












var x = 0;

var myObj = {x : "obj"};

function f1(){
  with(myObj){
    return x;
  }
}

if(!(f1() === "obj")){
  $ERROR("#1: Scope chain disturbed");
}

