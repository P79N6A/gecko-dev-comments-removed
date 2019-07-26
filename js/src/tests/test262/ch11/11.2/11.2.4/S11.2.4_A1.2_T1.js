









f_arg = function() {
  return arguments;
}


if (f_arg(1,2,3).length !== 3) {
  $ERROR('#1: f_arg = function()() {return arguments;} f_arg(1,2,3).length === 3. Actual: ' + (f_arg(1,2,3).length));
}


if (f_arg(1,2,3)[0] !== 1) {
  $ERROR('#1: f_arg = function()() {return arguments;} f_arg(1,2,3)[0] === 1. Actual: ' + (f_arg(1,2,3)[0]));
}


if (f_arg(1,2,3)[1] !== 2) {
  $ERROR('#3: f_arg = function()() {return arguments;} f_arg(1,2,3)[1] === 2. Actual: ' + (f_arg(1,2,3)[1]));
}


if (f_arg(1,2,3)[2] !== 3) {
  $ERROR('#4: f_arg = function()() {return arguments;} f_arg(1,2,3)[2] === 3. Actual: ' + (f_arg(1,2,3)[2]));
}


if (f_arg(1,2,3)[3] !== undefined) {
  $ERROR('#5: f_arg = function()() {return arguments;} f_arg(1,2,3)[3] === undefined. Actual: ' + (f_arg(1,2,3)[3]));
}

