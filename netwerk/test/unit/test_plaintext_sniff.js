

do_load_httpd_js();





var contentTypeHeaderList =
[
 [ "text/plain", true ],
 [ "text/plain; charset=ISO-8859-1", true ],
 [ "text/plain; charset=iso-8859-1", true ],
 [ "text/plain; charset=UTF-8", true ],
 [ "text/plain; charset=unknown", false ],
 [ "text/plain; param", false ],
 [ "text/plain; charset=ISO-8859-1; param", false ],
 [ "text/plain; charset=iso-8859-1; param", false ],
 [ "text/plain; charset=UTF-8; param", false ],
 [ "text/plain; charset=utf-8", false ],
 [ "text/plain; charset=utf8", false ],
 [ "text/plain; charset=UTF8", false ],
 [ "text/plain; charset=iSo-8859-1", false ]
];




var bodyList =
[
 [ "Plaintext", false ]
];


var BOMList =
[
 "\xFE\xFF",  
 "\xFF\xFE",  
 "\xEF\xBB\xBF", 
 "\x00\x00\xFE\xFF", 
 "\x00\x00\xFF\xFE" 
];





function isBinaryChar(ch) {
  return (0 <= ch && ch <= 8) || (14 <= ch && ch <= 26) ||
         (28 <= ch && ch <= 31);
}


var i;
for (i = 0; i <= 127; ++i) {  
  bodyList.push([ String.fromCharCode(i), isBinaryChar(i) ]);
}


var j;
for (i = 0; i <= 127; ++i) {
  for (j = 0; j < BOMList.length; ++j) {
    bodyList.push([ BOMList[j] + String.fromCharCode(i, i), false ]);
  }
}


for (i = 0; i <= 127; ++i) {
  for (j = 0; j < BOMList.length; ++j) {
    bodyList.push([ BOMList[j] + String.fromCharCode(i),
                    BOMList[j].length == 2 && isBinaryChar(i) ]);
  }
}

function makeChan(headerIdx, bodyIdx) {
  var ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
  var chan =
    ios.newChannel("http://localhost:4444/" + headerIdx + "/" + bodyIdx, null,
                   null)
       .QueryInterface(Components.interfaces.nsIHttpChannel);

  chan.loadFlags |=
    Components.interfaces.nsIChannel.LOAD_CALL_CONTENT_SNIFFERS;

  return chan;
}

function makeListener(headerIdx, bodyIdx) {
  var listener = {
    onStartRequest : function test_onStartR(request, ctx) {
      try {
        var chan = request.QueryInterface(Components.interfaces.nsIChannel);

        do_check_eq(chan.status, Components.results.NS_OK);
        
        var type = chan.contentType;

        var expectedType =
          contentTypeHeaderList[headerIdx][1] && bodyList[bodyIdx][1] ?
            "application/x-vnd.mozilla.guess-from-ext" : "text/plain";
        if (expectedType != type) {
          do_throw("Unexpected sniffed type '" + type + "'.  " +
                   "Should be '" + expectedType + "'.  " +
                   "Header is ['" +
                     contentTypeHeaderList[headerIdx][0] + "', " +
                     contentTypeHeaderList[headerIdx][1] + "].  " +
                   "Body is ['" +
                     bodyList[bodyIdx][0].toSource() + "', " +
                     bodyList[bodyIdx][1] +
                   "].");
        }
        do_check_eq(expectedType, type);
      } catch (e) {
        do_throw("Unexpected exception: " + e);
      }

      throw Components.results.NS_ERROR_ABORT;
    },

    onDataAvailable: function test_ODA() {
      do_throw("Should not get any data!");
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
      
      ++headerIdx;
      if (headerIdx == contentTypeHeaderList.length) {
        headerIdx = 0;
        ++bodyIdx;
      }

      if (bodyIdx == bodyList.length) {
        httpserv.stop();
      } else {
        doTest(headerIdx, bodyIdx);
      }

      do_test_finished();
    }    
  };

  return listener;
}

function doTest(headerIdx, bodyIdx) {
  var chan = makeChan(headerIdx, bodyIdx);

  var listener = makeListener(headerIdx, bodyIdx);

  chan.asyncOpen(listener, null);

  do_test_pending();    
}

function createResponse(headerIdx, bodyIdx, metadata, response) {
  response.setHeader("Content-Type", contentTypeHeaderList[headerIdx][0], false);
  response.bodyOutputStream.write(bodyList[bodyIdx][0],
                                  bodyList[bodyIdx][0].length);
}

function makeHandler(headerIdx, bodyIdx) {
  var f = 
    function handlerClosure(metadata, response) {
      return createResponse(headerIdx, bodyIdx, metadata, response);
    };
  return f;
}

var httpserv;
function run_test() {
  
  return;

  
  
  
  if ("@mozilla.org/windows-registry-key;1" in Cc) {
    return;
  }
  
  httpserv = new nsHttpServer();

  for (i = 0; i < contentTypeHeaderList.length; ++i) {
    for (j = 0; j < bodyList.length; ++j) {
      httpserv.registerPathHandler("/" + i + "/" + j, makeHandler(i, j));
    }
  }
  
  httpserv.start(4444);

  doTest(0, 0);
}
