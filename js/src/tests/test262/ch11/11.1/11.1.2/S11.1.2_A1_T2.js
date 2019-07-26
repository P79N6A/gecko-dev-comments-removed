










try {
  this.z;
  z;
  $ERROR('#1.1: this.z; z === undefined throw ReferenceError. Actual: ' + (z));
} catch(e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: this.z; z === undefined throw ReferenceError. Actual: ' + (e));
  }
}

