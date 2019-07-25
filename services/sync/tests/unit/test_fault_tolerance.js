function run_test() {
  Components.utils.import("resource://weave/faultTolerance.js");

  
  FaultTolerance.Service._testProperty = "hi";
  do_check_eq(FaultTolerance.Service._testProperty, "hi");
}
