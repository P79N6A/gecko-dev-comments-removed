















const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const CC = Components.Constructor;








const DATA_ARRAY = [0, 255, 254, 0, 1, 2, 3, 0, 255, 255, 254, 0],
      DATA_ARRAY_BUFFER = new ArrayBuffer(DATA_ARRAY.length),
      TYPED_DATA_ARRAY = new Uint8Array(DATA_ARRAY_BUFFER),
      HELLO_WORLD = "hlo wrld. ",
      BIG_ARRAY = new Array(65539),
      BIG_ARRAY_2 = new Array(65539);

TYPED_DATA_ARRAY.set(DATA_ARRAY, 0);

for (var i_big = 0; i_big < BIG_ARRAY.length; i_big++) {
  BIG_ARRAY[i_big] = Math.floor(Math.random() * 256);
  BIG_ARRAY_2[i_big] = Math.floor(Math.random() * 256);
}

const BIG_ARRAY_BUFFER = new ArrayBuffer(BIG_ARRAY.length),
      BIG_ARRAY_BUFFER_2 = new ArrayBuffer(BIG_ARRAY_2.length);
const BIG_TYPED_ARRAY = new Uint8Array(BIG_ARRAY_BUFFER),
      BIG_TYPED_ARRAY_2 = new Uint8Array(BIG_ARRAY_BUFFER_2);
BIG_TYPED_ARRAY.set(BIG_ARRAY);
BIG_TYPED_ARRAY_2.set(BIG_ARRAY_2);

const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "init"),
      InputStreamPump = CC("@mozilla.org/network/input-stream-pump;1",
                           "nsIInputStreamPump",
                           "init"),
      BinaryInputStream = CC("@mozilla.org/binaryinputstream;1",
                             "nsIBinaryInputStream",
                             "setInputStream"),
      BinaryOutputStream = CC("@mozilla.org/binaryoutputstream;1",
                              "nsIBinaryOutputStream",
                              "setOutputStream"),
      TCPSocket = new (CC("@mozilla.org/tcp-socket;1",
                     "nsIDOMTCPSocket"))();

const gInChild = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
                  .processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

Cu.import("resource://gre/modules/Services.jsm");







function get_platform() {
  var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"]
                              .getService(Components.interfaces.nsIXULRuntime);
  return xulRuntime.OS;
}

function is_content() {
  return this._inChild = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
                            .processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
}





function TestServer() {
  this.listener = ServerSocket(-1, true, -1);
  do_print('server: listening on ' + this.listener.port);
  this.listener.asyncListen(this);

  this.binaryInput = null;
  this.input = null;
  this.binaryOutput = null;
  this.output = null;

  this.onconnect = null;
  this.ondata = null;
  this.onclose = null;
}

TestServer.prototype = {
  onSocketAccepted: function(socket, trans) {
    if (this.input)
      do_throw("More than one live connection!?");

    do_print('server: got client connection');
    this.input = trans.openInputStream(0, 0, 0);
    this.binaryInput = new BinaryInputStream(this.input);
    this.output = trans.openOutputStream(0, 0, 0);
    this.binaryOutput = new BinaryOutputStream(this.output);

    new InputStreamPump(this.input, -1, -1, 0, 0, false).asyncRead(this, null);

    if (this.onconnect)
      this.onconnect();
    else
      do_throw("Received unexpected connection!");
  },

  onStopListening: function(socket) {
  },

  onDataAvailable: function(request, context, inputStream, offset, count) {
    var readData = this.binaryInput.readByteArray(count);
    if (this.ondata) {
      try {
        this.ondata(readData);
      } catch(ex) {
        
        if (ex === Cr.NS_ERROR_ABORT)
          throw ex;
        
        do_print('Caught exception: ' + ex + '\n' + ex.stack);
        do_throw('test is broken; bad ondata handler; see above');
      }
    } else {
      do_throw('Received ' + count + ' bytes of unexpected data!');
    }
  },

  onStartRequest: function(request, context) {
  },

  onStopRequest: function(request, context, status) {
    if (this.onclose)
      this.onclose();
    else
      do_throw("Received unexpected close!");
  },

  close: function() {
    this.binaryInput.close();
    this.binaryOutput.close();
  },

  


  reset: function() {
    this.binaryInput = null;
    this.input = null;
    this.binaryOutput = null;
    this.output = null;
  },
};

function makeSuccessCase(name) {
  return function() {
    do_print('got expected: ' + name);
    run_next_test();
  };
}

function makeJointSuccess(names) {
  let funcs = {}, successCount = 0;
  names.forEach(function(name) {
    funcs[name] = function() {
      do_print('got expected: ' + name);
      if (++successCount === names.length)
        run_next_test();
    };
  });
  return funcs;
}

