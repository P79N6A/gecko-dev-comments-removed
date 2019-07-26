










try {
  throw 1;
} catch(e) {
  if (!(e !== "1")) {
    $ERROR('#1: throw 1 !== "1"');
  }
}


try {
  throw "1";
} catch(e) {
  if (!(1 !== e)) {
    $ERROR('#2: 1 !== throw "1"');
  }
} 

