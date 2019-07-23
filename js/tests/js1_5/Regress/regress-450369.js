




































var gTestfile = 'regress-450369.js';

var BUGNUMBER = 450369;
var summary = 'Crash with JIT and json2.js';
var actual = 'No Crash';
var expect = 'No Crash';

jit(true);







































































if (!this.emulatedJSON) {

    emulatedJSON = function () {

        function f(n) {    
            return n < 10 ? '0' + n : n;
        }

        Date.prototype.toJSON = function () {



            return this.getUTCFullYear()   + '-' +
                 f(this.getUTCMonth() + 1) + '-' +
                 f(this.getUTCDate())      + 'T' +
                 f(this.getUTCHours())     + ':' +
                 f(this.getUTCMinutes())   + ':' +
                 f(this.getUTCSeconds())   + 'Z';
        };


        var m = {    
            '\b': '\\b',
            '\t': '\\t',
            '\n': '\\n',
            '\f': '\\f',
            '\r': '\\r',
            '"' : '\\"',
            '\\': '\\\\'
        };

        function stringify(value, whitelist) {
            var a,          
                i,          
                k,          
                l,          
                r = /["\\\x00-\x1f\x7f-\x9f]/g,
                v;          

            switch (typeof value) {
            case 'string':





                return r.test(value) ?
                    '"' + value.replace(r, function (a) {
                        var c = m[a];
                        if (c) {
                            return c;
                        }
                        c = a.charCodeAt();
                        return '\\u00' + Math.floor(c / 16).toString(16) +
                                                   (c % 16).toString(16);
                    }) + '"' :
                    '"' + value + '"';

            case 'number':



                return isFinite(value) ? String(value) : 'null';

            case 'boolean':
            case 'null':
                return String(value);

            case 'object':




                if (!value) {
                    return 'null';
                }



                if (typeof value.toJSON === 'function') {
                    return stringify(value.toJSON());
                }
                a = [];
                if (typeof value.length === 'number' &&
                        !(value.propertyIsEnumerable('length'))) {




                    l = value.length;
                    for (i = 0; i < l; i += 1) {
                        a.push(stringify(value[i], whitelist) || 'null');
                    }



                    return '[' + a.join(',') + ']';
                }
                if (whitelist) {




                    l = whitelist.length;
                    for (i = 0; i < l; i += 1) {
                        k = whitelist[i];
                        if (typeof k === 'string') {
                            v = stringify(value[k], whitelist);
                            if (v) {
                                a.push(stringify(k) + ':' + v);
                            }
                        }
                    }
                } else {



                    for (k in value) {
                        if (typeof k === 'string') {
                            v = stringify(value[k], whitelist);
                            if (v) {
                                a.push(stringify(k) + ':' + v);
                            }
                        }
                    }
                }



                return '{' + a.join(',') + '}';
            }
            return undefined;
        }

        return {
            stringify: stringify,
            parse: function (text, filter) {
                var j;

                function walk(k, v) {
                    var i, n;
                    if (v && typeof v === 'object') {
                        for (i in v) {
                            if (Object.prototype.hasOwnProperty.apply(v, [i])) {
                                n = walk(i, v[i]);
                                if (n !== undefined) {
                                    v[i] = n;
                                }
                            }
                        }
                    }
                    return filter(k, v);
                }
















                if (/^[\],:{}\s]*$/.test(text.replace(/\\./g, '@').
replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(:?[eE][+\-]?\d+)?/g, ']').
replace(/(?:^|:|,)(?:\s*\[)+/g, ''))) {






                    j = eval('(' + text + ')');




                    return typeof filter === 'function' ? walk('', j) : j;
                }



                throw new SyntaxError('parseJSON');
            }
        };
    }();
}



test();


jit(false);

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);


  var testPairs = [
    ["{}", {}],
    ["[]", []],
    ['{"foo":"bar"}', {"foo":"bar"}],
    ['{"null":null}', {"null":null}],
    ['{"five":5}', {"five":5}],
  ]
  
  var a = [];
  for (var i=0; i < testPairs.length; i++) {
    var pair = testPairs[i];
    var s = emulatedJSON.stringify(pair[1])
    a[i] = s;
  }
  print(a.join("\n"));
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

