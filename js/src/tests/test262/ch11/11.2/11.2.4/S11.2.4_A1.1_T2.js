









function f_arg(x,y) {
  return arguments;
}


if (f_arg().length !== 0) {
  $ERROR('#1: function f_arg(x,y) {return arguments;} f_arg().length === 0. Actual: ' + (f_arg().length));
}


if (f_arg()[0] !== undefined) {
  $ERROR('#2: function f_arg(x,y) {return arguments;} f_arg()[0] === undefined. Actual: ' + (f_arg()[0]));
}


if (f_arg.length !== 2) {
  $ERROR('#3: function f_arg(x,y) {return arguments;} f_arg.length === 2. Actual: ' + (f_arg.length));
}

