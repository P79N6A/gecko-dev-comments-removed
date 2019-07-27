










var config = getBuildConfiguration();
if (!config['moz-memory'])
  quit(0);

if (config['pointer-byte-size'] == 4)
  var s = (s32, s64) => s32
else
  var s = (s32, s64) => s64



function tByteSize(obj) {
  var nurserySize = byteSize(obj);
  minorgc();
  var tenuredSize = byteSize(obj);
  if (nurserySize != tenuredSize) {
    print("nursery size: " + nurserySize + "  tenured size: " + tenuredSize);
    return -1; 
  }

  return tenuredSize;
}














assertEq(tByteSize(""),                                                 s(16, 24)); 
assertEq(tByteSize("1"),                                                s(16, 24)); 
assertEq(tByteSize("1234567"),                                          s(16, 24)); 
assertEq(tByteSize("12345678"),                                         s(32, 24)); 
assertEq(tByteSize("123456789.12345"),                                  s(32, 24)); 
assertEq(tByteSize("123456789.123456"),                                 s(32, 32)); 
assertEq(tByteSize("123456789.123456789.123"),                          s(32, 32)); 
assertEq(tByteSize("123456789.123456789.1234"),                         s(48, 56)); 
assertEq(tByteSize("123456789.123456789.123456789.1"),                  s(48, 56)); 
assertEq(tByteSize("123456789.123456789.123456789.12"),                 s(64, 72)); 




assertEq(tByteSize("千"),						s(16, 24)); 
assertEq(tByteSize("千早"),    						s(16, 24)); 
assertEq(tByteSize("千早ぶ"),    					s(16, 24)); 
assertEq(tByteSize("千早ぶる"),    					s(32, 24)); 
assertEq(tByteSize("千早ぶる神"),    					s(32, 24)); 
assertEq(tByteSize("千早ぶる神代"),					s(32, 24)); 
assertEq(tByteSize("千早ぶる神代も"),					s(32, 24)); 
assertEq(tByteSize("千早ぶる神代もき"),					s(32, 32)); 
assertEq(tByteSize("千早ぶる神代もきかず龍"),				s(32, 32)); 
assertEq(tByteSize("千早ぶる神代もきかず龍田"),    			s(48, 56)); 
assertEq(tByteSize("千早ぶる神代もきかず龍田川 か"),    			s(48, 56)); 
assertEq(tByteSize("千早ぶる神代もきかず龍田川 から"),    			s(64, 72)); 
assertEq(tByteSize("千早ぶる神代もきかず龍田川 からくれなゐに水く"),    	s(64, 72)); 
assertEq(tByteSize("千早ぶる神代もきかず龍田川 からくれなゐに水くく"),    	s(80, 88)); 
assertEq(tByteSize("千早ぶる神代もきかず龍田川 からくれなゐに水くくるとは"),	s(80, 88)); 




var fragment8 = "En un lugar de la Mancha, de cuyo nombre no quiero acordarme"; 
var rope8 = fragment8;
for (var i = 0; i < 10; i++) 
  rope8 = rope8 + rope8;
assertEq(tByteSize(rope8),                                              s(16, 24));
var matches8 = rope8.match(/(de cuyo nombre no quiero acordarme)/);
assertEq(tByteSize(rope8),                                              s(16 + 65536,  24 + 65536));







rope8a = rope8 + fragment8;
assertEq(tByteSize(rope8a),                                             s(16, 24));
rope8a.match(/x/, function() { assertEq(true, false); });
assertEq(tByteSize(rope8a),                                             s(16 + 65536,  24 + 65536));
assertEq(tByteSize(rope8),                                              s(16, 24));





var fragment16 = "μουσάων Ἑλικωνιάδων ἀρχώμεθ᾽ ἀείδειν";
var rope16 = fragment16;
for (var i = 0; i < 10; i++) 
  rope16 = rope16 + rope16;
assertEq(tByteSize(rope16),                                     s(16,  24));
let matches16 = rope16.match(/(Ἑλικωνιάδων ἀρχώμεθ᾽)/);
assertEq(tByteSize(rope16),                                     s(16 + 131072,  24 + 131072));


assertEq(tByteSize(rope8.substr(1000, 2000)),                   s(16,  24));
assertEq(tByteSize(rope16.substr(1000, 2000)),                  s(16,  24));
assertEq(tByteSize(matches8[0]),                                s(16,  24));
assertEq(tByteSize(matches8[1]),                                s(16,  24));
assertEq(tByteSize(matches16[0]),                               s(16,  24));
assertEq(tByteSize(matches16[1]),                               s(16,  24));







rope16a = rope16 + fragment16;
assertEq(tByteSize(rope16a),                                    s(16, 24));
rope16a.match(/x/, function() { assertEq(true, false); });
assertEq(tByteSize(rope16a),                                    s(16 + 131072,  24 + 131072));
assertEq(tByteSize(rope16),                                     s(16, 24));
