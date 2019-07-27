self.addEventListener("fetch", function(event) {
  if (event.request.url.indexOf("index.html") >= 0 ||
      event.request.url.indexOf("register.html") >= 0 ||
      event.request.url.indexOf("unregister.html") >= 0 ||
      event.request.url.indexOf("csp-violate.sjs") >= 0) {
    
    event.respondWith(fetch(event.request));
  } else if (event.request.url.indexOf("fetch.txt") >= 0) {
    var body = event.request.context == "fetch" ?
               "so fetch" : "so unfetch";
    event.respondWith(new Response(body));
  } else if (event.request.url.indexOf("img.jpg") >= 0) {
    if (event.request.context == "image") {
      event.respondWith(fetch("realimg.jpg"));
    }
  } else if (event.request.url.indexOf("responsive.jpg") >= 0) {
    if (event.request.context == "imageset") {
      event.respondWith(fetch("realimg.jpg"));
    }
  } else if (event.request.url.indexOf("audio.ogg") >= 0) {
    if (event.request.context == "audio") {
      event.respondWith(fetch("realaudio.ogg"));
    }
  } else if (event.request.url.indexOf("video.ogg") >= 0) {
    
    if (event.request.context == "audio") {
      event.respondWith(fetch("realaudio.ogg"));
    }
  } else if (event.request.url.indexOf("beacon.sjs") >= 0) {
    if (event.request.url.indexOf("queryContext") == -1) {
      event.respondWith(fetch("beacon.sjs?" + event.request.context));
    } else {
      event.respondWith(fetch(event.request));
    }
  } else if (event.request.url.indexOf("csp-report.sjs") >= 0) {
    event.respondWith(clients.matchAll()
                      .then(function(clients) {
                        clients.forEach(function(c) {
                          c.postMessage({data: "csp-report", context: event.request.context});
                        });
                        return new Response("ack");
                      }));
  }
  
  try {
    event.respondWith(Promise.reject(event.request.url));
  } catch(e) {
    
    
  }
});
