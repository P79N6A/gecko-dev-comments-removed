

load('CharsetConversionTests.js');

const inString = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff";

const expectedString = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\u20ac\u0081\u201a\u0083\u201e\u2026\u2020\u2021\u0088\u2030\u0160\u2039\u015a\u0164\u017d\u0179\u0090\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u0098\u2122\u0161\u203a\u015b\u0165\u017e\u017a\u00a0\u02c7\u02d8\u0141\u00a4\u0104\u00a6\u00a7\u00a8\u00a9\u015e\u00ab\u00ac\u00ad\u00ae\u017b\u00b0\u00b1\u02db\u0142\u00b4\u00b5\u00b6\u00b7\u00b8\u0105\u015f\u00bb\u013d\u02dd\u013e\u017c\u0154\u00c1\u00c2\u0102\u00c4\u0139\u0106\u00c7\u010c\u00c9\u0118\u00cb\u011a\u00cd\u00ce\u010e\u0110\u0143\u0147\u00d3\u00d4\u0150\u00d6\u00d7\u0158\u016e\u00da\u0170\u00dc\u00dd\u0162\u00df\u0155\u00e1\u00e2\u0103\u00e4\u013a\u0107\u00e7\u010d\u00e9\u0119\u00eb\u011b\u00ed\u00ee\u010f\u0111\u0144\u0148\u00f3\u00f4\u0151\u00f6\u00f7\u0159\u016f\u00fa\u0171\u00fc\u00fd\u0163\u02d9";

const aliases = [ "windows-1250", "x-cp1250", "cp1250" ];

function run_test() {
  testDecodeAliases();
}
