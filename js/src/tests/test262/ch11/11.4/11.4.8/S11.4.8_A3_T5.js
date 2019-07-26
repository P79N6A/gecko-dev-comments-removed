










if (~({}) !== -1) {
  $ERROR('#1: ~({}) === -1. Actual: ' + (~({})));
}


if (~(function(){return 1}) !== -1) {
  $ERROR('#2: ~(function(){return 1}) === -1. Actual: ' + (~(function(){return 1})));
}

