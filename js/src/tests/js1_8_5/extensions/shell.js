









var workerDir = '';



if (typeof version != 'undefined')
{
  version(185);
}




function AsmJSArrayBuffer(size) {
    var ab = new ArrayBuffer(size);
    (new Function('global', 'foreign', 'buffer', '' +
'        "use asm";' +
'        var i32 = new global.Int32Array(buffer);' +
'        function g() {};' +
'        return g;' +
''))(Function("return this")(),null,ab);
    return ab;
}
