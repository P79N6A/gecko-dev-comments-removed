





"use strict";

Cu.import("resource://testing-common/httpd.js");

var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var nzp = Cc["@mozilla.org/network/networkzonepolicy;1"]
          .createInstance(Ci.nsINetworkZonePolicy);

var httpServ;

var uriBase;











function Listener(expectSuccess, loadGroupAllows) {
  this._expectSuccess = expectSuccess;
  this._loadGroupAllows = loadGroupAllows;
}

Listener.prototype = {
  _expectSuccess: false,
  _loadGroupAllows: true,
  _buffer: null,

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIStreamListener) ||
        iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest: function(request, ctx) {
    do_check_true(request instanceof Ci.nsIHttpChannel);
    if (this._expectSuccess) {
      do_check_true(Components.isSuccessCode(request.status));
      do_check_eq(request.requestSucceeded, true);
      do_check_eq(request.responseStatus, 200);
      request.visitResponseHeaders({ visitHeader: function(aHeader, aValue) {
        do_print(aHeader + ": " + aValue);
      }});
      do_print(request.responseStatus + ": " + request.responseStatusText);
      this._buffer = "";
    } else {
      do_check_false(Components.isSuccessCode(request.status));
    }
  },

  onDataAvailable: function(request, ctx, stream, off, cnt) {
    do_check_true(request instanceof Ci.nsIHttpChannel);
    if (!this._expectSuccess) {
      do_throw("Should not get data; private load forbidden!");
    }
    this._buffer = this._buffer.concat(read_stream(stream, cnt));
  },

  onStopRequest: function(request, ctx, status) {
    do_check_true(request instanceof Ci.nsIHttpChannel);
    
    do_check_eq(request.loadGroup.allowLoadsFromPrivateNetworks,
                this._loadGroupAllows);

    if (this._expectSuccess) {
      do_check_true(Components.isSuccessCode(status));
    } else {
      do_check_false(Components.isSuccessCode(status));
    }
    run_next_test();
  }
};






function test_basic_NetworkZonePolicy_pref() {
  
  
  var loadGroup = Cc["@mozilla.org/network/load-group;1"]
                  .createInstance(Ci.nsILoadGroup);
  var chan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  
  
  var prefs = Cc["@mozilla.org/preferences-service;1"]
              .getService(Ci.nsIPrefBranch);
  var nzpEnabled = prefs.getBoolPref("network.zonepolicy.enabled");
  do_check_true(nzpEnabled);

  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);

  
  
  var docChan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  docChan.loadGroup = loadGroup;
  docChan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  nzp.setPrivateNetworkPermission(docChan, false);
  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, false);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  
  
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  prefs.setBoolPref("network.zonepolicy.enabled", false);
  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, false);
  
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  prefs.setBoolPref("network.zonepolicy.enabled", true);
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, false);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  

  run_next_test();
}



function test_basic_NetworkZonePolicy_and_loadGroup() {
  
  var loadGroup = Cc["@mozilla.org/network/load-group;1"]
                  .createInstance(Ci.nsILoadGroup);
  var chan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);

  
  nzp.setPrivateNetworkPermission(chan, false);
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);

  
  
  var docChan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  docChan.loadGroup = loadGroup;
  docChan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  nzp.setPrivateNetworkPermission(docChan, false);
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, false);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  
  
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  run_next_test();
}




function test_loadGroup_and_ancestor(loadGroup, ancestor, chan, docChan) {
  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(ancestor.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  nzp.setPrivateNetworkPermission(chan, false);
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(ancestor.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  
  nzp.setPrivateNetworkPermission(docChan, false);
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, false);
  do_check_eq(ancestor.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  nzp.setPrivateNetworkPermission(docChan, true);
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(ancestor.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  ancestor.allowLoadsFromPrivateNetworks = false;
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  
  nzp.setPrivateNetworkPermission(docChan, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), false);

  
  ancestor.allowLoadsFromPrivateNetworks = true;
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);
  nzp.setPrivateNetworkPermission(docChan, false);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), false);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);
}



function test_basic_NetworkZonePolicy_loadGroup_and_parent() {
  
  var loadGroup = Cc["@mozilla.org/network/load-group;1"]
                  .createInstance(Ci.nsILoadGroup);
  var loadGroupAsChild = loadGroup.QueryInterface(Ci.nsILoadGroupChild)
  var chan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  var docChan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  docChan.loadGroup = loadGroup;
  docChan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  var parent = Cc["@mozilla.org/network/load-group;1"]
               .createInstance(Ci.nsILoadGroup);
  loadGroupAsChild.parentLoadGroup = parent;
  do_check_eq(parent, loadGroupAsChild.parentLoadGroup);

  test_loadGroup_and_ancestor(loadGroup, parent, chan, docChan);

  run_next_test();
}



