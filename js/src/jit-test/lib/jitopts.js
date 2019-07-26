




function jitTogglesMatch(opts) {
  var currentOpts = getJitCompilerOptions();
  for (var k in opts) {
    if (k.indexOf(".enable") > 0 && opts[k] != currentOpts[k])
      return false;
  }
  return true;
}


function withJitOptions(opts, fn) {
  var oldOpts = getJitCompilerOptions();
  for (var k in opts)
    setJitCompilerOption(k, opts[k]);
  try {
    fn();
  } finally {
    for (var k in oldOpts)
      setJitCompilerOption(k, oldOpts[k]);
  }
}





var Opts_BaselineEager =
    {
      'ion.enable': 1,
      'baseline.enable': 1,
      'baseline.usecount.trigger': 0,
      'parallel-compilation.enable': 1
    };





var Opts_IonEagerNoParallelCompilation =
    {
      'ion.enable': 1,
      'ion.usecount.trigger': 0,
      'baseline.enable': 1,
      'baseline.usecount.trigger': 0,
      'parallel-compilation.enable': 0,
    };

var Opts_Ion2NoParallelCompilation =
    {
      'ion.enable': 1,
      'ion.usecount.trigger': 2,
      'baseline.enable': 1,
      'baseline.usecount.trigger': 1,
      'parallel-compilation.enable': 0
    };

var Opts_NoJits =
    {
      'ion.enable': 0,
      'ion.usecount.trigger': 0,
      'baseline.usecount.trigger': 0,
      'baseline.enable': 0,
      'parallel-compilation.enable': 0
    };
