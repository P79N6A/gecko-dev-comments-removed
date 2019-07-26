


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head.js";

function testGetNeighboringCellIds() {
  log("Test getting neighboring cell ids");
  let deferred = Promise.defer();

  radioInterface.getNeighboringCellIds({
    notifyGetNeighboringCellIds: function(aResult) {
      deferred.resolve(aResult);
    },
    notifyGetNeighboringCellIdsFailed: function(aError) {
      deferred.reject(aError);
    }
  });
  return deferred.promise;
}


startTestBase(function() {
  
  
  
  return testGetNeighboringCellIds()
    .then(function resolve(aResult) {
      ok(false, "getNeighboringCellIds should not success");
    }, function reject(aError) {
      is(aError, "RequestNotSupported", "failed to getNeighboringCellIds");
    });
});
