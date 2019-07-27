self.addEventListener('fetch', function(event) {
    var url = event.request.url;
    if (url.indexOf('dummy?test') == -1) {
      return;
    }
    event.respondWith(new Promise(function(resolve) {
        var headers = new Headers;
        
        resolve(new Response(new Blob([],{type: 'a\0b'})));
      }));
  });
