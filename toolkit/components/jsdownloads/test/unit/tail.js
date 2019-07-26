








"use strict";




add_task(function test_common_terminate()
{
  
  
  let deferred = Promise.defer();
  gHttpServer.stop(deferred.resolve);
  yield deferred.promise;
});
