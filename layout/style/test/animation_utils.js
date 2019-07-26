function px_to_num(str)
{
    return Number(String(str).match(/^([\d.]+)px$/)[1]);
}

function bezier(x1, y1, x2, y2) {
    
    function x_for_t(t) {
        var omt = 1-t;
        return 3 * omt * omt * t * x1 + 3 * omt * t * t * x2 + t * t * t;
    }
    function y_for_t(t) {
        var omt = 1-t;
        return 3 * omt * omt * t * y1 + 3 * omt * t * t * y2 + t * t * t;
    }
    function t_for_x(x) {
        
        var mint = 0, maxt = 1;
        for (var i = 0; i < 30; ++i) {
            var guesst = (mint + maxt) / 2;
            var guessx = x_for_t(guesst);
            if (x < guessx)
                maxt = guesst;
            else
                mint = guesst;
        }
        return (mint + maxt) / 2;
    }
    return function bezier_closure(x) {
        if (x == 0) return 0;
        if (x == 1) return 1;
        return y_for_t(t_for_x(x));
    }
}

function step_end(nsteps) {
    return function step_end_closure(x) {
        return Math.floor(x * nsteps) / nsteps;
    }
}

function step_start(nsteps) {
    var stepend = step_end(nsteps);
    return function step_start_closure(x) {
        return 1.0 - stepend(1.0 - x);
    }
}

var gTF = {
  "ease": bezier(0.25, 0.1, 0.25, 1),
  "linear": function(x) { return x; },
  "ease_in": bezier(0.42, 0, 1, 1),
  "ease_out": bezier(0, 0, 0.58, 1),
  "ease_in_out": bezier(0.42, 0, 0.58, 1),
  "step_start": step_start(1),
  "step_end": step_end(1),
};

function is_approx(float1, float2, error, desc) {
  ok(Math.abs(float1 - float2) < error,
     desc + ": " + float1 + " and " + float2 + " should be within " + error);
}












function runOMTATest(aTestFunction, aOnSkip) {
  const OMTAPrefKey = "layers.offmainthreadcomposition.async-animations";
  var utils      = SpecialPowers.DOMWindowUtils;
  var expectOMTA = utils.layerManagerRemote &&
                   
                   
                   SpecialPowers.getBoolPref(OMTAPrefKey);

  isOMTAWorking().then(function(isWorking) {
    if (expectOMTA) {
      if (isWorking) {
        aTestFunction();
      } else {
        
        
        
        ok(isWorking, "OMTA is working as expected");
        aOnSkip();
      }
    } else {
      todo(isWorking, "OMTA is working");
      aOnSkip();
    }
  }).catch(function(err) {
    ok(false, err);
    aOnSkip();
  });

  function isOMTAWorking() {
    
    const animationName = "a6ce3091ed85"; 
    var ruleText = "@keyframes " + animationName +
                   " { from { opacity: 0.5 } to { opacity 0.5 } }";
    var style = document.createElement("style");
    style.appendChild(document.createTextNode(ruleText));
    document.head.appendChild(style);

    
    var div = document.createElement("div");
    document.body.appendChild(div);

    
    div.style.width  = "100px";
    div.style.height = "100px";
    div.style.backgroundColor = "white";

    
    var cleanUp = function() {
      div.parentNode.removeChild(div);
      style.parentNode.removeChild(style);
      if (utils.isTestControllingRefreshes) {
        utils.restoreNormalRefresh();
      }
    };

    return waitForDocumentLoad()
      .then(loadPaintListener)
      .then(function() {
        
        utils.advanceTimeAndRefresh(0);
        div.style.animation = animationName + " 10s";

        
        div.clientTop;
        return waitForPaints();
      }).then(function() {
        var opacity = utils.getOMTAStyle(div, "opacity");
        cleanUp();
        return Promise.resolve(opacity == 0.5);
      }).catch(function(err) {
        cleanUp();
        return Promise.reject(err);
      });
  }

  function waitForDocumentLoad() {
    return new Promise(function(resolve, reject) {
      if (document.readyState === "complete") {
        resolve();
      } else {
        window.addEventListener("load", resolve);
      }
    });
  }

  function waitForPaints() {
    return new Promise(function(resolve, reject) {
      waitForAllPaintsFlushed(resolve);
    });
  }

  function loadPaintListener() {
    return new Promise(function(resolve, reject) {
      if (typeof(window.waitForAllPaints) !== "function") {
        var script = document.createElement("script");
        script.onload = resolve;
        script.onerror = function() {
          reject(new Error("Failed to load paint listener"));
        };
        script.src = "/tests/SimpleTest/paint_listener.js";
        var firstScript = document.scripts[0];
        firstScript.parentNode.insertBefore(script, firstScript);
      } else {
        resolve();
      }
    });
  }
}












(function() {
  var tests = [];

  window.addAsyncAnimTest = function(generator) {
    tests.push(generator);
  };

  
  window.runAllAsyncAnimTests = function(aOnAbort) {
    
    
    return tests.reduce(function(sequence, test) {
        return sequence.then(function() {
          return runAsyncAnimTest(test, aOnAbort);
        });
      }, Promise.resolve() );
  };

  
  
  
  
  
  
  
  
  
  
  
  function runAsyncAnimTest(aTestFunc, aOnAbort) {
    var generator;

    function step(arg) {
      var next;
      try {
        next = generator.next(arg);
      } catch (e) {
        return Promise.reject(e);
      }
      if (next.done) {
        return Promise.resolve(next.value);
      } else {
        return Promise.resolve(next.value)
               .then(step, function(err) { throw err; });
      }
    }

    
    SpecialPowers.DOMWindowUtils.advanceTimeAndRefresh(0);

    
    generator = aTestFunc();
    return step()
    .catch(function(err) {
      ok(false, err.message);
      if (typeof aOnAbort == "function") {
        aOnAbort();
      }
    }).then(function() {
      
      SpecialPowers.DOMWindowUtils.restoreNormalRefresh();
    });
  }
})();
