












function f_arg(x,y,z) {
  return z;
}


if (f_arg(x=1,y=x,x+y) !== 2) {
  $ERROR('#1: function f_arg(x,y,z) {return z;} f_arg(x=1,y=x,x+y) === 2. Actual: ' + (f_arg(x=1,y=x,x+y)));    
}

