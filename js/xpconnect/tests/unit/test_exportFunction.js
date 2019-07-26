function run_test() {
  var Cu = Components.utils;
  var epsb = new Cu.Sandbox(["http://example.com", "http://example.org"], { wantExportHelpers: true });
  subsb = new Cu.Sandbox("http://example.com", { wantGlobalProperties: ["XMLHttpRequest"] });
  subsb2 = new Cu.Sandbox("http://example.com", { wantGlobalProperties: ["XMLHttpRequest"] });
  xorigsb = new Cu.Sandbox("http://test.com");

  epsb.subsb = subsb;
  epsb.xorigsb = xorigsb;
  epsb.do_check_true = do_check_true;
  epsb.do_check_eq = do_check_eq;
  epsb.do_check_neq = do_check_neq;

  
  
  Cu.evalInSandbox("(" + function() {
    Object.prototype.protoProp = "common";
    var wasCalled = false;
    var _this = this;
    this.funToExport = function(a, obj, native, mixed) {
      do_check_eq(a, 42);
      do_check_neq(obj, subsb.tobecloned);
      do_check_eq(obj.cloned, "cloned");
      do_check_eq(obj.protoProp, "common");
      do_check_eq(native, subsb.native);
      do_check_eq(_this, this);
      do_check_eq(mixed.xrayed, subsb.xrayed);
      do_check_eq(mixed.xrayed2, subsb.xrayed2);
      wasCalled = true;
    };
    this.checkIfCalled = function() {
      do_check_true(wasCalled);
      wasCalled = false;
    }
    exportFunction(funToExport, subsb, "imported");
  }.toSource() + ")()", epsb);

  subsb.xrayed = Cu.evalInSandbox("(" + function () {
      return new XMLHttpRequest();
  }.toSource() + ")()", subsb2);

  
  
  
  Cu.evalInSandbox("(" + function () {
    native = new XMLHttpRequest();
    xrayed2 = XPCNativeWrapper(new XMLHttpRequest());
    mixed = { xrayed: xrayed, xrayed2: xrayed2 };
    tobecloned = { cloned: "cloned" };
    imported(42,tobecloned, native, mixed);
  }.toSource() + ")()", subsb);

  
  
  Cu.evalInSandbox("(" + function() {
    imported.apply("something", [42, tobecloned, native, mixed]);
  }.toSource() + ")()", subsb);

  Cu.evalInSandbox("(" + function() {
    checkIfCalled();
  }.toSource() + ")()", epsb);

  
  
  Cu.evalInSandbox("(" + function() {
    try{
      exportFunction(function(){}, this.xorigsb, "denied");
      do_check_true(false);
    } catch (e) {
      do_check_true(e.toString().indexOf('Permission denied') > -1);
    }
  }.toSource() + ")()", epsb);

  
  
  Cu.evalInSandbox("(" + function() {
    var newContentObject = createObjectIn(subsb, {defineAs:"importedObject"});
    exportFunction(funToExport, newContentObject, "privMethod");
  }.toSource() + ")()", epsb);

  Cu.evalInSandbox("(" + function () {
    importedObject.privMethod(42, tobecloned, native, mixed);
  }.toSource() + ")()", subsb);

  Cu.evalInSandbox("(" + function() {
    checkIfCalled();
  }.toSource() + ")()", epsb);
}
