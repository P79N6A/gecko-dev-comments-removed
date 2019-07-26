



'use strict';
 
var PREFIX_NAME = 'identity.mozilla.com/picl/v1/';
var bitSlice = sjcl.bitArray.bitSlice;
var salt = sjcl.codec.hex.toBits('');












function hkdf(ikm, info, salt, length, callback) {
  var mac = new sjcl.misc.hmac(salt, sjcl.hash.sha256);
  mac.update(ikm);

  
  var prk = mac.digest();

  
  var hashLength = 32;
  var num_blocks = Math.ceil(length / hashLength);
  var prev = sjcl.codec.hex.toBits('');
  var output = '';

  for (var i = 0; i < num_blocks; i++) {
    var hmac = new sjcl.misc.hmac(prk, sjcl.hash.sha256);

    var input = sjcl.bitArray.concat(
      sjcl.bitArray.concat(prev, info),
      sjcl.codec.utf8String.toBits((String.fromCharCode(i + 1)))
    );

    hmac.update(input);

    prev = hmac.digest();
    output += sjcl.codec.hex.fromBits(prev);
  }

  var truncated = sjcl.bitArray.clamp(sjcl.codec.hex.toBits(output), length * 8);

  callback(truncated);
}

 








function deriveHawkCredentials(tokenHex, context, size, callback) {
  var token = sjcl.codec.hex.toBits(tokenHex);
  var info = sjcl.codec.utf8String.toBits(PREFIX_NAME + context);

  hkdf(token, info, salt, size || 3 * 32, function(out) {
    var authKey = bitSlice(out, 8 * 32, 8 * 64);
    var bundleKey = bitSlice(out, 8 * 64);
    callback({
      algorithm: 'sha256',
      id: sjcl.codec.hex.fromBits(bitSlice(out, 0, 8 * 32)),
      key: sjcl.codec.hex.fromBits(authKey),
      bundleKey: bundleKey
    });
  });
}
 
