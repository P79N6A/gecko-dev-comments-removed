










if (({} + function(){return 1}) !== ({}.toString() + function(){return 1}.toString())) {
  $ERROR('#1: ({} + function(){return 1}) === ({}.toString() + function(){return 1}.toString()). Actual: ' + (({} + function(){return 1})));
}


if ((function(){return 1} + {}) !== (function(){return 1}.toString() + {}.toString())) {
  $ERROR('#2: (function(){return 1} + {}) === (function(){return 1}.toString() + {}.toString()). Actual: ' + ((function(){return 1} + {})));
}


if ((function(){return 1} + function(){return 1}) !== (function(){return 1}.toString() + function(){return 1}.toString())) {
  $ERROR('#3: (function(){return 1} + function(){return 1}) === (function(){return 1}.toString() + function(){return 1}.toString()). Actual: ' + ((function(){return 1} + function(){return 1})));
}


if (({} + {}) !== ({}.toString() + {}.toString())) {
  $ERROR('#4: ({} + {}) === ({}.toString() + {}.toString()). Actual: ' + (({} + {})));
}



