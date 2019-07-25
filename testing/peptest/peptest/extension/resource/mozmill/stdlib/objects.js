




































var EXPORTED_SYMBOLS = ['getLength', ];

var getLength = function (obj) {
  var len = 0;
  for (i in obj) {
    len++;
  }
  return len;
}








































