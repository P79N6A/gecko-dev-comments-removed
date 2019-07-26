


let WSP = {};
subscriptLoader.loadSubScript("resource://gre/modules/WspPduHelper.jsm", WSP);
WSP.debug = do_print;

function run_test() {
  run_next_test();
}







add_test(function test_Text_decode() {
  for (let i = 0; i < 256; i++) {
    if (i == 0) {
      wsp_decode_test(WSP.Text, [0], null, "NullCharError");
    } else if ((i < WSP.CTLS) || (i == WSP.DEL)) {
      wsp_decode_test(WSP.Text, [i], null, "CodeError");
    } else {
      wsp_decode_test(WSP.Text, [i], String.fromCharCode(i));
    }
  }
  
  wsp_decode_test(WSP.Text, strToCharCodeArray("\r\n \t \t \t", true), " ");
  wsp_decode_test(WSP.Text, strToCharCodeArray("\r\n \t \t \t"), " ");
  wsp_decode_test(WSP.Text, strToCharCodeArray("\r\n \t \t \tA"), " ");

  run_next_test();
});



add_test(function test_Text_encode() {
  for (let i = 0; i < 256; i++) {
    if ((i < WSP.CTLS) || (i == WSP.DEL)) {
      wsp_encode_test(WSP.Text, String.fromCharCode(i), null, "CodeError");
    } else {
      wsp_encode_test(WSP.Text, String.fromCharCode(i), [i]);
    }
  }

  run_next_test();
});







add_test(function test_NullTerminatedTexts_decode() {
  
  wsp_decode_test(WSP.NullTerminatedTexts, strToCharCodeArray(" ", true), null, "RangeError");
  
  wsp_decode_test(WSP.NullTerminatedTexts, strToCharCodeArray(" \n"), null, "CodeError");
  
  wsp_decode_test(WSP.NullTerminatedTexts, strToCharCodeArray(""), "");
  wsp_decode_test(WSP.NullTerminatedTexts, strToCharCodeArray("oops"), "oops");
  
  wsp_decode_test(WSP.NullTerminatedTexts, strToCharCodeArray("A\r\n \t \t \tB"), "A B");

  run_next_test();
});



add_test(function test_NullTerminatedTexts_encode() {
  wsp_encode_test(WSP.NullTerminatedTexts, "", [0]);
  wsp_encode_test(WSP.NullTerminatedTexts, "Hello, World!",
                  strToCharCodeArray("Hello, World!"));

  run_next_test();
});





let TOKEN_SEPS = "()<>@,;:\\\"/[]?={} \t";



add_test(function test_Token_decode() {
  for (let i = 0; i < 256; i++) {
    if (i == 0) {
      wsp_decode_test(WSP.Token, [i], null, "NullCharError");
    } else if ((i < WSP.CTLS) || (i >= WSP.ASCIIS)
        || (TOKEN_SEPS.indexOf(String.fromCharCode(i)) >= 0)) {
      wsp_decode_test(WSP.Token, [i], null, "CodeError");
    } else {
      wsp_decode_test(WSP.Token, [i], String.fromCharCode(i));
    }
  }

  run_next_test();
});



add_test(function test_Token_encode() {
  for (let i = 0; i < 256; i++) {
    if ((i < WSP.CTLS) || (i >= WSP.ASCIIS)
        || (TOKEN_SEPS.indexOf(String.fromCharCode(i)) >= 0)) {
      wsp_encode_test(WSP.Token, String.fromCharCode(i), null, "CodeError");
    } else {
      wsp_encode_test(WSP.Token, String.fromCharCode(i), [i]);
    }
  }

  run_next_test();
});







add_test(function test_URIC_decode() {
  let uric = "!#$%&'()*+,-./0123456789:;=?@ABCDEFGHIJKLMN"
             + "OPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";
  for (let i = 0; i < 256; i++) {
    if (i == 0) {
      wsp_decode_test(WSP.URIC, [i], null, "NullCharError");
    } else if (uric.indexOf(String.fromCharCode(i)) >= 0) {
      wsp_decode_test(WSP.URIC, [i], String.fromCharCode(i));
    } else {
      wsp_decode_test(WSP.URIC, [i], null, "CodeError");
    }
  }

  run_next_test();
});







add_test(function test_TextString_decode() {
  
  wsp_decode_test(WSP.TextString, [127, 128, 0], String.fromCharCode(128));
  
  wsp_decode_test(WSP.TextString, [127, 32, 0], null, "CodeError");
  
  wsp_decode_test(WSP.TextString, [128, 0], null, "CodeError");
  
  wsp_decode_test(WSP.TextString, [32, 0], " ");

  run_next_test();
});



add_test(function test_TextString_encode() {
  
  wsp_encode_test(WSP.TextString, String.fromCharCode(128), [127, 128, 0]);
  
  wsp_encode_test(WSP.TextString, "Mozilla", strToCharCodeArray("Mozilla"));

  run_next_test();
});







add_test(function test_TokenText_decode() {
  wsp_decode_test(WSP.TokenText, [65], null, "RangeError");
  wsp_decode_test(WSP.TokenText, [0], "");
  wsp_decode_test(WSP.TokenText, [65, 0], "A");

  run_next_test();
});



add_test(function test_TokenText_encode() {
  wsp_encode_test(WSP.TokenText, "B2G", strToCharCodeArray("B2G"));

  run_next_test();
});







add_test(function test_QuotedString_decode() {
  
  wsp_decode_test(WSP.QuotedString, [32, 0], null, "CodeError");
  
  wsp_decode_test(WSP.QuotedString, [34, 32], null, "RangeError");
  wsp_decode_test(WSP.QuotedString, [34, 32, 0], " ");

  run_next_test();
});



add_test(function test_QuotedString_encode() {
  wsp_encode_test(WSP.QuotedString, "B2G", [34].concat(strToCharCodeArray("B2G")));

  run_next_test();
});







add_test(function test_TextValue_decode() {
  wsp_decode_test(WSP.TextValue, [0], null);
  wsp_decode_test(WSP.TextValue, [65, 0], "A");
  wsp_decode_test(WSP.TextValue, [32, 0], null, "CodeError");
  wsp_decode_test(WSP.TextValue, [34, 32, 0], " ");

  run_next_test();
});



add_test(function test_TextValue_encode() {
  wsp_encode_test(WSP.TextValue, undefined, [0]);
  wsp_encode_test(WSP.TextValue, null, [0]);
  wsp_encode_test(WSP.TextValue, "", [0]);
  wsp_encode_test(WSP.TextValue, "A", [65, 0]);
  wsp_encode_test(WSP.TextValue, "\x80", [34, 128, 0]);

  run_next_test();
});
