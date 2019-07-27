



var versionParse = require('./lib/utils').versionParse;

var COMPARATORS = ['>=', '<=', '>', '<', '=', '~', '^'];

exports.parse = function (input) {
  input = input || '';
  input = input.trim();
  if (!input)
    throw new Error('`parse` argument must be a populated string.');

  
  if (input === "*") {
    return { min: undefined, max: undefined };
  }

  var inputs = input.split(' ');
  var min;
  var max;

  
  if (inputs.length === 3 && inputs[1] === '-') {
    return { min: inputs[0], max: inputs[2] };
  }

  inputs.forEach(function (input) {
    var parsed = parseExpression(input);
    var version = parsed.version;
    var comparator = parsed.comparator;

    
    if (inputs.length === 1 && !comparator)
      min = max = version;

    
    if (~comparator.indexOf('>')) {
      if (~comparator.indexOf('='))
        min = version; 
      else
        min = increment(version); 
    }
    else if (~comparator.indexOf('<')) {
      if (~comparator.indexOf('='))
        max = version; 
      else
        max = decrement(version); 
    }
  });

  return {
    min: min,
    max : max
  };
};

function parseExpression (input) {
  for (var i = 0; i < COMPARATORS.length; i++)
    if (~input.indexOf(COMPARATORS[i]))
      return {
        comparator: COMPARATORS[i],
        version: input.substr(COMPARATORS[i].length)
      };
  return { version: input, comparator: '' };
}








function decrement (vString) {
  return vString + (vString.charAt(vString.length - 1) === '.' ? '' : '.') + '-1';
}
exports.decrement = decrement;












function increment (vString) {
  var match = versionParse(vString);
  var a = match[1];
  var b = match[2];
  var c = match[3];
  var d = match[4];
  var lastPos = vString.length - 1;
  var lastChar = vString.charAt(lastPos);

  if (!b) {
    return vString + (lastChar === '.' ? '' : '.') + '1';
  }
  if (!c) {
    return vString + '1';
  }
  if (!d) {
    return vString.substr(0, lastPos) + (++lastChar);
  }
  return vString.substr(0, lastPos) + String.fromCharCode(lastChar.charCodeAt(0) + 1);
}
exports.increment = increment;