function makeFailureCase(name) {
  return function() {
    let argstr;
    if (arguments.length) {
      argstr = '(args: ' +
        Array.map(arguments, function(x) { return x.data + ""; }).join(" ") + ')';
    }
    else {
      argstr = '(no arguments)';
    }
    do_throw('got unexpected: ' + name + ' ' + argstr);
  };
}

function makeExpectData(name, expectedData, fromEvent, callback) {
  let dataBuffer = fromEvent ? null : [], done = false;
  let dataBufferView = null;
  return function(receivedData) {
    if (receivedData.data) {
      receivedData = receivedData.data;
    }
    let recvLength = receivedData.byteLength !== undefined ?
        receivedData.byteLength : receivedData.length;

    if (fromEvent) {
      if (dataBuffer) {
        let newBuffer = new ArrayBuffer(dataBuffer.byteLength + recvLength);
        let newBufferView = new Uint8Array(newBuffer);
        newBufferView.set(dataBufferView, 0);
        newBufferView.set(receivedData, dataBuffer.byteLength);
        dataBuffer = newBuffer;
        dataBufferView = newBufferView;
      }
      else {
        dataBuffer = receivedData;
        dataBufferView = new Uint8Array(dataBuffer);
      }
    }
    else {
      dataBuffer = dataBuffer.concat(receivedData);
    }
    do_print(name + ' received ' + recvLength + ' bytes');

    if (done)
      do_throw(name + ' Received data event when already done!');

    let dataView = dataBuffer.byteLength !== undefined ? new Uint8Array(dataBuffer) : dataBuffer;
    if (dataView.length >= expectedData.length) {
      
      for (let i = 0; i < expectedData.length; i++) {
        if (dataView[i] !== expectedData[i]) {
          do_throw(name + ' Received mismatched character at position ' + i);
        }
      }
      if (dataView.length > expectedData.length)
        do_throw(name + ' Received ' + dataView.length + ' bytes but only expected ' +
                 expectedData.length + ' bytes.');

      done = true;
      if (callback) {
        callback();
      } else {
        run_next_test();
      }
    }
  };
}

var server = null, sock = null, failure_drain = null;













function connectSock() {
  server.reset();
  var yayFuncs = makeJointSuccess(['serveropen', 'clientopen']);

  sock = TCPSocket.open(
    '127.0.0.1', server.listener.port,
    { binaryType: 'arraybuffer' });

  sock.onopen = yayFuncs.clientopen;
  sock.ondrain = null;
  sock.ondata = makeFailureCase('data');
  sock.onerror = makeFailureCase('error');
  sock.onclose = makeFailureCase('close');

  server.onconnect = yayFuncs.serveropen;
  server.ondata = makeFailureCase('serverdata');
  server.onclose = makeFailureCase('serverclose');
}






function sendData() {
  server.ondata = makeExpectData('serverdata', DATA_ARRAY);
  if (!sock.send(DATA_ARRAY_BUFFER)) {
    do_throw("send should not have buffered such a small amount of data");
  }
}







function sendBig() {
  var yays = makeJointSuccess(['serverdata', 'clientdrain']),
      amount = 0;

  server.ondata = function (data) {
    amount += data.length;
    if (amount === BIG_TYPED_ARRAY.length) {
      yays.serverdata();
    }
  };
  sock.ondrain = function(evt) {
    if (sock.bufferedAmount) {
      do_throw("sock.bufferedAmount was > 0 in ondrain");
    }
    yays.clientdrain(evt);
  }
  if (sock.send(BIG_ARRAY_BUFFER)) {
    do_throw("expected sock.send to return false on large buffer send");
  }
}






function receiveData() {
  server.ondata = makeFailureCase('serverdata');
  sock.ondata = makeExpectData('data', DATA_ARRAY, true);

  server.binaryOutput.writeByteArray(DATA_ARRAY, DATA_ARRAY.length);
}






function serverCloses() {
  
  
  var yayFuncs = makeJointSuccess(['clientclose', 'serverclose']);
  sock.ondata = makeFailureCase('data');
  sock.onclose = yayFuncs.clientclose;
  server.onclose = yayFuncs.serverclose;

  server.close();
}






function clientCloses() {
  
  
  var yayFuncs = makeJointSuccess(['clientclose', 'serverclose']);
  server.onclose = yayFuncs.serverclose;
  sock.onclose = yayFuncs.clientclose;

  sock.close();
}





function bufferedClose() {
  var yays = makeJointSuccess(['serverdata', 'clientclose', 'serverclose']);
  server.ondata = makeExpectData(
    "ondata", BIG_TYPED_ARRAY, false, yays.serverdata);
  server.onclose = yays.serverclose;
  sock.onclose = yays.clientclose;
  sock.send(BIG_ARRAY_BUFFER);
  sock.close();
}






function badConnect() {
  
  sock = TCPSocket.open('127.0.0.1', 2);

  sock.onopen = makeFailureCase('open');
  sock.ondata = makeFailureCase('data');

  let success = makeSuccessCase('error');
  let gotError = false;
  sock.onerror = function(event) {
    do_check_eq(event.data.name, 'ConnectionRefusedError');
    gotError = true;
  };
  sock.onclose = function() {
    if (!gotError)
      do_throw('got close without error!');
    else
      success();
  };
}







