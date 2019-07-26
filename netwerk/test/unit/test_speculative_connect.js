





const CC = Components.Constructor;
const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "init");
var serv;
var ios;









var localIPv4Literals =
    [ 
      "10.0.0.1", "10.10.10.10", "10.255.255.255",         
      "172.16.0.1", "172.23.172.12", "172.31.255.255",     
      "192.168.0.1", "192.168.192.168", "192.168.255.255", 
      
      "169.254.0.1", "169.254.192.154", "169.254.255.255"  
    ];
var localIPv6Literals =
    [ 
      "fc00::1", "fdfe:dcba:9876:abcd:ef01:2345:6789:abcd",
      "fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
      
      "fe80::1", "fe80::abcd:ef01:2345:6789",
      "febf:ffff:ffff:ffff:ffff:ffff:ffff:ffff"
    ];
var localIPLiterals = localIPv4Literals.concat(localIPv6Literals);







var remoteIPv4Literals =
    [ "93.184.216.119", 
      "74.125.239.130", 
      "63.245.217.105", 
      "173.252.110.27"  
    ];

var remoteIPv6Literals =
    [ "2607:f8b0:4005:802::1009",        
      "2620:101:8008:5::2:1",            
      "2a03:2880:2110:df07:face:b00c::1" 
    ];

var remoteIPLiterals = remoteIPv4Literals.concat(remoteIPv6Literals);



var testList =
    [ test_speculative_connect,
      test_hostnames_resolving_to_local_addresses,
      test_hostnames_resolving_to_remote_addresses,
      test_proxies_with_local_addresses,
      test_proxies_with_remote_addresses
    ];

var testDescription =
    [ "Expect pass with localhost",
      "Expect failure with resolved local IPs",
      "Expect success with resolved remote IPs",
      "Expect failure for proxies with local IPs",
      "Expect success for proxies with remote IPs"
    ];

var testIdx = 0;
var hostIdx = 0;






function TestServer() {
    this.listener = ServerSocket(-1, true, -1);
    this.listener.asyncListen(this);
}

TestServer.prototype = {
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsIServerSocket) ||
            iid.equals(Ci.nsISupports))
            return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    },
    onSocketAccepted: function(socket, trans) {
        try { this.listener.close(); } catch(e) {}
        do_check_true(true);
        next_test();
    },

    onStopListening: function(socket) {}
};





function TestOutputStreamCallback(hostname, proxied, expectSuccess, next) {
    this.hostname = hostname;
    this.proxied = proxied;
    this.expectSuccess = expectSuccess;
    this.next = next;
    this.dummyContent = "Dummy content";
}

TestOutputStreamCallback.prototype = {
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsIOutputStreamCallback) ||
            iid.equals(Ci.nsISupports))
            return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    },
    onOutputStreamReady: function(stream) {
        do_check_neq(typeof(stream), undefined);
        try {
            stream.write(this.dummyContent, this.dummyContent.length);
        } catch (e) {
            
            do_check_instanceof(e, Ci.nsIException);
            if (this.expectSuccess) {
                
                
                if (this.proxied) {
                    do_check_true(e.result == Cr.NS_ERROR_NET_TIMEOUT ||
                                  e.result == Cr.NS_ERROR_PROXY_CONNECTION_REFUSED);
                } else {
                    do_check_true(e.result == Cr.NS_ERROR_NET_TIMEOUT ||
                                  e.result == Cr.NS_ERROR_CONNECTION_REFUSED);
                }
            } else {
                
                do_check_eq(e.result, Cr.NS_ERROR_CONNECTION_REFUSED);
            }
            stream.closeWithStatus(e.result);
            this.next();
            return;
        }
        
        if (this.expectSuccess) {
            do_check_true(true, "Success for " + this.hostname);
        } else {
            do_throw("Speculative Connect should have failed for " +
                     this.hostname);
        }
        stream.closeWithStatus(Cr.NS_ERROR_BINDING_ABORTED);
        this.next();
    }
};






function test_speculative_connect() {
    serv = new TestServer();
    var URI = ios.newURI("http://localhost:" + serv.listener.port + "/just/a/test", null, null);
    ios.QueryInterface(Ci.nsISpeculativeConnect)
        .speculativeConnect(URI, null);
}

















