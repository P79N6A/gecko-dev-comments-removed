
















var ios = Components.classes["@mozilla.org/network/io-service;1"]
                    .getService(Components.interfaces.nsIIOService);
var pps = Components.classes["@mozilla.org/network/protocol-proxy-service;1"]
                    .getService();
var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                     .getService(Components.interfaces.nsIPrefBranch);





function TestProtocolHandler() {
}
TestProtocolHandler.prototype = {
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIProtocolHandler) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  scheme: "moz-test",
  defaultPort: -1,
  protocolFlags: Components.interfaces.nsIProtocolHandler.URI_NOAUTH |
                 Components.interfaces.nsIProtocolHandler.URI_NORELATIVE |
                 Components.interfaces.nsIProtocolHandler.ALLOWS_PROXY |
                 Components.interfaces.nsIProtocolHandler.URI_DANGEROUS_TO_LOAD,
  newURI: function(spec, originCharset, baseURI) {
    var uri = Components.classes["@mozilla.org/network/simple-uri;1"]
                        .createInstance(Components.interfaces.nsIURI);
    uri.spec = spec;
    return uri;
  },
  newChannel: function(uri) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  allowPort: function(port, scheme) {
    return true;
  }
};

function TestProtocolHandlerFactory() {
}
TestProtocolHandlerFactory.prototype = {
  createInstance: function(delegate, iid) {
    return new TestProtocolHandler().QueryInterface(iid);
  },
  lockFactory: function(lock) {
  }
};

function register_test_protocol_handler() {
  var reg = Components.manager.QueryInterface(
      Components.interfaces.nsIComponentRegistrar);
  reg.registerFactory(Components.ID("{4ea7dd3a-8cae-499c-9f18-e1de773ca25b}"),
                      "TestProtocolHandler",
                      "@mozilla.org/network/protocol;1?name=moz-test",
                      new TestProtocolHandlerFactory());
}

function check_proxy(pi, type, host, port, flags, timeout, hasNext) {
  do_check_neq(pi, null);
  do_check_eq(pi.type, type);
  do_check_eq(pi.host, host);
  do_check_eq(pi.port, port);
  if (flags != -1)
    do_check_eq(pi.flags, flags);
  if (timeout != -1)
    do_check_eq(pi.failoverTimeout, timeout);
  if (hasNext)
    do_check_neq(pi.failoverProxy, null);
  else
    do_check_eq(pi.failoverProxy, null);
}

function TestFilter(type, host, port, flags, timeout) {
  this._type = type;
  this._host = host;
  this._port = port;
  this._flags = flags;
  this._timeout = timeout;
}
TestFilter.prototype = {
  _type: "",
  _host: "",
  _port: -1,
  _flags: 0,
  _timeout: 0,
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIProtocolProxyFilter) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  applyFilter: function(pps, uri, pi) {
    var pi_tail = pps.newProxyInfo(this._type, this._host, this._port,
                                   this._flags, this._timeout, null);
    if (pi)
      pi.failoverProxy = pi_tail;
    else
      pi = pi_tail;
    return pi;
  }
};

function BasicFilter() {}
BasicFilter.prototype = {
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIProtocolProxyFilter) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  applyFilter: function(pps, uri, pi) {
    return pps.newProxyInfo("http", "localhost", 8080, 0, 10,
           pps.newProxyInfo("direct", "", -1, 0, 0, null));
  }
};

function resolveCallback() { }
resolveCallback.prototype = {
  nextFunction: null,

  QueryInterface : function (iid) {
    const interfaces = [Components.interfaces.nsIProtocolProxyCallback,
                        Components.interfaces..nsISupports];
    if (!interfaces.some( function(v) { return iid.equals(v) } ))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },

  onProxyAvailable : function (req, uri, pi, status) {
    this.nextFunction(pi);
  }
};

