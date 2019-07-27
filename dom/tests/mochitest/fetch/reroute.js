onfetch = function(e) {
  if (e.request.url.indexOf("Referer") >= 0) {
    
    
    var url = e.request.url.substring(0, e.request.url.indexOf('?'));
    url += '?headers=' + ({ 'Referer': self.location.href }).toSource();

    e.respondWith(fetch(url, {
      method: e.request.method,
      headers: e.request.headers,
      body: e.request.body,
      mode: e.request.mode,
      credentials: e.request.credentials,
      cache: e.request.cache,
    }));
    return;
  }
  e.respondWith(fetch(e.request));
};
