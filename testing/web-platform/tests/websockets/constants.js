
var DOMAIN_FOR_WS_TESTS = "{{host}}";
var DOMAIN_FOR_WSS_TESTS = "{{host}}";

var PORT = "{{ports[ws][0]}}";

var PORT_SSL = "{{ports[ws][0]}}";


var SCHEME_AND_DOMAIN;
var SCHEME_DOMAIN_PORT;
if (location.search == '?wss') {
  SCHEME_AND_DOMAIN = 'wss://'+DOMAIN_FOR_WSS_TESTS;
  SCHEME_DOMAIN_PORT = SCHEME_AND_DOMAIN + ":" + PORT_SSL;
} else {
  SCHEME_AND_DOMAIN = 'ws://'+DOMAIN_FOR_WS_TESTS;
  SCHEME_DOMAIN_PORT = SCHEME_AND_DOMAIN + ":" + PORT;
}
