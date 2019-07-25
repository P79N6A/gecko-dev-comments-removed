



























load('base.js');
load('richards.js');
load('deltablue.js');
load('crypto.js');
load('raytrace.js');
load('earley-boyer.js');


function PrintResult(name, result) {
  print(name + ': ' + result);
}


function PrintScore(score) {
  print('----');
  print('Score: ' + score);
}


BenchmarkSuite.RunSuites({ NotifyResult: PrintResult,
                           NotifyScore: PrintScore });
