










function f1(){
  return arguments.length;
}


if(!(f1() === 0)){
  $ERROR('#1: argument.length === 0');
}


if(!(f1(0) === 1)){
  $ERROR('#2: argument.length === 1');
}


if(!(f1(0, 1) === 2)){
  $ERROR('#3: argument.length === 2');
}


if(!(f1(0, 1, 2) === 3)){
  $ERROR('#4: argument.length === 3');
}


if(!(f1(0, 1, 2, 3) === 4)){
  $ERROR('#5: argument.length === 4');
}

var f2 = function(){return arguments.length;};


if(!(f2() === 0)){
  $ERROR('#6: argument.length === 0');
}


if(!(f2(0) === 1)){
  $ERROR('#7: argument.length === 1');
}


if(!(f2(0, 1) === 2)){
  $ERROR('#8: argument.length === 2');
}


if(!(f2(0, 1, 2) === 3)){
  $ERROR('#9: argument.length === 3');
}


if(!(f2(0, 1, 2, 3) === 4)){
  $ERROR('#10: argument.length === 4');
}