function test_basic_NetworkZonePolicy_loadGroup_and_owner() {
  
  var loadGroup = Cc["@mozilla.org/network/load-group;1"]
                  .createInstance(Ci.nsILoadGroup);
  var chan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  var docChan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  docChan.loadGroup = loadGroup;
  docChan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  var owner = Cc["@mozilla.org/network/load-group;1"]
              .createInstance(Ci.nsILoadGroup);
  loadGroup.loadGroup = owner;
  do_check_eq(owner, loadGroup.loadGroup);

  test_loadGroup_and_ancestor(loadGroup, owner, chan, docChan);

  run_next_test();
}



function test_basic_NetworkZonePolicy_loadGroup_and_docshell() {
  
  var docShell = Cc["@mozilla.org/docshell;1"].createInstance(Ci.nsIDocShell);

  var docShellParent = Cc["@mozilla.org/docshell;1"]
                       .createInstance(Ci.nsIDocShell);
  docShellParent.addChild(docShell);

  var loadGroup = docShell.QueryInterface(Ci.nsIDocumentLoader).loadGroup;
  var dsParent = docShellParent.QueryInterface(Ci.nsIDocumentLoader).loadGroup;

  
  var chan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  
  var docChan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  docChan.loadGroup = loadGroup;
  docChan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  test_loadGroup_and_ancestor(loadGroup, dsParent, chan, docChan);

  run_next_test();
}



