











function f1(a, b){
  return (b === undefined);
}
if(!(f1(1, 2) === false)){
  $ERROR('#1: f1(1, 2) === false');
} else if(!(f1(1) === true)){
  $ERROR('#1: f1(1) === true');
}


function f2(a, b, c){
  return (b === undefined) && (c === undefined);
}
if(!(f2(1) === true)){
  $ERROR('#2: f2(1, 2) === true');
}

