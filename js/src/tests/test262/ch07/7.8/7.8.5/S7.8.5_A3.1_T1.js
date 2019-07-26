










var regexp = /(?:)/g; 
if (regexp.global !== true) {
  $ERROR('#1: var regexp = /(?:)/g; regexp.global === true. Actual: ' + (regexp.global));
}


if (regexp.ignoreCase !== false) {
  $ERROR('#2: var regexp = /(?:)/g; regexp.ignoreCase === false. Actual: ' + (regexp.ignoreCase));
}


if (regexp.multiline !== false) {
  $ERROR('#3: var regexp = /(?:)/g; regexp.multiline === false. Actual: ' + (regexp.multiline));
}                            

