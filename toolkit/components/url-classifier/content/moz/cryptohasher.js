# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:



















this.G_CryptoHasher =
function G_CryptoHasher() {
  this.debugZone = "cryptohasher";
  this.hasher_ = null;
}

G_CryptoHasher.algorithms = {
  MD2: Ci.nsICryptoHash.MD2,
  MD5: Ci.nsICryptoHash.MD5,
  SHA1: Ci.nsICryptoHash.SHA1,
  SHA256: Ci.nsICryptoHash.SHA256,
  SHA384: Ci.nsICryptoHash.SHA384,
  SHA512: Ci.nsICryptoHash.SHA512,
};







 
G_CryptoHasher.prototype.init = function(algorithm) {
  var validAlgorithm = false;
  for (var alg in G_CryptoHasher.algorithms)
    if (algorithm == G_CryptoHasher.algorithms[alg])
      validAlgorithm = true;

  if (!validAlgorithm)
    throw new Error("Invalid algorithm: " + algorithm);

  this.hasher_ = Cc["@mozilla.org/security/hash;1"]
                 .createInstance(Ci.nsICryptoHash);
  this.hasher_.init(algorithm);
}






 
G_CryptoHasher.prototype.updateFromString = function(input) {
  if (!this.hasher_)
    throw new Error("You must initialize the hasher first!");

  var stream = Cc['@mozilla.org/io/string-input-stream;1']
               .createInstance(Ci.nsIStringInputStream);
  stream.setData(input, input.length);
  this.updateFromStream(stream);
}






 
G_CryptoHasher.prototype.updateFromArray = function(input) {
  if (!this.hasher_)
    throw new Error("You must initialize the hasher first!");

  this.hasher_.update(input, input.length);
}





G_CryptoHasher.prototype.updateFromStream = function(stream) {
  if (!this.hasher_)
    throw new Error("You must initialize the hasher first!");

  if (stream.available())
    this.hasher_.updateFromStream(stream, stream.available());
}



 
G_CryptoHasher.prototype.digestRaw = function() {
  var digest = this.hasher_.finish(false );
  this.hasher_ = null;
  return digest;
}



 
G_CryptoHasher.prototype.digestBase64 = function() {
  var digest = this.hasher_.finish(true );
  this.hasher_ = null;
  return digest;
}



 
G_CryptoHasher.prototype.digestHex = function() {
  var raw = this.digestRaw();
  return this.toHex_(raw);
}









 
G_CryptoHasher.prototype.toHex_ = function(str) {
  var hexchars = '0123456789ABCDEF';
  var hexrep = new Array(str.length * 2);

  for (var i = 0; i < str.length; ++i) {
    hexrep[i * 2] = hexchars.charAt((str.charCodeAt(i) >> 4) & 15);
    hexrep[i * 2 + 1] = hexchars.charAt(str.charCodeAt(i) & 15);
  }
  return hexrep.join('');
}

#ifdef DEBUG



this.TEST_G_CryptoHasher = function TEST_G_CryptoHasher() {
  if (G_GDEBUG) {
    var z = "cryptohasher UNITTEST";
    G_debugService.enableZone(z);

    G_Debug(z, "Starting");

    var md5 = function(str) {
      var hasher = new G_CryptoHasher();
      hasher.init(G_CryptoHasher.algorithms.MD5);
      hasher.updateFromString(str);
      return hasher.digestHex().toLowerCase();
    };

    
    var vectors = {"": "d41d8cd98f00b204e9800998ecf8427e",
                   "a": "0cc175b9c0f1b6a831c399e269772661",
                   "abc": "900150983cd24fb0d6963f7d28e17f72",
                   "message digest": "f96b697d7cb7938d525a2f31aaf161d0",
                   "abcdefghijklmnopqrstuvwxyz": "c3fcd3d76192e4007dfb496cca67e13b",
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789": "d174ab98d277d9f5a5611c2c9f419d9f",
                   "12345678901234567890123456789012345678901234567890123456789012345678901234567890": "57edf4a22be3c955ac49da2e2107b67a"};
                   
    G_Debug(z, "PASSED");
  }
}
#endif
