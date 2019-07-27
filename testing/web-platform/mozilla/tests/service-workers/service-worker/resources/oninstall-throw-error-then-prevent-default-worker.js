


self.addEventListener('error', function(event) {});
self.addEventListener('error', function(event) { event.preventDefault(); });
self.addEventListener('error', function(event) {});
self.addEventListener('install', function(event) { throw new Error(); });
