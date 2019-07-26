



addOnPreMain(function() {

  onmessage = function(aMsg){

    
    var encoder = new TextEncoder();
    encoder['encoding'] = "utf-8";
    var utf8Array = encoder['encode'](aMsg.data);

    
    var strLength = utf8Array.length;
    var ptr = Module['_malloc'](strLength + 1);
    var heap = Module['HEAPU8'];
    new Uint8Array(heap.buffer, ptr, strLength).set(utf8Array);
    
    heap[ptr + strLength] = 0;

    var lang = Pointer_stringify(_detectLangCode(ptr));
    var confident = !!Module['ccall']("lastResultReliable", "number");
    postMessage({'language': lang,
                 'confident': confident});

    Module['_free'](ptr);
  };

  postMessage("ready");

});
