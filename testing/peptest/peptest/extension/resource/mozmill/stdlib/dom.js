




































var EXPORTED_SYMBOLS = ['getAttributes'];


var getAttributes = function (node) {
  var attributes = {};
  for (i in node.attributes) {
    if ( !isNaN(i) ) {
      try {
        var attr = node.attributes[i];
        attributes[attr.name] = attr.value;
      } catch (err) {
      }
    }
  }
  return attributes;
}