function test_hostnames_resolving_to_addresses(host, expectSuccess, next) {
    do_print(host);
    var sts = Cc["@mozilla.org/network/socket-transport-service;1"]
              .getService(Ci.nsISocketTransportService);
    do_check_neq(typeof(sts), undefined);
    var transport = sts.createTransport(null, 0, host, 80, null);
    do_check_neq(typeof(transport), undefined);

    transport.connectionFlags = Ci.nsISocketTransport.DISABLE_RFC1918;
    transport.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 1);
    do_check_eq(1, transport.getTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT));

    var outStream = transport.openOutputStream(Ci.nsITransport.OPEN_UNBUFFERED,0,0);
    do_check_neq(typeof(outStream), undefined);

    var callback = new TestOutputStreamCallback(host, false,
                                                expectSuccess,
                                                next);
    do_check_neq(typeof(callback), undefined);

    
    
    
    var gThreadManager = Cc["@mozilla.org/thread-manager;1"]
    .getService(Ci.nsIThreadManager);
    var mainThread = gThreadManager.currentThread;

    try {
        outStream.QueryInterface(Ci.nsIAsyncOutputStream)
                 .asyncWait(callback, 0, 0, mainThread);
    } catch (e) {
        do_throw("asyncWait should not fail!");
    }
}













function test_hostnames_resolving_to_local_addresses() {
    if (hostIdx >= localIPLiterals.length) {
        
        next_test();
        return;
    }
    var host = localIPLiterals[hostIdx++];
    
    var next = test_hostnames_resolving_to_local_addresses;
    test_hostnames_resolving_to_addresses(host, false, next);
}













function test_hostnames_resolving_to_remote_addresses() {
    if (hostIdx >= remoteIPLiterals.length) {
        
        next_test();
        return;
    }
    var host = remoteIPLiterals[hostIdx++];
    
    var next = test_hostnames_resolving_to_remote_addresses;
    test_hostnames_resolving_to_addresses(host, true, next);
}







function test_proxies(proxyHost, expectSuccess, next) {
    do_print("Proxy: " + proxyHost);
    var sts = Cc["@mozilla.org/network/socket-transport-service;1"]
              .getService(Ci.nsISocketTransportService);
    do_check_neq(typeof(sts), undefined);
    var pps = Cc["@mozilla.org/network/protocol-proxy-service;1"]
              .getService();
    do_check_neq(typeof(pps), undefined);

    var proxyInfo = pps.newProxyInfo("http", proxyHost, 8080, 0, 1, null);
    do_check_neq(typeof(proxyInfo), undefined);

    var transport = sts.createTransport(null, 0, "dummyHost", 80, proxyInfo);
    do_check_neq(typeof(transport), undefined);

    transport.connectionFlags = Ci.nsISocketTransport.DISABLE_RFC1918;

    transport.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 1);
    do_check_eq(1, transport.getTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT));

    var outStream = transport.openOutputStream(Ci.nsITransport.OPEN_UNBUFFERED,0,0);
    do_check_neq(typeof(outStream), undefined);

    var callback = new TestOutputStreamCallback(proxyHost, true,
                                                expectSuccess,
                                                next);
    do_check_neq(typeof(callback), undefined);

    
    
    
    var gThreadManager = Cc["@mozilla.org/thread-manager;1"]
    .getService(Ci.nsIThreadManager);
    var mainThread = gThreadManager.currentThread;

    try {
        outStream.QueryInterface(Ci.nsIAsyncOutputStream)
                 .asyncWait(callback, 0, 0, mainThread);
    } catch (e) {
        do_throw("asyncWait should not fail!");
    }
}













function test_proxies_with_local_addresses() {
    if (hostIdx >= localIPLiterals.length) {
        
        next_test();
        return;
    }
    var host = localIPLiterals[hostIdx++];
    
    var next = test_proxies_with_local_addresses;
    test_proxies(host, false, next);
}













function test_proxies_with_remote_addresses() {
    if (hostIdx >= remoteIPLiterals.length) {
        
        next_test();
        return;
    }
    var host = remoteIPLiterals[hostIdx++];
    
    var next = test_proxies_with_remote_addresses;
    test_proxies(host, true, next);
}






function next_test() {
    if (testIdx >= testList.length) {
        
        do_test_finished();
        return;
    }
    do_print("SpeculativeConnect: " + testDescription[testIdx]);
    hostIdx = 0;
    
    testList[testIdx++]();
}





function run_test() {
    ios = Cc["@mozilla.org/network/io-service;1"]
        .getService(Ci.nsIIOService);

    do_test_pending();
    
    test_hostnames_resolving_to_local_addresses();
}

