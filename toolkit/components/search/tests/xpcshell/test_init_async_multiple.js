












function run_test() {
  do_print("Setting up test");

  do_test_pending();
  updateAppInfo();

  do_print("Test starting");
  let numberOfInitializers = 4;
  let pending = [];
  let numberPending = numberOfInitializers;

  
  for (let i = 0; i < numberOfInitializers; ++i) {
    let me = i;
    pending[me] = true;
    Services.search.init(function search_initialized_0(aStatus) {
      do_check_true(Components.isSuccessCode(aStatus));
      init_complete(me);
    });
  }

  
  let init_complete = function init_complete(i) {
    do_check_true(pending[i]);
    pending[i] = false;
    numberPending--;
    do_check_true(numberPending >= 0);
    do_check_true(Services.search.isInitialized);
    if (numberPending == 0) {
      
      let engines = Services.search.getEngines();
      do_check_neq(engines, null);

      
      
      do_timeout(1000, function() {
        do_test_finished();
      });
    }
  };
}

