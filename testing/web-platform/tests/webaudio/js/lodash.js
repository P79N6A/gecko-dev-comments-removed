






;(function(window, undefined) {

  
  var freeExports = typeof exports == 'object' && exports;

  
  var freeGlobal = typeof global == 'object' && global;
  if (freeGlobal.global === freeGlobal) {
    window = freeGlobal;
  }

  
  var arrayRef = [],
      
      objectRef = new function(){};

  
  var idCounter = 0;

  
  var indicatorObject = objectRef;

  
  var largeArraySize = 30;

  
  var oldDash = window._;

  
  var reComplexDelimiter = /[-?+=!~*%&^<>|{(\/]|\[\D|\b(?:delete|in|instanceof|new|typeof|void)\b/;

  
  var reEscapedHtml = /&(?:amp|lt|gt|quot|#x27);/g;

  
  var reEmptyStringLeading = /\b__p \+= '';/g,
      reEmptyStringMiddle = /\b(__p \+=) '' \+/g,
      reEmptyStringTrailing = /(__e\(.*?\)|\b__t\)) \+\n'';/g;

  
  var reFlags = /\w*$/;

  
  var reInsertVariable = /(?:__e|__t = )\(\s*(?![\d\s"']|this\.)/g;

  
  var reNative = RegExp('^' +
    (objectRef.valueOf + '')
      .replace(/[.*+?^=!:${}()|[\]\/\\]/g, '\\$&')
      .replace(/valueOf|for [^\]]+/g, '.+?') + '$'
  );

  



  var reEsTemplate = /\$\{((?:(?=\\?)\\?[\s\S])*?)}/g;

  
  var reInterpolate = /<%=([\s\S]+?)%>/g;

  
  var reNoMatch = /($^)/;

  
  var reUnescapedHtml = /[&<>"']/g;

  
  var reUnescapedString = /['\n\r\t\u2028\u2029\\]/g;

  
  var shadowed = [
    'constructor', 'hasOwnProperty', 'isPrototypeOf', 'propertyIsEnumerable',
    'toLocaleString', 'toString', 'valueOf'
  ];

  
  var templateCounter = 0;

  
  var ceil = Math.ceil,
      concat = arrayRef.concat,
      floor = Math.floor,
      getPrototypeOf = reNative.test(getPrototypeOf = Object.getPrototypeOf) && getPrototypeOf,
      hasOwnProperty = objectRef.hasOwnProperty,
      push = arrayRef.push,
      propertyIsEnumerable = objectRef.propertyIsEnumerable,
      slice = arrayRef.slice,
      toString = objectRef.toString;

  
  var nativeBind = reNative.test(nativeBind = slice.bind) && nativeBind,
      nativeIsArray = reNative.test(nativeIsArray = Array.isArray) && nativeIsArray,
      nativeIsFinite = window.isFinite,
      nativeIsNaN = window.isNaN,
      nativeKeys = reNative.test(nativeKeys = Object.keys) && nativeKeys,
      nativeMax = Math.max,
      nativeMin = Math.min,
      nativeRandom = Math.random;

  
  var argsClass = '[object Arguments]',
      arrayClass = '[object Array]',
      boolClass = '[object Boolean]',
      dateClass = '[object Date]',
      funcClass = '[object Function]',
      numberClass = '[object Number]',
      objectClass = '[object Object]',
      regexpClass = '[object RegExp]',
      stringClass = '[object String]';

  





  var hasDontEnumBug;

  
  var iteratesOwnLast;

  









  var hasObjectSpliceBug = (hasObjectSpliceBug = { '0': 1, 'length': 1 },
    arrayRef.splice.call(hasObjectSpliceBug, 0, 1), hasObjectSpliceBug[0]);

  
  var noArgsEnum = true;

  (function() {
    var props = [];
    function ctor() { this.x = 1; }
    ctor.prototype = { 'valueOf': 1, 'y': 1 };
    for (var prop in new ctor) { props.push(prop); }
    for (prop in arguments) { noArgsEnum = !prop; }

    hasDontEnumBug = !/valueOf/.test(props);
    iteratesOwnLast = props[0] != 'x';
  }(1));

  
  var noArgsClass = !isArguments(arguments);

  
  var noArraySliceOnStrings = slice.call('x')[0] != 'x';

  





  var noCharByIndex = ('x'[0] + Object('x')[0]) != 'xx';

  




  try {
    var noNodeClass = ({ 'toString': 0 } + '', toString.call(window.document || 0) == objectClass);
  } catch(e) { }

  
  var isBindFast = nativeBind && /\n|Opera/.test(nativeBind + toString.call(window.opera));

  
  var isKeysFast = nativeKeys && /^.+$|true/.test(nativeKeys + !!window.attachEvent);

  












  try {
    var useSourceURL = (Function('//@')(), !window.attachEvent);
  } catch(e) { }

  
  var cloneableClasses = {};
  cloneableClasses[argsClass] = cloneableClasses[funcClass] = false;
  cloneableClasses[arrayClass] = cloneableClasses[boolClass] = cloneableClasses[dateClass] =
  cloneableClasses[numberClass] = cloneableClasses[objectClass] = cloneableClasses[regexpClass] =
  cloneableClasses[stringClass] = true;

  
  var objectTypes = {
    'boolean': false,
    'function': true,
    'object': true,
    'number': false,
    'string': false,
    'undefined': false
  };

  
  var stringEscapes = {
    '\\': '\\',
    "'": "'",
    '\n': 'n',
    '\r': 'r',
    '\t': 't',
    '\u2028': 'u2028',
    '\u2029': 'u2029'
  };

  

  








  function lodash(value) {
    
    if (value && value.__wrapped__) {
      return value;
    }
    
    if (!(this instanceof lodash)) {
      return new lodash(value);
    }
    this.__wrapped__ = value;
  }

  








  lodash.templateSettings = {

    






    'escape': /<%-([\s\S]+?)%>/g,

    






    'evaluate': /<%([\s\S]+?)%>/g,

    






    'interpolate': reInterpolate,

    






    'variable': ''
  };

  

  






  var iteratorTemplate = template(
    
    '<% if (obj.useStrict) { %>\'use strict\';\n<% } %>' +

    
    'var index, value, iteratee = <%= firstArg %>, ' +
    
    'result = <%= firstArg %>;\n' +
    
    'if (!<%= firstArg %>) return result;\n' +
    
    '<%= top %>;\n' +

    
    '<% if (arrayLoop) { %>' +
    'var length = iteratee.length; index = -1;\n' +
    'if (typeof length == \'number\') {' +

    
    '  <% if (noCharByIndex) { %>\n' +
    '  if (isString(iteratee)) {\n' +
    '    iteratee = iteratee.split(\'\')\n' +
    '  }' +
    '  <% } %>\n' +

    
    '  while (++index < length) {\n' +
    '    value = iteratee[index];\n' +
    '    <%= arrayLoop %>\n' +
    '  }\n' +
    '}\n' +
    'else {' +

    
    
    '  <%  } else if (noArgsEnum) { %>\n' +
    '  var length = iteratee.length; index = -1;\n' +
    '  if (length && isArguments(iteratee)) {\n' +
    '    while (++index < length) {\n' +
    '      value = iteratee[index += \'\'];\n' +
    '      <%= objectLoop %>\n' +
    '    }\n' +
    '  } else {' +
    '  <% } %>' +

    
    
    
    
    
    
    '  <% if (!hasDontEnumBug) { %>\n' +
    '  var skipProto = typeof iteratee == \'function\' && \n' +
    '    propertyIsEnumerable.call(iteratee, \'prototype\');\n' +
    '  <% } %>' +

    
    '  <% if (isKeysFast && useHas) { %>\n' +
    '  var ownIndex = -1,\n' +
    '      ownProps = objectTypes[typeof iteratee] ? nativeKeys(iteratee) : [],\n' +
    '      length = ownProps.length;\n\n' +
    '  while (++ownIndex < length) {\n' +
    '    index = ownProps[ownIndex];\n' +
    '    <% if (!hasDontEnumBug) { %>if (!(skipProto && index == \'prototype\')) {\n  <% } %>' +
    '    value = iteratee[index];\n' +
    '    <%= objectLoop %>\n' +
    '    <% if (!hasDontEnumBug) { %>}\n<% } %>' +
    '  }' +

    
    '  <% } else { %>\n' +
    '  for (index in iteratee) {<%' +
    '    if (!hasDontEnumBug || useHas) { %>\n    if (<%' +
    '      if (!hasDontEnumBug) { %>!(skipProto && index == \'prototype\')<% }' +
    '      if (!hasDontEnumBug && useHas) { %> && <% }' +
    '      if (useHas) { %>hasOwnProperty.call(iteratee, index)<% }' +
    '    %>) {' +
    '    <% } %>\n' +
    '    value = iteratee[index];\n' +
    '    <%= objectLoop %>;' +
    '    <% if (!hasDontEnumBug || useHas) { %>\n    }<% } %>\n' +
    '  }' +
    '  <% } %>' +

    
    
    
    
    '  <% if (hasDontEnumBug) { %>\n\n' +
    '  var ctor = iteratee.constructor;\n' +
    '    <% for (var k = 0; k < 7; k++) { %>\n' +
    '  index = \'<%= shadowed[k] %>\';\n' +
    '  if (<%' +
    '      if (shadowed[k] == \'constructor\') {' +
    '        %>!(ctor && ctor.prototype === iteratee) && <%' +
    '      } %>hasOwnProperty.call(iteratee, index)) {\n' +
    '    value = iteratee[index];\n' +
    '    <%= objectLoop %>\n' +
    '  }' +
    '    <% } %>' +
    '  <% } %>' +
    '  <% if (arrayLoop || noArgsEnum) { %>\n}<% } %>\n' +

    
    '<%= bottom %>;\n' +
    
    'return result'
  );

  
  var assignIteratorOptions = {
    'args': 'object, source, guard',
    'top':
      'for (var argsIndex = 1, argsLength = typeof guard == \'number\' ? 2 : arguments.length; argsIndex < argsLength; argsIndex++) {\n' +
      '  if ((iteratee = arguments[argsIndex])) {',
    'objectLoop': 'result[index] = value',
    'bottom': '  }\n}'
  };

  


  var forEachIteratorOptions = {
    'args': 'collection, callback, thisArg',
    'top': 'callback = createCallback(callback, thisArg)',
    'arrayLoop': 'if (callback(value, index, collection) === false) return result',
    'objectLoop': 'if (callback(value, index, collection) === false) return result'
  };

  
  var forOwnIteratorOptions = {
    'arrayLoop': null
  };

  

  










  function cachedContains(array, fromIndex, largeSize) {
    fromIndex || (fromIndex = 0);

    var length = array.length,
        isLarge = (length - fromIndex) >= (largeSize || largeArraySize);

    if (isLarge) {
      var cache = {},
          index = fromIndex - 1;

      while (++index < length) {
        
        
        var key = array[index] + '';
        (hasOwnProperty.call(cache, key) ? cache[key] : (cache[key] = [])).push(array[index]);
      }
    }
    return function(value) {
      if (isLarge) {
        var key = value + '';
        return hasOwnProperty.call(cache, key) && indexOf(cache[key], value) > -1;
      }
      return indexOf(array, value, fromIndex) > -1;
    }
  }

  







  function charAtCallback(value) {
    return value.charCodeAt(0);
  }

  








  function compareAscending(a, b) {
    var ai = a.index,
        bi = b.index;

    a = a.criteria;
    b = b.criteria;

    
    
    if (a !== b) {
      if (a > b || a === undefined) {
        return 1;
      }
      if (a < b || b === undefined) {
        return -1;
      }
    }
    return ai < bi ? -1 : 1;
  }

  










  function createBound(func, thisArg, partialArgs) {
    var isFunc = isFunction(func),
        isPartial = !partialArgs,
        key = thisArg;

    
    if (isPartial) {
      partialArgs = thisArg;
    }
    if (!isFunc) {
      thisArg = func;
    }

    function bound() {
      
      
      var args = arguments,
          thisBinding = isPartial ? this : thisArg;

      if (!isFunc) {
        func = thisArg[key];
      }
      if (partialArgs.length) {
        args = args.length
          ? partialArgs.concat(slice.call(args))
          : partialArgs;
      }
      if (this instanceof bound) {
        
        noop.prototype = func.prototype;
        thisBinding = new noop;

        
        
        var result = func.apply(thisBinding, args);
        return isObject(result)
          ? result
          : thisBinding
      }
      return func.apply(thisBinding, args);
    }
    return bound;
  }

  









  function createCallback(func, thisArg) {
    if (!func) {
      return identity;
    }
    if (typeof func != 'function') {
      return function(object) {
        return object[func];
      };
    }
    if (thisArg !== undefined) {
      return function(value, index, object) {
        return func.call(thisArg, value, index, object);
      };
    }
    return func;
  }

  













  function createIterator() {
    var data = {
      'arrayLoop': '',
      'bottom': '',
      'hasDontEnumBug': hasDontEnumBug,
      'isKeysFast': isKeysFast,
      'objectLoop': '',
      'noArgsEnum': noArgsEnum,
      'noCharByIndex': noCharByIndex,
      'shadowed': shadowed,
      'top': '',
      'useHas': true
    };

    
    for (var object, index = 0; object = arguments[index]; index++) {
      for (var key in object) {
        data[key] = object[key];
      }
    }
    var args = data.args;
    data.firstArg = /^[^,]+/.exec(args)[0];

    
    var factory = Function(
        'createCallback, hasOwnProperty, isArguments, isString, objectTypes, ' +
        'nativeKeys, propertyIsEnumerable',
      'return function(' + args + ') {\n' + iteratorTemplate(data) + '\n}'
    );
    
    return factory(
      createCallback, hasOwnProperty, isArguments, isString, objectTypes,
      nativeKeys, propertyIsEnumerable
    );
  }

  







  function escapeStringChar(match) {
    return '\\' + stringEscapes[match];
  }

  






  function escapeHtmlChar(match) {
    return htmlEscapes[match];
  }

  




  function noop() {
    
  }

  






  function unescapeHtmlChar(match) {
    return htmlUnescapes[match];
  }

  

  
















  var assign = createIterator(assignIteratorOptions);

  















  function isArguments(value) {
    return toString.call(value) == argsClass;
  }
  
  if (noArgsClass) {
    isArguments = function(value) {
      return value ? hasOwnProperty.call(value, 'callee') : false;
    };
  }

  



























  var forIn = createIterator(forEachIteratorOptions, forOwnIteratorOptions, {
    'useHas': false
  });

  



















  var forOwn = createIterator(forEachIteratorOptions, forOwnIteratorOptions);

  









  function shimIsPlainObject(value) {
    
    var result = false;
    if (!(value && typeof value == 'object') || isArguments(value)) {
      return result;
    }
    
    
    
    var ctor = value.constructor;
    if ((!noNodeClass || !(typeof value.toString != 'function' && typeof (value + '') == 'string')) &&
        (!isFunction(ctor) || ctor instanceof ctor)) {
      
      
      
      if (iteratesOwnLast) {
        forIn(value, function(value, key, object) {
          result = !hasOwnProperty.call(object, key);
          return false;
        });
        return result === false;
      }
      
      
      
      forIn(value, function(value, key) {
        result = key;
      });
      return result === false || hasOwnProperty.call(value, result);
    }
    return result;
  }

  







  function shimKeys(object) {
    var result = [];
    forOwn(object, function(value, key) {
      result.push(key);
    });
    return result;
  }

  







  var htmlEscapes = {
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#x27;'
  };

  
  var htmlUnescapes = invert(htmlEscapes);

  

  



































  function clone(value, deep, guard, stackA, stackB) {
    if (value == null) {
      return value;
    }
    if (guard) {
      deep = false;
    }
    
    var isObj = isObject(value);
    if (isObj) {
      
      var className = toString.call(value);
      if (!cloneableClasses[className] || (noArgsClass && isArguments(value))) {
        return value;
      }
      var isArr = className == arrayClass;
      isObj = isArr || (className == objectClass ? isPlainObject(value) : isObj);
    }
    
    if (!isObj || !deep) {
      
      return isObj
        ? (isArr ? slice.call(value) : assign({}, value))
        : value;
    }

    var ctor = value.constructor;
    switch (className) {
      case boolClass:
      case dateClass:
        return new ctor(+value);

      case numberClass:
      case stringClass:
        return new ctor(value);

      case regexpClass:
        return ctor(value.source, reFlags.exec(value));
    }
    
    stackA || (stackA = []);
    stackB || (stackB = []);

    var length = stackA.length;
    while (length--) {
      if (stackA[length] == value) {
        return stackB[length];
      }
    }
    
    var result = isArr ? ctor(value.length) : {};

    
    
    stackA.push(value);
    stackB.push(result);

    
    (isArr ? forEach : forOwn)(value, function(objValue, key) {
      result[key] = clone(objValue, deep, null, stackA, stackB);
    });

    return result;
  }

  

















  var defaults = createIterator(assignIteratorOptions, {
    'objectLoop': 'if (result[index] == null) ' + assignIteratorOptions.objectLoop
  });

  














  function functions(object) {
    var result = [];
    forIn(object, function(value, key) {
      if (isFunction(value)) {
        result.push(key);
      }
    });
    return result.sort();
  }

  














  function has(object, property) {
    return object ? hasOwnProperty.call(object, property) : false;
  }

  












  function invert(object) {
    var result = {};
    forOwn(object, function(value, key) {
      result[value] = key;
    });
    return result;
  }

  















  var isArray = nativeIsArray || function(value) {
    return toString.call(value) == arrayClass;
  };

  












  function isBoolean(value) {
    return value === true || value === false || toString.call(value) == boolClass;
  }

  












  function isDate(value) {
    return toString.call(value) == dateClass;
  }

  












  function isElement(value) {
    return value ? value.nodeType === 1 : false;
  }

  




















  function isEmpty(value) {
    var result = true;
    if (!value) {
      return result;
    }
    var className = toString.call(value),
        length = value.length;

    if ((className == arrayClass || className == stringClass ||
        className == argsClass || (noArgsClass && isArguments(value))) ||
        (className == objectClass && typeof length == 'number' && isFunction(value.splice))) {
      return !length;
    }
    forOwn(value, function() {
      return (result = false);
    });
    return result;
  }

  






















  function isEqual(a, b, stackA, stackB) {
    
    if (a === b) {
      
      return a !== 0 || (1 / a == 1 / b);
    }
    
    if (a == null || b == null) {
      return a === b;
    }
    
    var className = toString.call(a);
    if (className != toString.call(b)) {
      return false;
    }
    switch (className) {
      case boolClass:
      case dateClass:
        
        
        return +a == +b;

      case numberClass:
        
        return a != +a
          ? b != +b
          
          : (a == 0 ? (1 / a == 1 / b) : a == +b);

      case regexpClass:
      case stringClass:
        
        
        return a == b + '';
    }
    
    var isArr = className == arrayClass || className == argsClass;
    if (noArgsClass && !isArr && (isArr = isArguments(a)) && !isArguments(b)) {
      return false;
    }
    if (!isArr) {
      
      if (a.__wrapped__ || b.__wrapped__) {
        return isEqual(a.__wrapped__ || a, b.__wrapped__ || b);
      }
      
      if (className != objectClass || (noNodeClass && (
          (typeof a.toString != 'function' && typeof (a + '') == 'string') ||
          (typeof b.toString != 'function' && typeof (b + '') == 'string')))) {
        return false;
      }
      var ctorA = a.constructor,
          ctorB = b.constructor;

      
      if (ctorA != ctorB && !(
            isFunction(ctorA) && ctorA instanceof ctorA &&
            isFunction(ctorB) && ctorB instanceof ctorB
          )) {
        return false;
      }
    }
    
    
    
    stackA || (stackA = []);
    stackB || (stackB = []);

    var length = stackA.length;
    while (length--) {
      if (stackA[length] == a) {
        return stackB[length] == b;
      }
    }

    var index = -1,
        result = true,
        size = 0;

    
    stackA.push(a);
    stackB.push(b);

    
    if (isArr) {
      
      size = a.length;
      result = size == b.length;

      if (result) {
        
        while (size--) {
          if (!(result = isEqual(a[size], b[size], stackA, stackB))) {
            break;
          }
        }
      }
      return result;
    }
    
    for (var key in a) {
      if (hasOwnProperty.call(a, key)) {
        
        size++;
        
        if (!(hasOwnProperty.call(b, key) && isEqual(a[key], b[key], stackA, stackB))) {
          return false;
        }
      }
    }
    
    for (key in b) {
      
      
      
      if (hasOwnProperty.call(b, key) && !(size--)) {
        
        return false;
      }
    }
    
    if (hasDontEnumBug) {
      while (++index < 7) {
        key = shadowed[index];
        if (hasOwnProperty.call(a, key) &&
            !(hasOwnProperty.call(b, key) && isEqual(a[key], b[key], stackA, stackB))) {
          return false;
        }
      }
    }
    return true;
  }

  




























  function isFinite(value) {
    return nativeIsFinite(value) && !nativeIsNaN(parseFloat(value));
  }

  












  function isFunction(value) {
    return typeof value == 'function';
  }
  
  if (isFunction(/x/)) {
    isFunction = function(value) {
      return toString.call(value) == funcClass;
    };
  }

  



















  function isObject(value) {
    
    
    
    
    return value ? objectTypes[typeof value] : false;
  }

  

























  function isNaN(value) {
    
    
    return toString.call(value) == numberClass && value != +value
  }

  
















  function isNull(value) {
    return value === null;
  }

  












  function isNumber(value) {
    return toString.call(value) == numberClass;
  }

  























  var isPlainObject = !getPrototypeOf ? shimIsPlainObject : function(value) {
    if (!(value && typeof value == 'object')) {
      return false;
    }
    var valueOf = value.valueOf,
        objProto = typeof valueOf == 'function' && (objProto = getPrototypeOf(valueOf)) && getPrototypeOf(objProto);

    return objProto
      ? value == objProto || (getPrototypeOf(value) == objProto && !isArguments(value))
      : shimIsPlainObject(value);
  };

  












  function isRegExp(value) {
    return toString.call(value) == regexpClass;
  }

  












  function isString(value) {
    return toString.call(value) == stringClass;
  }

  













  function isUndefined(value) {
    return value === undefined;
  }

  












  var keys = !nativeKeys ? shimKeys : function(object) {
    
    return typeof object == 'function' && propertyIsEnumerable.call(object, 'prototype')
      ? shimKeys(object)
      : (isObject(object) ? nativeKeys(object) : []);
  };

  






























  function merge(object, source, indicator) {
    var args = arguments,
        index = 0,
        length = 2,
        stackA = args[3],
        stackB = args[4];

    if (indicator !== indicatorObject) {
      stackA = [];
      stackB = [];

      
      if (typeof indicator != 'number') {
        length = args.length;
      }
    }
    while (++index < length) {
      forOwn(args[index], function(source, key) {
        var found, isArr, value;
        if (source && ((isArr = isArray(source)) || isPlainObject(source))) {
          
          var stackLength = stackA.length;
          while (stackLength--) {
            found = stackA[stackLength] == source;
            if (found) {
              break;
            }
          }
          if (found) {
            object[key] = stackB[stackLength];
          }
          else {
            
            stackA.push(source);
            stackB.push(value = (value = object[key], isArr)
              ? (isArray(value) ? value : [])
              : (isPlainObject(value) ? value : {})
            );
            
            object[key] = merge(value, source, indicatorObject, stackA, stackB);
          }
        } else if (source != null) {
          object[key] = source;
        }
      });
    }
    return object;
  }

  
























  function omit(object, callback, thisArg) {
    var isFunc = typeof callback == 'function',
        result = {};

    if (isFunc) {
      callback = createCallback(callback, thisArg);
    } else {
      var props = concat.apply(arrayRef, arguments);
    }
    forIn(object, function(value, key, object) {
      if (isFunc
            ? !callback(value, key, object)
            : indexOf(props, key, 1) < 0
          ) {
        result[key] = value;
      }
    });
    return result;
  }

  













  function pairs(object) {
    var result = [];
    forOwn(object, function(value, key) {
      result.push([key, value]);
    });
    return result;
  }

  
























  function pick(object, callback, thisArg) {
    var result = {};
    if (typeof callback != 'function') {
      var index = 0,
          props = concat.apply(arrayRef, arguments),
          length = props.length;

      while (++index < length) {
        var key = props[index];
        if (key in object) {
          result[key] = object[key];
        }
      }
    } else {
      callback = createCallback(callback, thisArg);
      forIn(object, function(value, key, object) {
        if (callback(value, key, object)) {
          result[key] = value;
        }
      });
    }
    return result;
  }

  












  function values(object) {
    var result = [];
    forOwn(object, function(value) {
      result.push(value);
    });
    return result;
  }

  

  


























  function contains(collection, target, fromIndex) {
    var index = -1,
        length = collection ? collection.length : 0,
        result = false;

    fromIndex = (fromIndex < 0 ? nativeMax(0, length + fromIndex) : fromIndex) || 0;
    if (typeof length == 'number') {
      result = (isString(collection)
        ? collection.indexOf(target, fromIndex)
        : indexOf(collection, target, fromIndex)
      ) > -1;
    } else {
      forEach(collection, function(value) {
        if (++index >= fromIndex) {
          return !(result = value === target);
        }
      });
    }
    return result;
  }

  

























  function countBy(collection, callback, thisArg) {
    var result = {};
    callback = createCallback(callback, thisArg);
    forEach(collection, function(value, key, collection) {
      key = callback(value, key, collection);
      (hasOwnProperty.call(result, key) ? result[key]++ : result[key] = 1);
    });
    return result;
  }

  


















  function every(collection, callback, thisArg) {
    var result = true;
    callback = createCallback(callback, thisArg);

    if (isArray(collection)) {
      var index = -1,
          length = collection.length;

      while (++index < length) {
        if (!(result = !!callback(collection[index], index, collection))) {
          break;
        }
      }
    } else {
      forEach(collection, function(value, index, collection) {
        return (result = !!callback(value, index, collection));
      });
    }
    return result;
  }

  

















  function filter(collection, callback, thisArg) {
    var result = [];
    callback = createCallback(callback, thisArg);

    if (isArray(collection)) {
      var index = -1,
          length = collection.length;

      while (++index < length) {
        var value = collection[index];
        if (callback(value, index, collection)) {
          result.push(value);
        }
      }
    } else {
      forEach(collection, function(value, index, collection) {
        if (callback(value, index, collection)) {
          result.push(value);
        }
      });
    }
    return result;
  }

  



















  function find(collection, callback, thisArg) {
    var result;
    callback = createCallback(callback, thisArg);
    forEach(collection, function(value, index, collection) {
      if (callback(value, index, collection)) {
        result = value;
        return false;
      }
    });
    return result;
  }

  





















  var forEach = createIterator(forEachIteratorOptions);

  

























  function groupBy(collection, callback, thisArg) {
    var result = {};
    callback = createCallback(callback, thisArg);
    forEach(collection, function(value, key, collection) {
      key = callback(value, key, collection);
      (hasOwnProperty.call(result, key) ? result[key] : result[key] = []).push(value);
    });
    return result;
  }

  





















  function invoke(collection, methodName) {
    var args = slice.call(arguments, 2),
        isFunc = typeof methodName == 'function',
        result = [];

    forEach(collection, function(value) {
      result.push((isFunc ? methodName : value[methodName]).apply(value, args));
    });
    return result;
  }

  




















  function map(collection, callback, thisArg) {
    var index = -1,
        length = collection ? collection.length : 0,
        result = Array(typeof length == 'number' ? length : 0);

    callback = createCallback(callback, thisArg);
    if (isArray(collection)) {
      while (++index < length) {
        result[index] = callback(collection[index], index, collection);
      }
    } else {
      forEach(collection, function(value, key, collection) {
        result[++index] = callback(value, key, collection);
      });
    }
    return result;
  }

  























  function max(collection, callback, thisArg) {
    var computed = -Infinity,
        index = -1,
        length = collection ? collection.length : 0,
        result = computed;

    if (callback || !isArray(collection)) {
      callback = !callback && isString(collection)
        ? charAtCallback
        : createCallback(callback, thisArg);

      forEach(collection, function(value, index, collection) {
        var current = callback(value, index, collection);
        if (current > computed) {
          computed = current;
          result = value;
        }
      });
    } else {
      while (++index < length) {
        if (collection[index] > result) {
          result = collection[index];
        }
      }
    }
    return result;
  }

  

















  function min(collection, callback, thisArg) {
    var computed = Infinity,
        index = -1,
        length = collection ? collection.length : 0,
        result = computed;

    if (callback || !isArray(collection)) {
      callback = !callback && isString(collection)
        ? charAtCallback
        : createCallback(callback, thisArg);

      forEach(collection, function(value, index, collection) {
        var current = callback(value, index, collection);
        if (current < computed) {
          computed = current;
          result = value;
        }
      });
    } else {
      while (++index < length) {
        if (collection[index] < result) {
          result = collection[index];
        }
      }
    }
    return result;
  }

  




















  function pluck(collection, property) {
    var result = [];
    forEach(collection, function(value) {
      result.push(value[property]);
    });
    return result;
  }

  



















  function reduce(collection, callback, accumulator, thisArg) {
    var noaccum = arguments.length < 3;
    callback = createCallback(callback, thisArg);
    forEach(collection, function(value, index, collection) {
      accumulator = noaccum
        ? (noaccum = false, value)
        : callback(accumulator, value, index, collection)
    });
    return accumulator;
  }

  

















  function reduceRight(collection, callback, accumulator, thisArg) {
    var iteratee = collection,
        length = collection ? collection.length : 0,
        noaccum = arguments.length < 3;

    if (typeof length != 'number') {
      var props = keys(collection);
      length = props.length;
    } else if (noCharByIndex && isString(collection)) {
      iteratee = collection.split('');
    }
    forEach(collection, function(value, index, collection) {
      index = props ? props[--length] : --length;
      accumulator = noaccum
        ? (noaccum = false, iteratee[index])
        : callback.call(thisArg, accumulator, iteratee[index], index, collection);
    });
    return accumulator;
  }

  
















  function reject(collection, callback, thisArg) {
    callback = createCallback(callback, thisArg);
    return filter(collection, function(value, index, collection) {
      return !callback(value, index, collection);
    });
  }

  













  function shuffle(collection) {
    var index = -1,
        result = Array(collection ? collection.length : 0);

    forEach(collection, function(value) {
      var rand = floor(nativeRandom() * (++index + 1));
      result[index] = result[rand];
      result[rand] = value;
    });
    return result;
  }

  



















  function size(collection) {
    var length = collection ? collection.length : 0;
    return typeof length == 'number' ? length : keys(collection).length;
  }

  



















  function some(collection, callback, thisArg) {
    var result;
    callback = createCallback(callback, thisArg);

    if (isArray(collection)) {
      var index = -1,
          length = collection.length;

      while (++index < length) {
        if ((result = callback(collection[index], index, collection))) {
          break;
        }
      }
    } else {
      forEach(collection, function(value, index, collection) {
        return !(result = callback(value, index, collection));
      });
    }
    return !!result;
  }

  
























  function sortBy(collection, callback, thisArg) {
    var result = [];
    callback = createCallback(callback, thisArg);
    forEach(collection, function(value, index, collection) {
      result.push({
        'criteria': callback(value, index, collection),
        'index': index,
        'value': value
      });
    });

    var length = result.length;
    result.sort(compareAscending);
    while (length--) {
      result[length] = result[length].value;
    }
    return result;
  }

  












  function toArray(collection) {
    if (collection && typeof collection.length == 'number') {
      return (noArraySliceOnStrings ? isString(collection) : typeof collection == 'string')
        ? collection.split('')
        : slice.call(collection);
    }
    return values(collection);
  }

  




















  function where(collection, properties) {
    var props = keys(properties);
    return filter(collection, function(object) {
      var length = props.length;
      while (length--) {
        var result = object[props[length]] === properties[props[length]];
        if (!result) {
          break;
        }
      }
      return !!result;
    });
  }

  

  













  function compact(array) {
    var index = -1,
        length = array ? array.length : 0,
        result = [];

    while (++index < length) {
      var value = array[index];
      if (value) {
        result.push(value);
      }
    }
    return result;
  }

  















  function difference(array) {
    var index = -1,
        length = array ? array.length : 0,
        flattened = concat.apply(arrayRef, arguments),
        contains = cachedContains(flattened, length),
        result = [];

    while (++index < length) {
      var value = array[index];
      if (!contains(value)) {
        result.push(value);
      }
    }
    return result;
  }

  


















  function first(array, n, guard) {
    if (array) {
      return (n == null || guard) ? array[0] : slice.call(array, 0, n);
    }
  }

  

















  function flatten(array, shallow) {
    var index = -1,
        length = array ? array.length : 0,
        result = [];

    while (++index < length) {
      var value = array[index];

      
      if (isArray(value)) {
        push.apply(result, shallow ? value : flatten(value));
      } else {
        result.push(value);
      }
    }
    return result;
  }

  























  function indexOf(array, value, fromIndex) {
    var index = -1,
        length = array ? array.length : 0;

    if (typeof fromIndex == 'number') {
      index = (fromIndex < 0 ? nativeMax(0, length + fromIndex) : fromIndex || 0) - 1;
    } else if (fromIndex) {
      index = sortedIndex(array, value);
      return array[index] === value ? index : -1;
    }
    while (++index < length) {
      if (array[index] === value) {
        return index;
      }
    }
    return -1;
  }

  
















  function initial(array, n, guard) {
    return array
      ? slice.call(array, 0, -((n == null || guard) ? 1 : n))
      : [];
  }

  














  function intersection(array) {
    var args = arguments,
        argsLength = args.length,
        cache = {},
        result = [];

    forEach(array, function(value) {
      if (indexOf(result, value) < 0) {
        var length = argsLength;
        while (--length) {
          if (!(cache[length] || (cache[length] = cachedContains(args[length])))(value)) {
            return;
          }
        }
        result.push(value);
      }
    });
    return result;
  }

  

















  function last(array, n, guard) {
    if (array) {
      var length = array.length;
      return (n == null || guard) ? array[length - 1] : slice.call(array, -n || length);
    }
  }

  



















  function lastIndexOf(array, value, fromIndex) {
    var index = array ? array.length : 0;
    if (typeof fromIndex == 'number') {
      index = (fromIndex < 0 ? nativeMax(0, index + fromIndex) : nativeMin(fromIndex, index - 1)) + 1;
    }
    while (index--) {
      if (array[index] === value) {
        return index;
      }
    }
    return -1;
  }

  
















  function object(keys, values) {
    var index = -1,
        length = keys ? keys.length : 0,
        result = {};

    while (++index < length) {
      var key = keys[index];
      if (values) {
        result[key] = values[index];
      } else {
        result[key[0]] = key[1];
      }
    }
    return result;
  }

  




























  function range(start, end, step) {
    start = +start || 0;
    step = +step || 1;

    if (end == null) {
      end = start;
      start = 0;
    }
    
    
    var index = -1,
        length = nativeMax(0, ceil((end - start) / step)),
        result = Array(length);

    while (++index < length) {
      result[index] = start;
      start += step;
    }
    return result;
  }

  

















  function rest(array, n, guard) {
    return array
      ? slice.call(array, (n == null || guard) ? 1 : n)
      : [];
  }

  







































  function sortedIndex(array, value, callback, thisArg) {
    var low = 0,
        high = array ? array.length : low;

    
    callback = callback ? createCallback(callback, thisArg) : identity;
    value = callback(value);
    while (low < high) {
      var mid = (low + high) >>> 1;
      callback(array[mid]) < value
        ? low = mid + 1
        : high = mid;
    }
    return low;
  }

  














  function union() {
    return uniq(concat.apply(arrayRef, arguments));
  }

  





























  function uniq(array, isSorted, callback, thisArg) {
    var index = -1,
        length = array ? array.length : 0,
        result = [],
        seen = result;

    
    if (typeof isSorted == 'function') {
      thisArg = callback;
      callback = isSorted;
      isSorted = false;
    }
    
    var isLarge = !isSorted && length > 74;
    if (isLarge) {
      var cache = {};
    }
    if (callback) {
      seen = [];
      callback = createCallback(callback, thisArg);
    }
    while (++index < length) {
      var value = array[index],
          computed = callback ? callback(value, index, array) : value;

      if (isLarge) {
        
        
        seen = hasOwnProperty.call(cache, computed + '') ? cache[computed] : (cache[computed] = []);
      }
      if (isSorted
            ? !index || seen[seen.length - 1] !== computed
            : indexOf(seen, computed) < 0
          ) {
        if (callback || isLarge) {
          seen.push(computed);
        }
        result.push(value);
      }
    }
    return result;
  }

  














  function without(array) {
    var index = -1,
        length = array ? array.length : 0,
        contains = cachedContains(arguments, 1, 20),
        result = [];

    while (++index < length) {
      var value = array[index];
      if (!contains(value)) {
        result.push(value);
      }
    }
    return result;
  }

  















  function zip(array) {
    var index = -1,
        length = array ? max(pluck(arguments, 'length')) : 0,
        result = Array(length);

    while (++index < length) {
      result[index] = pluck(arguments, index);
    }
    return result;
  }

  

  



















  function after(n, func) {
    if (n < 1) {
      return func();
    }
    return function() {
      if (--n < 1) {
        return func.apply(this, arguments);
      }
    };
  }

  





















  function bind(func, thisArg) {
    
    
    return isBindFast || (nativeBind && arguments.length > 2)
      ? nativeBind.call.apply(nativeBind, arguments)
      : createBound(func, thisArg, slice.call(arguments, 2));
  }

  





















  function bindAll(object) {
    var funcs = arguments,
        index = funcs.length > 1 ? 0 : (funcs = functions(object), -1),
        length = funcs.length;

    while (++index < length) {
      var key = funcs[index];
      object[key] = bind(object[key], object);
    }
    return object;
  }

  

































  function bindKey(object, key) {
    return createBound(object, key, slice.call(arguments, 2));
  }

  


















  function compose() {
    var funcs = arguments;
    return function() {
      var args = arguments,
          length = funcs.length;

      while (length--) {
        args = [funcs[length].apply(this, args)];
      }
      return args[0];
    };
  }

  



















  function debounce(func, wait, immediate) {
    var args,
        result,
        thisArg,
        timeoutId;

    function delayed() {
      timeoutId = null;
      if (!immediate) {
        result = func.apply(thisArg, args);
      }
    }
    return function() {
      var isImmediate = immediate && !timeoutId;
      args = arguments;
      thisArg = this;

      clearTimeout(timeoutId);
      timeoutId = setTimeout(delayed, wait);

      if (isImmediate) {
        result = func.apply(thisArg, args);
      }
      return result;
    };
  }

  
















  function delay(func, wait) {
    var args = slice.call(arguments, 2);
    return setTimeout(function() { func.apply(undefined, args); }, wait);
  }

  














  function defer(func) {
    var args = slice.call(arguments, 1);
    return setTimeout(function() { func.apply(undefined, args); }, 1);
  }

  


















  function memoize(func, resolver) {
    var cache = {};
    return function() {
      var key = resolver ? resolver.apply(this, arguments) : arguments[0];
      return hasOwnProperty.call(cache, key)
        ? cache[key]
        : (cache[key] = func.apply(this, arguments));
    };
  }

  
















  function once(func) {
    var result,
        ran = false;

    return function() {
      if (ran) {
        return result;
      }
      ran = true;
      result = func.apply(this, arguments);

      
      func = null;
      return result;
    };
  }

  

















  function partial(func) {
    return createBound(func, slice.call(arguments, 1));
  }

  

















  function throttle(func, wait) {
    var args,
        result,
        thisArg,
        timeoutId,
        lastCalled = 0;

    function trailingCall() {
      lastCalled = new Date;
      timeoutId = null;
      result = func.apply(thisArg, args);
    }
    return function() {
      var now = new Date,
          remaining = wait - (now - lastCalled);

      args = arguments;
      thisArg = this;

      if (remaining <= 0) {
        clearTimeout(timeoutId);
        lastCalled = now;
        result = func.apply(thisArg, args);
      }
      else if (!timeoutId) {
        timeoutId = setTimeout(trailingCall, remaining);
      }
      return result;
    };
  }

  




















  function wrap(value, wrapper) {
    return function() {
      var args = [value];
      push.apply(args, arguments);
      return wrapper.apply(this, args);
    };
  }

  

  













  function escape(string) {
    return string == null ? '' : (string + '').replace(reUnescapedHtml, escapeHtmlChar);
  }

  















  function identity(value) {
    return value;
  }

  





















  function mixin(object) {
    forEach(functions(object), function(methodName) {
      var func = lodash[methodName] = object[methodName];

      lodash.prototype[methodName] = function() {
        var args = [this.__wrapped__];
        push.apply(args, arguments);

        var result = func.apply(lodash, args);
        if (this.__chain__) {
          result = new lodash(result);
          result.__chain__ = true;
        }
        return result;
      };
    });
  }

  











  function noConflict() {
    window._ = oldDash;
    return this;
  }

  

















  function random(min, max) {
    if (min == null && max == null) {
      max = 1;
    }
    min = +min || 0;
    if (max == null) {
      max = min;
      min = 0;
    }
    return min + floor(nativeRandom() * ((+max || 0) - min + 1));
  }

  


























  function result(object, property) {
    
    
    var value = object ? object[property] : null;
    return isFunction(value) ? object[property]() : value;
  }

  













































































  function template(text, data, options) {
    
    
    
    
    text || (text = '');
    options || (options = {});

    var isEvaluating,
        result,
        settings = lodash.templateSettings,
        index = 0,
        interpolate = options.interpolate || settings.interpolate || reNoMatch,
        source = "__p += '",
        variable = options.variable || settings.variable,
        hasVariable = variable;

    
    var reDelimiters = RegExp(
      (options.escape || settings.escape || reNoMatch).source + '|' +
      interpolate.source + '|' +
      (interpolate === reInterpolate ? reEsTemplate : reNoMatch).source + '|' +
      (options.evaluate || settings.evaluate || reNoMatch).source + '|$'
    , 'g');

    text.replace(reDelimiters, function(match, escapeValue, interpolateValue, esTemplateValue, evaluateValue, offset) {
      interpolateValue || (interpolateValue = esTemplateValue);

      
      source += text.slice(index, offset).replace(reUnescapedString, escapeStringChar);

      
      source +=
        escapeValue ? "' +\n__e(" + escapeValue + ") +\n'" :
        evaluateValue ? "';\n" + evaluateValue + ";\n__p += '" :
        interpolateValue ? "' +\n((__t = (" + interpolateValue + ")) == null ? '' : __t) +\n'" : '';

      isEvaluating || (isEvaluating = evaluateValue || reComplexDelimiter.test(escapeValue || interpolateValue));
      index = offset + match.length;
    });

    source += "';\n";

    
    
    
    if (!hasVariable) {
      variable = 'obj';
      if (isEvaluating) {
        source = 'with (' + variable + ') {\n' + source + '\n}\n';
      }
      else {
        
        var reDoubleVariable = RegExp('(\\(\\s*)' + variable + '\\.' + variable + '\\b', 'g');
        source = source
          .replace(reInsertVariable, '$&' + variable + '.')
          .replace(reDoubleVariable, '$1__d');
      }
    }

    
    source = (isEvaluating ? source.replace(reEmptyStringLeading, '') : source)
      .replace(reEmptyStringMiddle, '$1')
      .replace(reEmptyStringTrailing, '$1;');

    
    source = 'function(' + variable + ') {\n' +
      (hasVariable ? '' : variable + ' || (' + variable + ' = {});\n') +
      'var __t, __p = \'\', __e = _.escape' +
      (isEvaluating
        ? ', __j = Array.prototype.join;\n' +
          'function print() { __p += __j.call(arguments, \'\') }\n'
        : (hasVariable ? '' : ', __d = ' + variable + '.' + variable + ' || ' + variable) + ';\n'
      ) +
      source +
      'return __p\n}';

    
    
    var sourceURL = useSourceURL
      ? '\n//@ sourceURL=' + (options.sourceURL || '/lodash/template/source[' + (templateCounter++) + ']')
      : '';

    try {
      result = Function('_', 'return ' + source + sourceURL)(lodash);
    } catch(e) {
      e.source = source;
      throw e;
    }

    if (data) {
      return result(data);
    }
    
    
    
    result.source = source;
    return result;
  }

  






















  function times(n, callback, thisArg) {
    n = +n || 0;
    var index = -1,
        result = Array(n);

    while (++index < n) {
      result[index] = callback.call(thisArg, index);
    }
    return result;
  }

  














  function unescape(string) {
    return string == null ? '' : (string + '').replace(reEscapedHtml, unescapeHtmlChar);
  }

  













  function uniqueId(prefix) {
    var id = idCounter++;
    return prefix ? prefix + id : id;
  }

  

  






















  function chain(value) {
    value = new lodash(value);
    value.__chain__ = true;
    return value;
  }

  




















  function tap(value, interceptor) {
    interceptor(value);
    return value;
  }

  












  function wrapperChain() {
    this.__chain__ = true;
    return this;
  }

  











  function wrapperValue() {
    return this.__wrapped__;
  }

  

  






  lodash.VERSION = '0.10.0';

  
  lodash.assign = assign;
  lodash.after = after;
  lodash.bind = bind;
  lodash.bindAll = bindAll;
  lodash.bindKey = bindKey;
  lodash.chain = chain;
  lodash.clone = clone;
  lodash.compact = compact;
  lodash.compose = compose;
  lodash.contains = contains;
  lodash.countBy = countBy;
  lodash.debounce = debounce;
  lodash.defaults = defaults;
  lodash.defer = defer;
  lodash.delay = delay;
  lodash.difference = difference;
  lodash.escape = escape;
  lodash.every = every;
  lodash.filter = filter;
  lodash.find = find;
  lodash.first = first;
  lodash.flatten = flatten;
  lodash.forEach = forEach;
  lodash.forIn = forIn;
  lodash.forOwn = forOwn;
  lodash.functions = functions;
  lodash.groupBy = groupBy;
  lodash.has = has;
  lodash.identity = identity;
  lodash.indexOf = indexOf;
  lodash.initial = initial;
  lodash.intersection = intersection;
  lodash.invert = invert;
  lodash.invoke = invoke;
  lodash.isArguments = isArguments;
  lodash.isArray = isArray;
  lodash.isBoolean = isBoolean;
  lodash.isDate = isDate;
  lodash.isElement = isElement;
  lodash.isEmpty = isEmpty;
  lodash.isEqual = isEqual;
  lodash.isFinite = isFinite;
  lodash.isFunction = isFunction;
  lodash.isNaN = isNaN;
  lodash.isNull = isNull;
  lodash.isNumber = isNumber;
  lodash.isObject = isObject;
  lodash.isPlainObject = isPlainObject;
  lodash.isRegExp = isRegExp;
  lodash.isString = isString;
  lodash.isUndefined = isUndefined;
  lodash.keys = keys;
  lodash.last = last;
  lodash.lastIndexOf = lastIndexOf;
  lodash.map = map;
  lodash.max = max;
  lodash.memoize = memoize;
  lodash.merge = merge;
  lodash.min = min;
  lodash.mixin = mixin;
  lodash.noConflict = noConflict;
  lodash.object = object;
  lodash.omit = omit;
  lodash.once = once;
  lodash.pairs = pairs;
  lodash.partial = partial;
  lodash.pick = pick;
  lodash.pluck = pluck;
  lodash.random = random;
  lodash.range = range;
  lodash.reduce = reduce;
  lodash.reduceRight = reduceRight;
  lodash.reject = reject;
  lodash.rest = rest;
  lodash.result = result;
  lodash.shuffle = shuffle;
  lodash.size = size;
  lodash.some = some;
  lodash.sortBy = sortBy;
  lodash.sortedIndex = sortedIndex;
  lodash.tap = tap;
  lodash.template = template;
  lodash.throttle = throttle;
  lodash.times = times;
  lodash.toArray = toArray;
  lodash.unescape = unescape;
  lodash.union = union;
  lodash.uniq = uniq;
  lodash.uniqueId = uniqueId;
  lodash.values = values;
  lodash.where = where;
  lodash.without = without;
  lodash.wrap = wrap;
  lodash.zip = zip;

  
  lodash.all = every;
  lodash.any = some;
  lodash.collect = map;
  lodash.detect = find;
  lodash.drop = rest;
  lodash.each = forEach;
  lodash.extend = assign;
  lodash.foldl = reduce;
  lodash.foldr = reduceRight;
  lodash.head = first;
  lodash.include = contains;
  lodash.inject = reduce;
  lodash.methods = functions;
  lodash.select = filter;
  lodash.tail = rest;
  lodash.take = first;
  lodash.unique = uniq;

  
  lodash._iteratorTemplate = iteratorTemplate;

  

  
  mixin(lodash);

  
  
  lodash.prototype.chain = wrapperChain;
  lodash.prototype.value = wrapperValue;

  
  forEach(['pop', 'push', 'reverse', 'shift', 'sort', 'splice', 'unshift'], function(methodName) {
    var func = arrayRef[methodName];

    lodash.prototype[methodName] = function() {
      var value = this.__wrapped__;
      func.apply(value, arguments);

      
      
      if (hasObjectSpliceBug && value.length === 0) {
        delete value[0];
      }
      if (this.__chain__) {
        value = new lodash(value);
        value.__chain__ = true;
      }
      return value;
    };
  });

  
  forEach(['concat', 'join', 'slice'], function(methodName) {
    var func = arrayRef[methodName];

    lodash.prototype[methodName] = function() {
      var value = this.__wrapped__,
          result = func.apply(value, arguments);

      if (this.__chain__) {
        result = new lodash(result);
        result.__chain__ = true;
      }
      return result;
    };
  });

  

  
  
  if (typeof define == 'function' && typeof define.amd == 'object' && define.amd) {
    
    
    
    
    window._ = lodash;

    
    
    define(function() {
      return lodash;
    });
  }
  
  else if (freeExports) {
    
    if (typeof module == 'object' && module && module.exports == freeExports) {
      (module.exports = lodash)._ = lodash;
    }
    
    else {
      freeExports._ = lodash;
    }
  }
  else {
    
    window._ = lodash;
  }
}(this));
