'use strict';

const SERVER_BACKLOG = -1;

const SOCKET_EVENTS = ['open', 'data', 'drain', 'error', 'close'];

function concatUint8Arrays(a, b) {
  let newArr = new Uint8Array(a.length + b.length);
  newArr.set(a, 0);
  newArr.set(b, a.length);
  return newArr;
}

function assertUint8ArraysEqual(a, b, comparingWhat) {
  if (a.length !== b.length) {
    ok(false, comparingWhat + ' arrays do not have the same length; ' +
       a.length + ' versus ' + b.length);
    return;
  }
  for (let i = 0; i < a.length; i++) {
    if (a[i] !== b[i]) {
      ok(false, comparingWhat + ' arrays differ at index ' + i +
         a[i] + ' versus ' + b[i]);
      return;
    }
  }
  ok(true, comparingWhat + ' arrays were equivalent.');
}







function listenForEventsOnSocket(socket, socketType) {
  let wantDataLength = null;
  let pendingResolve = null;
  let receivedEvents = [];
  let receivedData = null;
  let handleGenericEvent = function(event) {
    dump('(' + socketType + ' event: ' + event.type + ')\n');
    if (pendingResolve && wantDataLength === null) {
      pendingResolve(event);
      pendingResolve = null;
    } else {
      receivedEvents.push(event);
    }
  };

  socket.onopen = handleGenericEvent;
  socket.ondrain = handleGenericEvent;
  socket.onerror = handleGenericEvent;
  socket.onclose = handleGenericEvent;
  socket.ondata = function(event) {
    dump('(' + socketType + ' event: ' + event.type + ' length: ' +
         event.data.byteLength + ')\n');
    var arr = new Uint8Array(event.data);
    if (receivedData === null) {
      receivedData = arr;
    } else {
      receivedData = concatUint8Arrays(receivedData, arr);
    }
    if (wantDataLength !== null &&
        receivedData.length >= wantDataLength) {
      pendingResolve(receivedData);
      pendingResolve = null;
      receivedData = null;
      wantDataLength = null;
    }
  };


  return {
    





    waitForEvent: function() {
      if (pendingResolve) {
        throw new Error('only one wait allowed at a time.');
      }

      if (receivedEvents.length) {
        return Promise.resolve(receivedEvents.shift());
      }

      dump('(' + socketType + ' waiting for event)\n');
      return new Promise(function(resolve, reject) {
        pendingResolve = resolve;
      });
    },
    





    waitForDataWithAtLeastLength: function(length) {
      if (pendingResolve) {
        throw new Error('only one wait allowed at a time.');
      }
      if (receivedData && receivedData.length >= length) {
        let promise = Promise.resolve(receivedData);
        receivedData = null;
        return promise;
      }
      dump('(' + socketType + ' waiting for ' + length + ' bytes)\n');
      return new Promise(function(resolve, reject) {
        pendingResolve = resolve;
        wantDataLength = length;
      });
    }
  };
}







function waitForConnection(listeningServer) {
  return new Promise(function(resolve, reject) {
    
    
    
    listeningServer.onconnect = function(socket) {
      
      
      listeningServer.onconnect = function() {
        ok(false, 'Received a connection when not expecting one.');
      };
      ok(true, 'Listening server accepted socket');
      resolve({
        socket: socket,
        queue: listenForEventsOnSocket(socket, 'server')
      });
    };
  });
}

function defer() {
  var deferred = {};
  deferred.promise = new Promise(function(resolve, reject) {
    deferred.resolve = resolve;
    deferred.reject = reject;
  });
  return deferred;
}


