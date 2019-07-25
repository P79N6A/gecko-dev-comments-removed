










































const EXPORTED_SYMBOLS = ['encrypt', 'decrypt'];

function Paused() {
}
Paused.prototype = {
  toString: function Paused_toString() {
    return "[Generator Paused]";
  }
}










function encrypt(plaintext, password) {
  var v = new Array(2), k = new Array(4), s = "", i;

  
  plaintext = escape(plaintext);

  
  for (i = 0; i < 4; i++)
    k[i] = Str4ToLong(password.slice(i * 4, (i + 1) * 4));

  for (i = 0; i < plaintext.length; i += 8) {
    
    
    v[0] = Str4ToLong(plaintext.slice(i, i + 4));
    v[1] = Str4ToLong(plaintext.slice(i + 4, i + 8));
    code(v, k);
    s += LongToStr4(v[0]) + LongToStr4(v[1]);

    if (i % 512 == 0)
      yield new Paused();
  }

  yield escCtrlCh(s);
}



function decrypt(ciphertext, password) {
  var v = new Array(2), k = new Array(4), s = "", i;

  for (i = 0; i < 4; i++)
    k[i] = Str4ToLong(password.slice(i * 4, (i + 1) * 4));

  ciphertext = unescCtrlCh(ciphertext);
  for (i = 0; i < ciphertext.length; i += 8) {
    
    v[0] = Str4ToLong(ciphertext.slice(i, i + 4));
    v[1] = Str4ToLong(ciphertext.slice(i + 4, i + 8));
    decode(v, k);
    s += LongToStr4(v[0]) + LongToStr4(v[1]);

    if (i % 512 == 0)
      yield new Paused();
  }

  
  s = s.replace(/\0+$/, '');

  yield unescape(s);
}


function code(v, k) {
  
  
  var y = v[0], z = v[1];
  var delta = 0x9E3779B9, limit = delta*32, sum = 0;

  while (sum != limit) {
    y += (z<<4 ^ z>>>5)+z ^ sum+k[sum & 3];
    sum += delta;
    z += (y<<4 ^ y>>>5)+y ^ sum+k[sum>>>11 & 3];
    
    
  }
  v[0] = y; v[1] = z;
}

function decode(v, k) {
  var y = v[0], z = v[1];
  var delta = 0x9E3779B9, sum = delta*32;

  while (sum != 0) {
    z -= (y<<4 ^ y>>>5)+y ^ sum+k[sum>>>11 & 3];
    sum -= delta;
    y -= (z<<4 ^ z>>>5)+z ^ sum+k[sum & 3];
  }
  v[0] = y; v[1] = z;
}




function Str4ToLong(s) {  
  var v = 0;
  for (var i=0; i<4; i++) v |= s.charCodeAt(i) << i*8;
  return isNaN(v) ? 0 : v;
}

function LongToStr4(v) {  
  var s = String.fromCharCode(v & 0xFF, v>>8 & 0xFF, v>>16 & 0xFF, v>>24 & 0xFF);
  return s;
}

function escCtrlCh(str) {  
  return str.replace(/[\0\t\n\v\f\r\xa0'"!]/g, function(c) { return '!' + c.charCodeAt(0) + '!'; });
}

function unescCtrlCh(str) {  
  return str.replace(/!\d\d?\d?!/g, function(c) { return String.fromCharCode(c.slice(1,-1)); });
}
