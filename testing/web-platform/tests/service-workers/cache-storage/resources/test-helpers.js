(function() {
  var next_cache_index = 1;

  
  
  function create_temporary_cache(test) {
    var uniquifier = String(++next_cache_index);
    var cache_name = self.location.pathname + '/' + uniquifier;

    test.add_cleanup(function() {
        self.caches.delete(cache_name);
      });

    return self.caches.delete(cache_name)
      .then(function() {
          return self.caches.open(cache_name);
        });
  }

  self.create_temporary_cache = create_temporary_cache;
})();










function cache_test(test_function, description) {
  promise_test(function(test) {
      return create_temporary_cache(test)
        .then(test_function);
    }, description);
}