function test_loadGroup_immediate_ancestors(isInitialDocLoad) {
  
  var docShell = Cc["@mozilla.org/docshell;1"].createInstance(Ci.nsIDocShell);
  var docShellParent = Cc["@mozilla.org/docshell;1"]
                       .createInstance(Ci.nsIDocShell);
  docShellParent.addChild(docShell);

  var loadGroup = docShell.QueryInterface(Ci.nsIDocumentLoader).loadGroup;
  var dsParent = docShellParent.QueryInterface(Ci.nsIDocumentLoader).loadGroup;

  
  var owner = Cc["@mozilla.org/network/load-group;1"]
              .createInstance(Ci.nsILoadGroup);
  loadGroup.loadGroup = owner;
  do_check_eq(owner, loadGroup.loadGroup);

  
  var loadGroupAsChild = loadGroup.QueryInterface(Ci.nsILoadGroupChild)
  var parent = Cc["@mozilla.org/network/load-group;1"]
               .createInstance(Ci.nsILoadGroup);
  loadGroupAsChild.parentLoadGroup = parent;
  do_check_eq(parent, loadGroupAsChild.parentLoadGroup);

  
  var chan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  
  var docChan = ios.newChannel("http://localhost/failme/", null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  docChan.loadGroup = loadGroup;
  docChan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  
  do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
  do_check_eq(dsParent.allowLoadsFromPrivateNetworks, true);
  do_check_eq(owner.allowLoadsFromPrivateNetworks, true);
  do_check_eq(parent.allowLoadsFromPrivateNetworks, true);
  do_check_eq(nzp.checkPrivateNetworkPermission(chan), true);
  do_check_eq(nzp.checkPrivateNetworkPermission(docChan), true);

  
  for (var i = 0; i < 8; i++) {
    dsParent.allowLoadsFromPrivateNetworks = !!(i & 1);
    owner.allowLoadsFromPrivateNetworks = !!(i & 2);
    parent.allowLoadsFromPrivateNetworks = !!(i & 4);
    
    do_check_eq(loadGroup.allowLoadsFromPrivateNetworks, true);
    
    do_check_eq(nzp.checkPrivateNetworkPermission(chan), (i == 7));
    do_check_eq(nzp.checkPrivateNetworkPermission(docChan), (i == 7));
  }

  run_next_test();
}











function test_single_loadGroup(allowPrivateLoads,
                               expectSuccessfulResponse,
                               urlStr) {
  
  var loadGroup = Cc["@mozilla.org/network/load-group;1"]
                  .createInstance(Ci.nsILoadGroup);
  var chan = ios.newChannel(urlStr, null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;

  do_print("Setting loadgroup permission: " +
           (allowPrivateLoads ? "Allowing" : "Forbidding") +
           " private loads for " + urlStr + ".");
  loadGroup.allowLoadsFromPrivateNetworks = allowPrivateLoads;

  
  
  var listener = new Listener(expectSuccessfulResponse, allowPrivateLoads);
  chan.asyncOpen(listener, null);
}


function test_single_loadGroup_doc_load(allowPrivateLoads,
                                        loadGroupAllowsAfter,
                                        urlStr) {
  
  var loadGroup = Cc["@mozilla.org/network/load-group;1"]
                  .createInstance(Ci.nsILoadGroup);
  var chan = ios.newChannel(urlStr, null, null)
             .QueryInterface(Ci.nsIHttpChannel);
  chan.loadGroup = loadGroup;
  chan.loadFlags |= Ci.nsIChannel.LOAD_DOCUMENT_URI;

  do_print("Setting loadgroup permission: " +
           (allowPrivateLoads ? "Allowing" : "Forbidding") +
           " private loads for doc load " + urlStr + ".");
  loadGroup.allowLoadsFromPrivateNetworks = allowPrivateLoads;

  
  var listener = new Listener(true, loadGroupAllowsAfter);
  chan.asyncOpen(listener, null);
}

function prime_cache_entry(uri, isPrivate, networkID, onEntryPrimed) {
  do_print("Priming cache with a " + (isPrivate ? "private" : "public") +
           " entry.");

  var fakeResponseHead = "HTTP/1.1 200 OK\r\n" +
                         "Content-Type: text/plain\r\n" +
                         "Server: httpd.js\r\n" +
                         "Date: " + (new Date()).toString() + "\r\n";

  asyncOpenCacheEntry(uri, "disk", Ci.nsICacheStorage.OPEN_TRUNCATE, null,
    new OpenCallback(NEW, "a1m", "a1d", function(entry) {
      do_print("Created " + (isPrivate ? "private" : "public") + " entry");
      entry.setMetaDataElement("request-method", "GET");
      entry.setMetaDataElement("response-head", fakeResponseHead);
      if (isPrivate) {
        entry.setMetaDataElement("loaded-from-private-network", networkID);
      }
      do_print("Added metadata");
      asyncOpenCacheEntry(uri, "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "a1m", "a1d", function(entry) {
          do_print("Verifying " + (isPrivate ? "private" : "public") +
                   " entry created");
          if (isPrivate) {
            do_check_eq(entry.getMetaDataElement("loaded-from-private-network"),
                        networkID);
          }

          do_print((isPrivate ? "Private" : "Public") + " cache entry primed.");
          onEntryPrimed();
        })
      );
    })
  );
}




function test_private_cached_entry_same_network(privateLoadAllowed) {
  var uri = uriBase + "/failme/";
  prime_cache_entry(uri, true, ios.networkLinkID, function() {
    test_single_loadGroup(privateLoadAllowed, privateLoadAllowed, uri);
  });
}




function test_private_cached_entry_same_network_doc_load(privateLoadAllowed) {
  var uri = uriBase + "/failme/";
  prime_cache_entry(uri, true, ios.networkLinkID, function() {
    test_single_loadGroup_doc_load(privateLoadAllowed, true, uri);
  });
}


var fakeNetworkID = "{86437A10-658B-4637-8D41-9B3693F72762}";



function test_private_cached_entry_diff_network(privateLoadAllowed) {
  
  
  
  var uri = uriBase + (privateLoadAllowed ? "/passme/" : "/failme/");
  prime_cache_entry(uri, true, fakeNetworkID, function() {
    test_single_loadGroup(privateLoadAllowed, privateLoadAllowed, uri);
  });
}



function test_private_cached_entry_diff_network_doc_load(privateLoadAllowed) {
  
  
  
  
  var uri = uriBase + "/passme/";
  prime_cache_entry(uri, true, fakeNetworkID, function() {
    test_single_loadGroup_doc_load(privateLoadAllowed, true, uri);
  });
}


function test_public_cached_entry(privateCacheEntryAllowed) {
  var uri = uriBase + "/failme/";
  prime_cache_entry(uri, false, null, function() {
    test_single_loadGroup(privateCacheEntryAllowed, true, uri);
  });
}


function test_public_cached_entry_doc_load(privateCacheEntryAllowed) {
  
  
  
  var uri = uriBase + "/failme/";
  prime_cache_entry(uri, false, null, function() {
    test_single_loadGroup_doc_load(privateCacheEntryAllowed, false, uri);
  });
}