function* test_basics() {
  
  let prefDeferred = defer();
  SpecialPowers.pushPrefEnv(
    { set: [ ['dom.mozTCPSocket.enabled', true] ] },
    prefDeferred.resolve);
  yield prefDeferred.promise;

  let permDeferred = defer();
  SpecialPowers.pushPermissions(
    [ { type: 'tcp-socket', allow: true, context: document } ],
    permDeferred.resolve);
  yield permDeferred.promise;

  
  
  
  let serverPort = 8085;

  let TCPSocket = navigator.mozTCPSocket;
  
  let listeningServer = TCPSocket.listen(serverPort,
                                         { binaryType: 'arraybuffer' },
                                         SERVER_BACKLOG);

  let connectedPromise = waitForConnection(listeningServer);

  
  let clientSocket = TCPSocket.open('127.0.0.1', serverPort,
                                    { binaryType: 'arraybuffer' });
  let clientQueue = listenForEventsOnSocket(clientSocket, 'client');

  
  is((yield clientQueue.waitForEvent()).type, 'open', 'got open event');
  is(clientSocket.readyState, 'open', 'client readyState is open');

  
  let { socket: serverSocket, queue: serverQueue } = yield connectedPromise;
  is(serverSocket.readyState, 'open', 'server readyState is open');

  
  
  
  let smallUint8Array = new Uint8Array(256);
  for (let i = 0; i < smallUint8Array.length; i++) {
    smallUint8Array[i] = i;
  }
  is(clientSocket.send(smallUint8Array.buffer, 0, smallUint8Array.length), true,
     'Client sending less than 64k, buffer should not be full.');

  let serverReceived = yield serverQueue.waitForDataWithAtLeastLength(256);
  assertUint8ArraysEqual(serverReceived, smallUint8Array,
                         'Server received/client sent');

  
  
  is(serverSocket.send(smallUint8Array.buffer, 0, smallUint8Array.length), true,
     'Server sending less than 64k, buffer should not be full.');

  let clientReceived = yield clientQueue.waitForDataWithAtLeastLength(256);
  assertUint8ArraysEqual(clientReceived, smallUint8Array,
                         'Client received/server sent');

  
  
  
  is(clientSocket.send(smallUint8Array.buffer, 0, 7),
     true, 'Client sending less than 64k, buffer should not be full.');
  is(clientSocket.send(smallUint8Array.buffer, 7, smallUint8Array.length - 7),
     true, 'Client sending less than 64k, buffer should not be full.');

  serverReceived = yield serverQueue.waitForDataWithAtLeastLength(256);
  assertUint8ArraysEqual(serverReceived, smallUint8Array,
                         'Server received/client sent');

  
  
  is(serverSocket.send(smallUint8Array.buffer, 0, 7),
     true, 'Server sending less than 64k, buffer should not be full.');
  is(serverSocket.send(smallUint8Array.buffer, 7, smallUint8Array.length - 7),
     true, 'Server sending less than 64k, buffer should not be full.');

  clientReceived = yield clientQueue.waitForDataWithAtLeastLength(256);
  assertUint8ArraysEqual(clientReceived, smallUint8Array,
                         'Client received/server sent');


  
  
  let bigUint8Array = new Uint8Array(65536 + 3);
  for (let i = 0; i < bigUint8Array.length; i++) {
    bigUint8Array[i] = i % 256;
  }
  
  
  for (let iSend = 0; iSend < 2; iSend++) {
    
    is(clientSocket.send(bigUint8Array.buffer, 0, bigUint8Array.length), false,
       'Client sending more than 64k should result in the buffer being full.');
    is((yield clientQueue.waitForEvent()).type, 'drain',
       'The drain event should fire after a large send that indicated full.');

    serverReceived = yield serverQueue.waitForDataWithAtLeastLength(
      bigUint8Array.length);
    assertUint8ArraysEqual(serverReceived, bigUint8Array,
                           'server received/client sent');

    
    is(serverSocket.send(bigUint8Array.buffer, 0, bigUint8Array.length), false,
       'Server sending more than 64k should result in the buffer being full.');
    is((yield serverQueue.waitForEvent()).type, 'drain',
       'The drain event should fire after a large send that indicated full.');

    clientReceived = yield clientQueue.waitForDataWithAtLeastLength(
      bigUint8Array.length);
    assertUint8ArraysEqual(clientReceived, bigUint8Array,
                           'client received/server sent');
  }

  
  serverSocket.close();
  is(serverSocket.readyState, 'closing',
     'readyState should be closing immediately after calling close');

  is((yield clientQueue.waitForEvent()).type, 'close',
     'The client should get a close event when the server closes.');
  is(clientSocket.readyState, 'closed',
     'client readyState should be closed after close event');
  is((yield serverQueue.waitForEvent()).type, 'close',
     'The server should get a close event when it closes itself.');
  is(serverSocket.readyState, 'closed',
     'server readyState should be closed after close event');

  
  connectedPromise = waitForConnection(listeningServer);
  clientSocket = TCPSocket.open('127.0.0.1', serverPort,
                                { binaryType: 'arraybuffer' });
  clientQueue = listenForEventsOnSocket(clientSocket, 'client');
  is((yield clientQueue.waitForEvent()).type, 'open', 'got open event');

  let connectedResult = yield connectedPromise;
  
  serverSocket = connectedResult.socket;
  serverQueue = connectedResult.queue;

  
  clientSocket.close();
  is(clientSocket.readyState, 'closing',
     'client readyState should be losing immediately after calling close');

  is((yield clientQueue.waitForEvent()).type, 'close',
     'The client should get a close event when it closes itself.');
  is(clientSocket.readyState, 'closed',
     'client readyState should be closed after the close event is received');
  is((yield serverQueue.waitForEvent()).type, 'close',
     'The server should get a close event when the client closes.');
  is(serverSocket.readyState, 'closed',
     'server readyState should be closed after the close event is received');


  
  connectedPromise = waitForConnection(listeningServer);
  clientSocket = TCPSocket.open('127.0.0.1', serverPort,
                                { binaryType: 'arraybuffer' });
  clientQueue = listenForEventsOnSocket(clientSocket, 'client');
  is((yield clientQueue.waitForEvent()).type, 'open', 'got open event');

  connectedResult = yield connectedPromise;
  
  serverSocket = connectedResult.socket;
  serverQueue = connectedResult.queue;

  
  
  is(clientSocket.send(bigUint8Array.buffer, 0, bigUint8Array.length), false,
     'Client sending more than 64k should result in the buffer being full.');
  clientSocket.close();
  
  is((yield clientQueue.waitForEvent()).type, 'drain',
     'The drain event should fire after a large send that returned true.');
  
  is((yield clientQueue.waitForEvent()).type, 'close',
     'The close event should fire after the drain event.');

  
  serverReceived = yield serverQueue.waitForDataWithAtLeastLength(
    bigUint8Array.length);
  assertUint8ArraysEqual(serverReceived, bigUint8Array,
                         'server received/client sent');
  
  is((yield serverQueue.waitForEvent()).type, 'close',
     'The drain event should fire after a large send that returned true.');


  
  
  
  listeningServer.close();

  
  clientSocket = TCPSocket.open('127.0.0.1', serverPort,
                                { binaryType: 'arraybuffer' });
  clientQueue = listenForEventsOnSocket(clientSocket, 'client');
  is((yield clientQueue.waitForEvent()).type, 'error', 'fail to connect');
  is(clientSocket.readyState, 'closed',
     'client readyState should be closed after the failure to connect');
}

add_task(test_basics);
