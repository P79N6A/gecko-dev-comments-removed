











try{
  null['foo'];
  $ERROR('#1.1: null[\'foo\'] throw TypeError. Actual: ' + (null['foo']));
}
catch(e){
  if((e instanceof TypeError) !== true){
    $ERROR('#1.2: null[\'foo\'] must throw TypeError. Actual: ' + (e));
  }
}


try{
  with(null) x = 2;
  $ERROR('#2.1: with(null) x = 2 must throw TypeError. Actual: x === . Actual: ' + (x));
}
catch(e){
  if((e instanceof TypeError) !== true){
    $ERROR('#2.2: with(null) x = 2 must throw TypeError. Actual: ' + (e));
  }
}

