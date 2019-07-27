function get_host_info() {
  var ORIGINAL_HOST = '127.0.0.1';
  var REMOTE_HOST = 'localhost';
  var UNAUTHENTICATED_HOST = 'example.test';
  var HTTP_PORT = 8000;
  var HTTPS_PORT = 8443;
  try {
    
    
    
    HTTP_PORT = eval('{{ports[http][0]}}');
    HTTPS_PORT = eval('{{ports[https][0]}}');
    ORIGINAL_HOST = eval('\'{{host}}\'');
    REMOTE_HOST = 'www1.' + ORIGINAL_HOST;
  } catch (e) {
  }
  return {
    HTTP_ORIGIN: 'http://' + ORIGINAL_HOST + ':' + HTTP_PORT,
    HTTPS_ORIGIN: 'https://' + ORIGINAL_HOST + ':' + HTTPS_PORT,
    HTTP_REMOTE_ORIGIN: 'http://' + REMOTE_HOST + ':' + HTTP_PORT,
    HTTPS_REMOTE_ORIGIN: 'https://' + REMOTE_HOST + ':' + HTTPS_PORT,
    UNAUTHENTICATED_ORIGIN: 'http://' + UNAUTHENTICATED_HOST + ':' + HTTP_PORT
  };
}