function run_filter_test() {
  var uri = ios.newURI("http://www.mozilla.org/", null, null);

  
  var cb = new resolveCallback();
  cb.nextFunction = filter_test0_1;
  var req = pps.asyncResolve(uri, 0, cb);
}

var filter01;
var filter02;

function filter_test0_1(pi) {
  do_check_eq(pi, null);

  

  filter01 = new BasicFilter();
  filter02 = new BasicFilter();
  pps.registerFilter(filter01, 10);
  pps.registerFilter(filter02, 20);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test0_2;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test0_2(pi)
{
  check_proxy(pi, "http", "localhost", 8080, 0, 10, true);
  check_proxy(pi.failoverProxy, "direct", "", -1, 0, 0, false);

  pps.unregisterFilter(filter02);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test0_3;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test0_3(pi)
{
  check_proxy(pi, "http", "localhost", 8080, 0, 10, true);
  check_proxy(pi.failoverProxy, "direct", "", -1, 0, 0, false);

  

  pps.unregisterFilter(filter01);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test0_4;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test0_4(pi)
{
  do_check_eq(pi, null);
  run_filter_test2();
}

var filter11;
var filter12;

function run_filter_test2() {
  

  filter11 = new TestFilter("http", "foo", 8080, 0, 10);
  filter12 = new TestFilter("http", "bar", 8090, 0, 10);
  pps.registerFilter(filter11, 20);
  pps.registerFilter(filter12, 10);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test1_1;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test1_1(pi) {
  check_proxy(pi, "http", "bar", 8090, 0, 10, true);
  check_proxy(pi.failoverProxy, "http", "foo", 8080, 0, 10, false);

  pps.unregisterFilter(filter12);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test1_2;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test1_2(pi) {
  check_proxy(pi, "http", "foo", 8080, 0, 10, false);

  

  pps.unregisterFilter(filter11);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test1_3;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test1_3(pi) {
  do_check_eq(pi, null);
  run_filter_test3();
}

var filter_3_1;

function run_filter_test3() {
  var uri = ios.newURI("http://www.mozilla.org/", null, null);

  

  filter_3_1 = new TestFilter("http", "foo", 8080, 0, 10);
  pps.registerFilter(filter_3_1, 20);

  var cb = new resolveCallback();
  cb.nextFunction = filter_test3_1;
  var req = pps.asyncResolve(uri, 0, cb);
}

function filter_test3_1(pi) {
  check_proxy(pi, "http", "foo", 8080, 0, 10, false);
  pps.unregisterFilter(filter_3_1);
  run_pref_test();
}

function run_pref_test() {
  var uri = ios.newURI("http://www.mozilla.org/", null, null);

  

  prefs.setIntPref("network.proxy.type", 0);

  var cb = new resolveCallback();
  cb.nextFunction = pref_test1_1;
  var req = pps.asyncResolve(uri, 0, cb);
}

function pref_test1_1(pi)
{
  do_check_eq(pi, null);

  
  prefs.setIntPref("network.proxy.type", 1);

  var cb = new resolveCallback();
  cb.nextFunction = pref_test1_2;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function pref_test1_2(pi)
{
  
  do_check_eq(pi, null);

  
  prefs.setCharPref("network.proxy.http", "foopy");
  prefs.setIntPref("network.proxy.http_port", 8080);

  var cb = new resolveCallback();
  cb.nextFunction = pref_test1_3;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function pref_test1_3(pi)
{
  check_proxy(pi, "http", "foopy", 8080, 0, -1, false);

  prefs.setCharPref("network.proxy.http", "");
  prefs.setIntPref("network.proxy.http_port", 0);

  
  prefs.setCharPref("network.proxy.socks", "barbar");
  prefs.setIntPref("network.proxy.socks_port", 1203);

  var cb = new resolveCallback();
  cb.nextFunction = pref_test1_4;
  var uri = ios.newURI("http://www.mozilla.org/", null, null);
  var req = pps.asyncResolve(uri, 0, cb);
}

function pref_test1_4(pi)
{
  check_proxy(pi, "socks", "barbar", 1203, 0, -1, false);
  run_pac_test();
}

function run_protocol_handler_test() {
  var uri = ios.newURI("moz-test:foopy", null, null);

  var cb = new resolveCallback();
  cb.nextFunction = protocol_handler_test_1;
  var req = pps.asyncResolve(uri, 0, cb);
}

function protocol_handler_test_1(pi)
{
  do_check_eq(pi, null);
  prefs.setCharPref("network.proxy.autoconfig_url", "");
  prefs.setIntPref("network.proxy.type", 0);

  run_pac_cancel_test();
}

function TestResolveCallback() {
}
TestResolveCallback.prototype = {
  QueryInterface:
  function TestResolveCallback_QueryInterface(iid) {
    if (iid.equals(Components.interfaces.nsIProtocolProxyCallback) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onProxyAvailable:
  function TestResolveCallback_onProxyAvailable(req, uri, pi, status) {
    dump("*** uri=" + uri.spec + ", status=" + status + "\n");

    do_check_neq(req, null);
    do_check_neq(uri, null);
    do_check_eq(status, 0);
    do_check_neq(pi, null);

    check_proxy(pi, "http", "foopy", 8080, 0, -1, true);
    check_proxy(pi.failoverProxy, "direct", "", -1, -1, -1, false);

    run_protocol_handler_test();
  }
};

function run_pac_test() {
  var pac = 'data:text/plain,' +
            'function FindProxyForURL(url, host) {' +
            '  return "PROXY foopy:8080; DIRECT";' +
            '}';
  var uri = ios.newURI("http://www.mozilla.org/", null, null);

  

  prefs.setIntPref("network.proxy.type", 2);
  prefs.setCharPref("network.proxy.autoconfig_url", pac);

  var req = pps.asyncResolve(uri, 0, new TestResolveCallback());
}

function TestResolveCancelationCallback() {
}
TestResolveCancelationCallback.prototype = {
  QueryInterface:
  function TestResolveCallback_QueryInterface(iid) {
    if (iid.equals(Components.interfaces.nsIProtocolProxyCallback) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onProxyAvailable:
  function TestResolveCancelationCallback_onProxyAvailable(req, uri, pi, status) {
    dump("*** uri=" + uri.spec + ", status=" + status + "\n");

    do_check_neq(req, null);
    do_check_neq(uri, null);
    do_check_eq(status, Components.results.NS_ERROR_ABORT);
    do_check_eq(pi, null);

    prefs.setCharPref("network.proxy.autoconfig_url", "");
    prefs.setIntPref("network.proxy.type", 0);

    run_proxy_host_filters_test();
  }
};

function run_pac_cancel_test() {
  var uri = ios.newURI("http://www.mozilla.org/", null, null);

  
  var pac = 'data:text/plain,' +
            'function FindProxyForURL(url, host) {' +
            '  return "PROXY foopy:8080; DIRECT";' +
            '}';
  prefs.setIntPref("network.proxy.type", 2);
  prefs.setCharPref("network.proxy.autoconfig_url", pac);

  var req = pps.asyncResolve(uri, 0, new TestResolveCancelationCallback());
  req.cancel(Components.results.NS_ERROR_ABORT);
}

var hostList;
var hostIDX;
var bShouldBeFiltered;
var hostNextFX;

function check_host_filters(hl, shouldBe, nextFX) {
  hostList = hl;
  hostIDX = 0;
  bShouldBeFiltered = shouldBe;
  hostNextFX = nextFX;

  if (hostList.length > hostIDX)
    check_host_filter(hostIDX);
}

function check_host_filters_cb()
{
  hostIDX++;
  if (hostList.length > hostIDX)
    check_host_filter(hostIDX);
  else
    hostNextFX();
}

function check_host_filter(i) {
  var uri;
  dump("*** uri=" + hostList[i] + " bShouldBeFiltered=" + bShouldBeFiltered + "\n");
  uri = ios.newURI(hostList[i], null, null);

  var cb = new resolveCallback();
  cb.nextFunction = host_filter_cb;
  var req = pps.asyncResolve(uri, 0, cb);
}

function host_filter_cb(proxy)
{
  if (bShouldBeFiltered) {
    do_check_eq(proxy, null);
  } else {
    do_check_neq(proxy, null);
    
    
    check_proxy(proxy, "http", "foopy", 8080, 0, -1, false);
  }
  check_host_filters_cb();
}





var uriStrUseProxyList;
var uriStrUseProxyList;
var hostFilterList;

function run_proxy_host_filters_test() {
  
  
  
  prefs.setIntPref("network.proxy.type", 1);
  prefs.setCharPref("network.proxy.http", "foopy");
  prefs.setIntPref("network.proxy.http_port", 8080);

  
  hostFilterList = "www.mozilla.org, www.google.com, www.apple.com, "
                       + ".domain, .domain2.org"
  prefs.setCharPref("network.proxy.no_proxies_on", hostFilterList);
  do_check_eq(prefs.getCharPref("network.proxy.no_proxies_on"), hostFilterList);

  var rv;
  
  uriStrFilterList = [ "http://www.mozilla.org/",
                           "http://www.google.com/",
                           "http://www.apple.com/",
                           "http://somehost.domain/",
                           "http://someotherhost.domain/",
                           "http://somehost.domain2.org/",
                           "http://somehost.subdomain.domain2.org/" ];
  check_host_filters(uriStrFilterList, true, host_filters_1);
}

function host_filters_1()
{
  
  uriStrUseProxyList = [ "http://www.mozilla.com/",
                             "http://mail.google.com/",
                             "http://somehost.domain.co.uk/",
                             "http://somelocalhost/" ];  
  check_host_filters(uriStrUseProxyList, false, host_filters_2);
}

function host_filters_2()
{
  
  prefs.setCharPref("network.proxy.no_proxies_on", hostFilterList + ", <local>");
  do_check_eq(prefs.getCharPref("network.proxy.no_proxies_on"),
              hostFilterList + ", <local>");
  
  uriStrFilterList.push(uriStrUseProxyList.pop());
  check_host_filters(uriStrFilterList, true, host_filters_3);
}

function host_filters_3()
{
  check_host_filters(uriStrUseProxyList, false, host_filters_4);
}

function host_filters_4()
{
  
  prefs.setCharPref("network.proxy.no_proxies_on", "");
  do_check_eq(prefs.getCharPref("network.proxy.no_proxies_on"), "");  

  do_test_finished();
}

function run_deprecated_sync_test()
{
  var uri = ios.newURI("http://www.mozilla.org/", null, null);

  pps.QueryInterface(Components.interfaces.nsIProtocolProxyService2);

  
  var pi = pps.deprecatedBlockingResolve(uri, 0);
  do_check_eq(pi, null);

  
  var filter1 = new BasicFilter();
  var filter2 = new BasicFilter();
  pps.registerFilter(filter1, 10);
  pps.registerFilter(filter2, 20);

  pi = pps.deprecatedBlockingResolve(uri, 0);
  check_proxy(pi, "http", "localhost", 8080, 0, 10, true);
  check_proxy(pi.failoverProxy, "direct", "", -1, 0, 0, false);

  pps.unregisterFilter(filter2);
  pi = pps.deprecatedBlockingResolve(uri, 0);
  check_proxy(pi, "http", "localhost", 8080, 0, 10, true);
  check_proxy(pi.failoverProxy, "direct", "", -1, 0, 0, false);

  
  pps.unregisterFilter(filter1);
  pi = pps.deprecatedBlockingResolve(uri, 0);
  do_check_eq(pi, null);
}

function run_test() {
  register_test_protocol_handler();

  
  run_deprecated_sync_test();

  
  run_filter_test();
  do_test_pending();
}
