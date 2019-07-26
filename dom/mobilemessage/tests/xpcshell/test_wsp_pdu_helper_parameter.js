


let WSP = {};
subscriptLoader.loadSubScript("resource://gre/modules/WspPduHelper.jsm", WSP);
WSP.debug = do_print;

function run_test() {
  run_next_test();
}







add_test(function test_Parameter_decodeTypedParameter() {
  function func(data) {
    return WSP.Parameter.decodeTypedParameter(data);
  }

  
  wsp_decode_test_ex(func, [7, 0, 0, 0, 0, 0, 0, 0], null, "CodeError");
  
  wsp_decode_test_ex(func, [1, 0, 0], {name: "q", value: null});
  
  wsp_decode_test_ex(func, [1, 0xFF], null, "NotWellKnownEncodingError");
  
  wsp_decode_test_ex(func, [1, 0, 100], {name: "q", value: 0.99});
  
  wsp_decode_test_ex(func, [1, 0x10, 48, 46, 57, 57, 0],
                     {name: "secure", value: "0.99"});
  
  wsp_decode_test_ex(func, [1, 0x0A, 60, 115, 109, 105, 108, 62, 0],
                     {name: "start", value: "<smil>"});
  
  wsp_decode_test_ex(func, [1, 0x0A, 128], null);

  run_next_test();
});



add_test(function test_Parameter_decodeUntypedParameter() {
  function func (data) {
    return WSP.Parameter.decodeUntypedParameter(data);
  }

  wsp_decode_test_ex(func, [1], null, "CodeError");
  wsp_decode_test_ex(func, [65, 0, 0], {name: "a", value: null});
  
  wsp_decode_test_ex(func, [65, 0, 1, 0], {name: "a", value: 0});
  
  wsp_decode_test_ex(func, [65, 0, 66, 0], {name: "a", value: "B"});

  run_next_test();
});



add_test(function test_Parameter_decode() {
  wsp_decode_test(WSP.Parameter, [1, 0x0A, 60, 115, 109, 105, 108, 62, 0],
                  {name: "start", value: "<smil>"});
  wsp_decode_test(WSP.Parameter, [65, 0, 66, 0], {name: "a", value: "B"});

  run_next_test();
});



add_test(function test_Parameter_decodeMultiple() {
  wsp_decode_test_ex(function(data) {
      return WSP.Parameter.decodeMultiple(data, 13);
    }, [1, 0x0A, 60, 115, 109, 105, 108, 62, 0, 65, 0, 66, 0], {start: "<smil>", a: "B"}
  );

  run_next_test();
});



add_test(function test_Parameter_encodeTypedParameter() {
  function func(data, input) {
    WSP.Parameter.encodeTypedParameter(data, input);
    return data.array;
  }

  
  wsp_encode_test_ex(func, {name: "xxx", value: 0}, null, "NotWellKnownEncodingError");
  wsp_encode_test_ex(func, {name: "q", value: 0}, [0x80, 1]);
  wsp_encode_test_ex(func, {name: "name", value: "A"}, [0x85, 65, 0]);

  run_next_test();
});



add_test(function test_Parameter_encodeUntypedParameter() {
  function func(data, input) {
    WSP.Parameter.encodeUntypedParameter(data, input);
    return data.array;
  }

  wsp_encode_test_ex(func, {name: "q", value: 0}, [113, 0, 0x80]);
  wsp_encode_test_ex(func, {name: "name", value: "A"}, [110, 97, 109, 101, 0, 65, 0]);

  run_next_test();
});



add_test(function test_Parameter_encodeMultiple() {
  function func(data, input) {
    WSP.Parameter.encodeMultiple(data, input);
    return data.array;
  }

  wsp_encode_test_ex(func, {q: 0, n: "A"}, [0x80, 1, 110, 0, 65, 0]);

  run_next_test();
});



add_test(function test_Parameter_encode() {

  wsp_encode_test(WSP.Parameter, {name: "q", value: 0}, [0x80, 1]);
  wsp_encode_test(WSP.Parameter, {name: "n", value: "A"}, [110, 0, 65, 0]);

  run_next_test();
});
