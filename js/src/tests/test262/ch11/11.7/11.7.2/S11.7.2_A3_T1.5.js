










if (({} >> function(){return 1}) !== 0) {
  $ERROR('#1: ({} >> function(){return 1}) === 0. Actual: ' + (({} >> function(){return 1})));
}


if ((function(){return 1} >> {}) !== 0) {
  $ERROR('#2: (function(){return 1} >> {}) === 0. Actual: ' + ((function(){return 1} >> {})));
}


if ((function(){return 1} >> function(){return 1}) !== 0) {
  $ERROR('#3: (function(){return 1} >> function(){return 1}) === 0. Actual: ' + ((function(){return 1} >> function(){return 1})));
}


if (({} >> {}) !== 0) {
  $ERROR('#4: ({} >> {}) === 0. Actual: ' + (({} >> {})));
}


