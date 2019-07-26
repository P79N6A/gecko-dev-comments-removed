



const Cu = Components.utils;

function run_test() {
  var sb = new Cu.Sandbox('http://www.example.com', { wantComponents: true } );
  do_check_false(Cu.evalInSandbox('"Components" in this', sb));
}