function test_public_to_private_navigation() {

}









function setup_http_server() {
  httpServ = new HttpServer();

  httpServ.registerPathHandler("/passme/", function (metadata, response) {
    do_print("Received request on http://localhost/passme/");
    var httpbody = "0123456789";
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "text/plain", false);
    response.bodyOutputStream.write(httpbody, httpbody.length);
  });

  httpServ.registerPathHandler("/failme/", function (metadata, response) {
    do_throw("http://localhost/failme/ should not not receive requests!");
  });
  httpServ.start(-1);

  do_register_cleanup(function() {
    httpServ.stop(function() {});
  });

  do_print("Started HTTP Server on " +
           httpServ.identity.primaryScheme + "://" +
           httpServ.identity.primaryHost + ":" +
           httpServ.identity.primaryPort);
}

function setup_and_add_tests() {
  
  do_get_profile();

  
  
  {
    var prefs = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
    if (!prefs.getBoolPref("network.zonepolicy.enabled")) {
      prefs.setBoolPref("network.zonepolicy.enabled", true);
      do_register_cleanup(function() {
        prefs.setBoolPref("network.zonepolicy.enabled", false);
      });
    }
  }

  uriBase = httpServ.identity.primaryScheme + "://" +
            httpServ.identity.primaryHost + ":" +
            httpServ.identity.primaryPort;

  var tests = [
    
    test_basic_NetworkZonePolicy_pref,

    
    function test_basic_NetworkZonePolicy_and_loadGroup_non_init_doc() {
      test_basic_NetworkZonePolicy_and_loadGroup(false);
    },
    function test_basic_NetworkZonePolicy_and_loadGroup_init_doc() {
      test_basic_NetworkZonePolicy_and_loadGroup(true);
    },
    test_basic_NetworkZonePolicy_loadGroup_and_parent,
    test_basic_NetworkZonePolicy_loadGroup_and_owner,
    test_basic_NetworkZonePolicy_loadGroup_and_docshell,
    function test_loadGroup_immediate_ancestors_non_init_doc() {
      test_loadGroup_immediate_ancestors(false);
    },
    function test_loadGroup_immediate_ancestors_init_doc() {
      test_loadGroup_immediate_ancestors(true);
    },

    
    function test_network_private_allowed() {
      test_single_loadGroup(true, true, uriBase + "/passme/"); },
    function test_network_private_forbidden() {
      test_single_loadGroup(false, false, uriBase + "/failme/"); },

    
    function test_network_private_allowed_doc_load() {
      test_single_loadGroup_doc_load(true, true, uriBase + "/passme/"); },
    function test_network_private_forbidden_doc_load() {
      
      test_single_loadGroup_doc_load(false, true, uriBase + "/passme/"); },

    
    function test_private_cache_same_network_private_allowed() {
      test_private_cached_entry_same_network(true); },
    function test_private_cache_same_network_private_forbidden() {
      test_private_cached_entry_same_network(false); },

    
    function test_private_cache_same_network_private_allowed_doc_load() {
      test_private_cached_entry_same_network_doc_load(true); },
    function test_private_cache_same_network_private_forbidden_doc_load() {
      test_private_cached_entry_same_network_doc_load(false); },

    
    function test_private_cache_diff_network_private_allowed() {
      test_private_cached_entry_diff_network(true); },
    function test_private_cache_diff_network_private_forbidden() {
      test_private_cached_entry_diff_network(false); },

    
    function test_private_cache_diff_network_private_allowed_doc_load() {
      test_private_cached_entry_diff_network_doc_load(true); },
    function test_private_cache_diff_network_private_forbidden_doc_load() {
      test_private_cached_entry_diff_network_doc_load(false); },

    
    function test_public_cache_private_allowed() {
      test_public_cached_entry(true); },
    function test_public_cache_private_forbidden() {
      test_public_cached_entry(false); },

    
    function test_public_cache_private_allowed_doc_load() {
      test_public_cached_entry_doc_load(true); },
    function test_public_cache_private_forbidden_doc_load() {
      test_public_cached_entry_doc_load(false); }
  ];

  tests.forEach(function(test) {
    add_test(test);
  });

  do_print("Added tests.");
}

function run_test() {
  setup_http_server();

  setup_and_add_tests();

  run_next_test();
}
