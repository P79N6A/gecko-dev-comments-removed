const CC = Components.Constructor;

const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "init");















function TestServer() {
  this.reset();

  
  
  this.listener = ServerSocket(-1, true, -1);
  this.port = this.listener.port;
  do_print('server: listening on', this.port);
  this.listener.asyncListen(this);
}

TestServer.prototype = {
  onSocketAccepted: function(socket, trans) {
    do_print('server: got client connection');

    
    if (this.input !== null) {
      try { socket.close(); } catch(ignore) {}
      do_throw("Test written to handle one connection at a time.");
    }

    try {
      this.input = trans.openInputStream(0, 0, 0);
      this.output = trans.openOutputStream(0, 0, 0);
      this.selfAddr = trans.getScriptableSelfAddr();
      this.peerAddr = trans.getScriptablePeerAddr();

      this.acceptCallback();
    } catch(e) {
      


      do_report_unexpected_exception(e, "in TestServer.onSocketAccepted");
    }

    this.reset();
  } ,
  
  onStopListening: function(socket) {} ,

  


  reset: function() {
    if (this.input)
      try { this.input.close(); } catch(ignore) {}
    if (this.output)
      try { this.output.close(); } catch(ignore) {}

    this.input = null;
    this.output = null;
    this.acceptCallback = null;
    this.selfAddr = null;
    this.peerAddr = null;
  } ,

  


  stop: function() {
    this.reset();
    try { this.listener.close(); } catch(ignore) {}
  }
};






function checkAddrEqual(lhs, rhs) {
  do_check_eq(lhs.family, rhs.family);

  if (lhs.family === Ci.nsINetAddr.FAMILY_INET) {
    do_check_eq(lhs.address, rhs.address);
    do_check_eq(lhs.port, rhs.port);
  }
  
  
}





var sts;




var serv;






var connectTimeout = 5*1000;







var testDataStore = null;




function testIpv4() {
  testDataStore = {
    transport : null ,
    ouput : null
  }

  serv.acceptCallback = function() {
    
    serv.timeoutCallback = function(){};

    var selfAddr = testDataStore.transport.getScriptableSelfAddr();
    var peerAddr = testDataStore.transport.getScriptablePeerAddr();

    
    do_check_eq(peerAddr.family, Ci.nsINetAddr.FAMILY_INET);
    do_check_eq(peerAddr.port, testDataStore.transport.port);
    do_check_eq(peerAddr.port, serv.port);
    do_check_eq(peerAddr.address, "127.0.0.1");

    
    do_check_eq(selfAddr.family, Ci.nsINetAddr.FAMILY_INET);
    do_check_eq(selfAddr.address, "127.0.0.1");

    
    checkAddrEqual(selfAddr, serv.peerAddr);
    checkAddrEqual(peerAddr, serv.selfAddr);

    testDataStore = null;
    do_execute_soon(run_next_test);
  };

  
  





  testDataStore.transport = sts.createTransport(null, 0, '127.0.0.1', serv.port, null);
  



  testDataStore.output = testDataStore.transport.openOutputStream(Ci.nsITransport.OPEN_BLOCKING,0,0);

  




}





function run_test() {
  sts = Cc["@mozilla.org/network/socket-transport-service;1"]
            .getService(Ci.nsISocketTransportService);
  serv = new TestServer();

  do_register_cleanup(function(){ serv.stop(); });

  add_test(testIpv4);
  
  
    
  run_next_test();
}