function drainTwice() {
  let yays = makeJointSuccess(
    ['ondrain', 'ondrain2',
    'ondata', 'ondata2',
    'serverclose', 'clientclose']);
  let ondrainCalled = false,
      ondataCalled = false;

  function maybeSendNextData() {
    if (!ondrainCalled || !ondataCalled) {
      
      return;
    }

    server.ondata = makeExpectData(
      "ondata2", BIG_TYPED_ARRAY_2, false, yays.ondata2);

    sock.ondrain = yays.ondrain2;

    if (sock.send(BIG_ARRAY_BUFFER_2)) {
      do_throw("sock.send(BIG_TYPED_ARRAY_2) did not return false to indicate buffering");
    }

    sock.close();
  }

  function clientOndrain() {
    yays.ondrain();
    ondrainCalled = true;
    maybeSendNextData();
  }

  function serverSideCallback() {
    yays.ondata();
    ondataCalled = true;
    maybeSendNextData();
  }

  server.onclose = yays.serverclose;
  server.ondata = makeExpectData(
    "ondata", BIG_TYPED_ARRAY, false, serverSideCallback);

  sock.onclose = yays.clientclose;
  sock.ondrain = clientOndrain;

  if (sock.send(BIG_ARRAY_BUFFER)) {
    throw new Error("sock.send(BIG_TYPED_ARRAY) did not return false to indicate buffering");
  }
}

function cleanup() {
  do_print("Cleaning up");
  sock.close();
  if (!gInChild)
    Services.prefs.clearUserPref('dom.mozTCPSocket.enabled');
  run_next_test();
}






function bufferTwice() {
  let yays = makeJointSuccess(
    ['ondata', 'ondrain', 'serverclose', 'clientclose']);

  let double_array = new Uint8Array(BIG_ARRAY.concat(BIG_ARRAY_2));
  server.ondata = makeExpectData(
    "ondata", double_array, false, yays.ondata);

  server.onclose = yays.serverclose;
  sock.onclose = yays.clientclose;

  sock.ondrain = function () {
    sock.close();
    yays.ondrain();
  }

  if (sock.send(BIG_ARRAY_BUFFER)) {
    throw new Error("sock.send(BIG_TYPED_ARRAY) did not return false to indicate buffering");
  }
  if (sock.send(BIG_ARRAY_BUFFER_2)) {
    throw new Error("sock.send(BIG_TYPED_ARRAY_2) did not return false to indicate buffering on second synchronous call to send");
  }
}








function childbuffered() {
  let yays = makeJointSuccess(['ondrain', 'serverdata',
                               'clientclose', 'serverclose']);
  sock.ondrain = function() {
    yays.ondrain();
    sock.close();
  };

  server.ondata = makeExpectData(
    'ondata', DATA_ARRAY, false, yays.serverdata);

  let internalSocket = sock.QueryInterface(Ci.nsITCPSocketInternal);
  internalSocket.updateBufferedAmount(65535, 
                                      0);
  if (sock.send(DATA_ARRAY_BUFFER)) {
    do_throw("expected sock.send to return false.");
  }

  sock.onclose = yays.clientclose;
  server.onclose = yays.serverclose;
}









function childnotbuffered() {
  let yays = makeJointSuccess(['serverdata', 'clientclose', 'serverclose']);
  server.ondata = makeExpectData('ondata', BIG_ARRAY, false, yays.serverdata);
  if (sock.send(BIG_ARRAY_BUFFER)) {
    do_throw("sock.send(BIG_TYPED_ARRAY) did not return false to indicate buffering");
  }
  let internalSocket = sock.QueryInterface(Ci.nsITCPSocketInternal);
  internalSocket.updateBufferedAmount(0, 
                                      1);

  
  sock.ondrain = makeFailureCase('drain');
  sock.onclose = yays.clientclose;
  server.onclose = yays.serverclose;
  do_timeout(1000, function() {
    sock.close();
  });
};


add_test(connectSock);
add_test(sendData);
add_test(sendBig);
add_test(receiveData);

add_test(serverCloses);


add_test(connectSock);
add_test(clientCloses);


add_test(connectSock);
add_test(bufferedClose);

if (get_platform() !== "Darwin") {
  
  
  
  add_test(badConnect);
}


add_test(connectSock);
add_test(drainTwice);


add_test(connectSock);
add_test(bufferTwice);

if (is_content()) {
  add_test(connectSock);
  add_test(childnotbuffered);

  add_test(connectSock);
  add_test(childbuffered);
}


add_test(cleanup);

function run_test() {
  if (!gInChild)
    Services.prefs.setBoolPref('dom.mozTCPSocket.enabled', true);

  server = new TestServer();

  run_next_test();
}
