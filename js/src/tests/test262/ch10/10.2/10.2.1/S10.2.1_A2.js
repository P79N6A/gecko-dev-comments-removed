













function f1(x, x) {
  return x;
}
if(!(f1(1, 2) === 2)) {
  $ERROR("#1: f1(1, 2) === 2");
}


function f2(x, x, x){
  return x*x*x;
}
if(!(f2(1, 2, 3) === 27)){
  $ERROR("f2(1, 2, 3) === 27");
}


function f3(x, x) {
  return 'a' + x;
}
if(!(f3(1, 2) === 'a2')){
  $ERROR("#3: f3(1, 2) === 'a2'");
}

