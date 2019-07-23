

































function Benchmark(name, run) {
  this.name = name;
  this.run = run;
}





function BenchmarkResult(benchmark, time) {
  this.benchmark = benchmark;
  this.time = time;
}




BenchmarkResult.prototype.valueOf = function() {
  return this.time;
}






function BenchmarkSuite(name, reference, benchmarks) {
  this.name = name;
  this.reference = reference;
  this.benchmarks = benchmarks;
  BenchmarkSuite.suites.push(this);
}



BenchmarkSuite.suites = [];





BenchmarkSuite.version = '1';






BenchmarkSuite.RunSuites = function(runner) {
  var continuation = null;
  var suites = BenchmarkSuite.suites;
  var length = suites.length;
  BenchmarkSuite.scores = [];
  var index = 0;
  function RunStep() {
    while (continuation || index < length) {
      if (continuation) {
        continuation = continuation();
      } else {
        var suite = suites[index++];
        if (runner.NotifyStart) runner.NotifyStart(suite.name);
        continuation = suite.RunStep(runner);
      }
      if (continuation && typeof window != 'undefined' && window.setTimeout) {
        window.setTimeout(RunStep, 100);
        return;
      }
    }
    if (runner.NotifyScore) {
      var score = BenchmarkSuite.GeometricMean(BenchmarkSuite.scores);
      runner.NotifyScore(Math.round(100 * score));
    }
  }
  RunStep();
}




BenchmarkSuite.CountBenchmarks = function() {
  var result = 0;
  var suites = BenchmarkSuite.suites;
  for (var i = 0; i < suites.length; i++) {
    result += suites[i].benchmarks.length;
  }
  return result;
}



BenchmarkSuite.GeometricMean = function(numbers) {
  var log = 0;
  for (var i = 0; i < numbers.length; i++) {
    log += Math.log(numbers[i]);
  }
  return Math.pow(Math.E, log / numbers.length);
}




BenchmarkSuite.prototype.NotifyStep = function(result) {
  this.results.push(result);
  if (this.runner.NotifyStep) this.runner.NotifyStep(result.benchmark.name);
}




BenchmarkSuite.prototype.NotifyResult = function() {
  var mean = BenchmarkSuite.GeometricMean(this.results);
  var score = this.reference / mean;
  BenchmarkSuite.scores.push(score);
  if (this.runner.NotifyResult) {
    this.runner.NotifyResult(this.name, Math.round(100 * score));
  }
}




BenchmarkSuite.prototype.RunSingle = function(benchmark) {
  var elapsed = 0;
  var start = new Date();
  for (var n = 0; elapsed < 1000; n++) {
    benchmark.run();
    elapsed = new Date() - start;
  }
  var usec = (elapsed * 1000) / n;
  this.NotifyStep(new BenchmarkResult(benchmark, usec));
}






BenchmarkSuite.prototype.RunStep = function(runner) {
  this.results = [];
  this.runner = runner;
  var length = this.benchmarks.length;
  var index = 0;
  var suite = this;
  function RunNext() {
    if (index < length) {
      suite.RunSingle(suite.benchmarks[index++]);
      return RunNext;
    }
    suite.NotifyResult();
    return null;
  }
  return RunNext();
}
