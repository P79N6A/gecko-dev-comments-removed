

var running = false;

onmessage = function (event) {
  
  if (running == false) {
    running = true;
    run(1);
  } else {
    running = false;
  }
};

function run(n) {
  
  var limit = n + 20000;
  search: while (running && n < limit) {
    n += 1;
    for (var i = 2; i <= Math.sqrt(n); i += 1) {
      if (n % i == 0) {
        continue search;
      }
    }
    
    postMessage(n);
  }
  if (n === limit) {
    
    setTimeout(function(start_time) {
      
      run(n);
    }, 150);
  }
}