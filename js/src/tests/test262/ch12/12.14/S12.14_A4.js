











try {
  throw "catchme";
  $ERROR('#1.1: throw "catchme" lead to throwing exception');
}
catch (e) {
  if (delete e){
    $ERROR('#1.2: Exception has DontDelete property');
  }
  if (e!=="catchme") {
    $ERROR('#1.3: Exception === "catchme". Actual:  Exception ==='+ e  );
  }
}


try {
  throw "catchme";
  $ERROR('#2.1: throw "catchme" lead to throwing exception');
}
catch(e){}
try{
  e;
  $ERROR('#2.2: Deleting catching exception after ending "catch" block');
}
catch(err){}

