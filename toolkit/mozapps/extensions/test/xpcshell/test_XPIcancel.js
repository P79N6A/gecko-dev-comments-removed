





let scope = Components.utils.import("resource://gre/modules/XPIProvider.jsm");
let XPIProvider = scope.XPIProvider;

function run_test() {
  
  XPIProvider.cancelAll();

  
  let getsCancelled = {
    isCancelled: false,
    cancel: function () {
      if (this.isCancelled)
        do_throw("Already cancelled");
      this.isCancelled = true;
    }
  };
  XPIProvider.doing(getsCancelled);
  XPIProvider.cancelAll();
  do_check_true(getsCancelled.isCancelled);

  
  let doesntGetCancelled = {
    cancel: () => do_throw("This should not have been cancelled")
  };
  XPIProvider.doing(doesntGetCancelled);
  do_check_true(XPIProvider.done(doesntGetCancelled));
  XPIProvider.cancelAll();

  
  getsCancelled.isCancelled = false;
  let addsAnother = {
    isCancelled: false,
    cancel: function () {
      if (this.isCancelled)
        do_throw("Already cancelled");
      this.isCancelled = true;
      XPIProvider.doing(getsCancelled);
    }
  }
  XPIProvider.doing(addsAnother);
  XPIProvider.cancelAll();
  do_check_true(addsAnother.isCancelled);
  do_check_true(getsCancelled.isCancelled);

  
  
  let removesAnother = {
    isCancelled: false,
    cancel: function () {
      if (this.isCancelled)
        do_throw("Already cancelled");
      this.isCancelled = true;
      XPIProvider.done(doesntGetCancelled);
    }
  }
  XPIProvider.doing(removesAnother);
  XPIProvider.doing(doesntGetCancelled);
  XPIProvider.cancelAll();
  do_check_true(removesAnother.isCancelled);
}
