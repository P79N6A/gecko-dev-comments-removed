









var y;

function f(){
  var x;
  
  if(x === undefined) {
    x = 0;
  } else {
    x = 1;
  }
  
  return x;
}

y = f();
y = f();

if(!(y === 0)){
  $ERROR("#1: Sequenced function calls shares execution context");
}

