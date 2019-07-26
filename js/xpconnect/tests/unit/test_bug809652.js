





const Cu = Components.utils;
const TypedArrays = [ Int8Array, Uint8Array, Int16Array, Uint16Array,
                      Int32Array, Uint32Array, Float32Array, Float64Array,
                      Uint8ClampedArray ];



function run_test() {

  var sb = new Cu.Sandbox('http://www.example.org');
  sb.obj = {foo: 2};

  
  sb.ab = new ArrayBuffer(8);
  for (var i = 0; i < 8; ++i)
    new Uint8Array(sb.ab)[i] = i * 10;
  sb.ta = [];
  TypedArrays.forEach(function(f) sb.ta.push(new f(sb.ab)));
  sb.dv = new DataView(sb.ab);

  
  checkThrows("Object.prototype.__lookupSetter__('__proto__').call(obj, {});", sb);
  sb.re = /f/;
  checkThrows("RegExp.prototype.exec.call(re, 'abcdefg').index", sb);
  sb.d = new Date();
  checkThrows("Date.prototype.setYear.call(d, 2011)", sb);
  sb.m = new Map();
  checkThrows("(new Map()).clear.call(m)", sb);
  checkThrows("ArrayBuffer.prototype.__lookupGetter__('byteLength').call(ab);", sb);
  checkThrows("ArrayBuffer.prototype.slice.call(ab, 0);", sb);
  checkThrows("DataView.prototype.getInt8.call(dv, 0);", sb);

  
  checkThrows("Date.prototype.getYear.call(d)", sb);
  checkThrows("Date.prototype.valueOf.call(d)", sb);
  checkThrows("d.valueOf()", sb);
  checkThrows("Date.prototype.toString.call(d)", sb);
  checkThrows("d.toString()", sb);

  
  function testForTypedArray(t) {
    sb.curr = t;
    sb.currName = t.constructor.name;
    checkThrows("this[currName].prototype.subarray.call(curr, 0)[0]", sb);
    checkThrows("(new this[currName]).__lookupGetter__('length').call(curr)", sb);
    checkThrows("(new this[currName]).__lookupGetter__('buffer').call(curr)", sb);
    checkThrows("(new this[currName]).__lookupGetter__('byteOffset').call(curr)", sb);
    checkThrows("(new this[currName]).__lookupGetter__('byteLength').call(curr)", sb);
  }
  sb.ta.forEach(testForTypedArray);
}

function checkThrows(expression, sb) {
  var result = Cu.evalInSandbox('(function() { try { ' + expression + '; return "allowed"; } catch (e) { return e.toString(); }})();', sb);
  dump('result: ' + result + '\n\n\n');
  do_check_true(!!/denied/.exec(result));
}

