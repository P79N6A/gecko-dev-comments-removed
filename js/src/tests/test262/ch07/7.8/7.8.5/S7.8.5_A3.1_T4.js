










var regexp = /(?:)/gi; 
if (regexp.global !== true) {
  $ERROR('#1: var regexp = /(?:)/g; regexp.global === true. Actual: ' + (regexp.global));
}


if (regexp.ignoreCase !== true) {
  $ERROR('#2: var regexp = /(?:)/g; regexp.ignoreCase === true. Actual: ' + (regexp.ignoreCase));
}


if (regexp.multiline !== false) {
  $ERROR('#3: var regexp = /(?:)/g; regexp.multiline === false. Actual: ' + (regexp.multiline));
}                            

