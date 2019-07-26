









f_arg = function(x,y) {
  return arguments;
}


if (f_arg(1,2,3).length !== 3) {
  $ERROR('#1: f_arg = function(x,y) {return arguments;} f_arg(1,2,3).length === 3. Actual: ' + (f_arg(1,2,3).length));
}


if (f_arg(1)[0] !== 1) {
  $ERROR('#1: f_arg = function(x,y) {return arguments;} f_arg(1)[0] === 1. Actual: ' + (f_arg(1)[0]));
}


if (f_arg(1,2)[1] !== 2) {
  $ERROR('#3: f_arg = function(x,y) {return arguments;} f_arg(1,2)[1] === 2. Actual: ' + (f_arg(1,2)[1]));
}


if (f_arg(1,2,3)[2] !== 3) {
  $ERROR('#4: f_arg = function(x,y) {return arguments;} f_arg(1,2,3)[2] === 3. Actual: ' + (f_arg(1,2,3)[2]));
}


if (f_arg(1,2,3)[3] !== undefined) {
  $ERROR('#5: f_arg = function(x,y) {return arguments;} f_arg(1,2,3)[3] === undefined. Actual: ' + (f_arg(1,2,3)[3]));
}


if (f_arg.length !== 2) {
  $ERROR('#6: f_arg = function(x,y) {return arguments;} f_arg.length === 2. Actual: ' + (f_arg.length));
}

