










if (isNaN({} / function(){return 1}) !== true) {
  $ERROR('#1: {} / function(){return 1} === Not-a-Number. Actual: ' + ({} / function(){return 1}));
}


if (isNaN(function(){return 1} / {}) !== true) {
  $ERROR('#2: function(){return 1} / {} === Not-a-Number. Actual: ' + (function(){return 1} / {}));
}


if (isNaN(function(){return 1} / function(){return 1}) !== true) {
  $ERROR('#3: function(){return 1} / function(){return 1} === Not-a-Number. Actual: ' + (function(){return 1} / function(){return 1}));
}


if (isNaN({} / {}) !== true) {
  $ERROR('#4: {} / {} === Not-a-Number. Actual: ' + ({} / {}));
}

