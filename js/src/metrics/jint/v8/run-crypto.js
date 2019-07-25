



























load('base.js');
load('crypto.js');

function PrintResult(name, result) {
  print(name + ': ' + result);
}


function PrintScore(score) {
  print('----');
  print('Score: ' + score);
}


BenchmarkSuite.RunSuites({ NotifyResult: PrintResult,
                           NotifyScore: PrintScore });
