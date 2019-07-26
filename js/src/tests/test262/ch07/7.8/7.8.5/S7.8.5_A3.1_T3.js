










var regexp = /(?:)/m; 
if (regexp.global !== false) {
  $ERROR('#1: var regexp = /(?:)/g; regexp.global === false. Actual: ' + (regexp.global));
}


if (regexp.ignoreCase !== false) {
  $ERROR('#2: var regexp = /(?:)/g; regexp.ignoreCase === false. Actual: ' + (regexp.ignoreCase));
}


if (regexp.multiline !== true) {
  $ERROR('#3: var regexp = /(?:)/g; regexp.multiline === true. Actual: ' + (regexp.multiline));
}                            

