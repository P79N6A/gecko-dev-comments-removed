










if (isNaN(-{}) !== true) {
  $ERROR('#1: -{} === Not-a-Number. Actual: ' + (-{}));
}


if (isNaN(-function(){return 1}) !== true) {
  $ERROR('#2: -function(){return 1} === Not-a-Number. Actual: ' + (-function(){return 1}));
}

