


self.addEventListener('error', function(event) {});
self.addEventListener('error', function(event) { event.preventDefault(); });
self.addEventListener('error', function(event) {});
self.addEventListener('activate', function(event) { throw new Error(); });
