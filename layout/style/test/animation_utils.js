





function advance_clock(milliseconds) {
  SpecialPowers.DOMWindowUtils.advanceTimeAndRefresh(milliseconds);
}


(function() {
  var gElem;
  var gEventsReceived = [];

  function new_div(style) {
    return new_element("div", style);
  }

  
  
  
  
  function new_element(tagname, style) {
    if (gElem) {
      ok(false, "test author forgot to call done_div/done_elem");
    }
    if (typeof(style) != "string") {
      ok(false, "test author forgot to pass argument");
    }
    if (!document.getElementById("display")) {
      ok(false, "no 'display' element to append to");
    }
    gElem = document.createElement(tagname);
    gElem.setAttribute("style", style);
    gElem.classList.add("target");
    document.getElementById("display").appendChild(gElem);
    return [ gElem, getComputedStyle(gElem, "") ];
  }

  function listen() {
    if (!gElem) {
      ok(false, "test author forgot to call new_div before listen");
    }
    gEventsReceived = [];
    function listener(event) {
      gEventsReceived.push(event);
    }
    gElem.addEventListener("animationstart", listener, false);
    gElem.addEventListener("animationiteration", listener, false);
    gElem.addEventListener("animationend", listener, false);
  }

  function check_events(eventsExpected, desc) {
    
    
    
    is(gEventsReceived.length, gEventsReceived.length,
       "number of events received for " + desc);
    for (var i = 0,
         i_end = Math.min(eventsExpected.length, gEventsReceived.length);
         i != i_end; ++i) {
      var exp = eventsExpected[i];
      var rec = gEventsReceived[i];
      for (var prop in exp) {
        if (prop == "elapsedTime") {
          
          ok(Math.abs(rec.elapsedTime - exp.elapsedTime) < 0.000002,
             "events[" + i + "]." + prop + " for " + desc +
             " received=" + rec.elapsedTime + " expected=" + exp.elapsedTime);
        } else {
          is(rec[prop], exp[prop],
             "events[" + i + "]." + prop + " for " + desc);
        }
      }
    }
    for (var i = eventsExpected.length; i < gEventsReceived.length; ++i) {
      ok(false, "unexpected " + gEventsReceived[i].type + " event for " + desc);
    }
    gEventsReceived = [];
  }

  function done_element() {
    if (!gElem) {
      ok(false, "test author called done_element/done_div without matching"
                + " call to new_element/new_div");
    }
    gElem.remove();
    gElem = null;
    if (gEventsReceived.length) {
      ok(false, "caller should have called check_events");
    }
  }

  [ new_div
    , new_element
    , listen
    , check_events
    , done_element ]
  .forEach(function(fn) {
    window[fn.name] = fn;
  });
  window.done_div = done_element;
})();

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

function findKeyframesRule(name) {
  for (var i = 0; i < document.styleSheets.length; i++) {
    var match = [].find.call(document.styleSheets[i].cssRules, function(rule) {
      return rule.type == CSSRule.KEYFRAMES_RULE &&
             rule.name == name;
    });
    if (match) {
      return match;
    }
  }
  return undefined;
}

















function runOMTATest(aTestFunction, aOnSkip, specialPowersForPrefs) {
  const OMTAPrefKey = "layers.offmainthreadcomposition.async-animations";
  var utils      = SpecialPowers.DOMWindowUtils;
  if (!specialPowersForPrefs) {
      specialPowersForPrefs = SpecialPowers;
  }
  var expectOMTA = utils.layerManagerRemote &&
                   
                   
                   specialPowersForPrefs.getBoolPref(OMTAPrefKey);

  isOMTAWorking().then(function(isWorking) {
    if (expectOMTA) {
      if (isWorking) {
        aTestFunction();
      } else {
        
        
        
        ok(isWorking, "OMTA should work");
        aOnSkip();
      }
    } else {
      todo(isWorking,
           "OMTA should ideally work, though we don't expect it to work on " +
           "this platform/configuration");
      aOnSkip();
    }
  }).catch(function(err) {
    ok(false, err);
    aOnSkip();
  });

  function isOMTAWorking() {
    
    const animationName = "a6ce3091ed85"; 
    var ruleText = "@keyframes " + animationName +
                   " { from { opacity: 0.5 } to { opacity: 0.5 } }";
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







const RunningOn = {
  MainThread: 0,
  Compositor: 1,
  Either: 2,
  TodoMainThread: 3
};

const ExpectComparisonTo = {
  Pass: 1,
  Fail: 2
};

(function() {
  window.omta_todo_is = function(elem, property, expected, runningOn, desc) {
    return omta_is_approx(elem, property, expected, 0, runningOn, desc,
                          ExpectComparisonTo.Fail);
  };

  window.omta_is = function(elem, property, expected, runningOn, desc) {
    return omta_is_approx(elem, property, expected, 0, runningOn, desc);
  };

  
  
  window.omta_is_approx = function(elem, property, expected, tolerance,
                                   runningOn, desc, expectedComparisonResult) {
    
    const omtaProperties = [ "transform", "opacity" ];
    if (omtaProperties.indexOf(property) === -1) {
      ok(false, property + " is not an OMTA property");
      return;
    }
    var isTransform = property == "transform";
    var normalize = isTransform ? convertTo3dMatrix : parseFloat;
    var compare = isTransform ?
                  matricesRoughlyEqual :
                  function(a, b, error) { return Math.abs(a - b) <= error; };
    var normalizedToString = isTransform ?
                             convert3dMatrixToString :
                             JSON.stringify;

    
    var compositorStr =
      SpecialPowers.DOMWindowUtils.getOMTAStyle(elem, property);
    var computedStr = window.getComputedStyle(elem)[property];

    
    var expectedValue = normalize(expected);
    if (expectedValue === null) {
      ok(false, desc + ": test author should provide a valid 'expected' value" +
                " - got " + expected.toString());
      return;
    }

    
    var actualStr;
    switch (runningOn) {
      case RunningOn.Either:
        runningOn = compositorStr !== "" ?
                    RunningOn.Compositor :
                    RunningOn.MainThread;
        actualStr = compositorStr !== "" ? compositorStr : computedStr;
        break;

      case RunningOn.Compositor:
        if (compositorStr === "") {
          ok(false, desc + ": should be animating on compositor");
          return;
        }
        actualStr = compositorStr;
        break;

      case RunningOn.TodoMainThread:
        todo(compositorStr === "",
             desc + ": should NOT be animating on compositor");
        actualStr = compositorStr === "" ? computedStr : compositorStr;
        break;

      default:
        if (compositorStr !== "") {
          ok(false, desc + ": should NOT be animating on compositor");
          return;
        }
        actualStr = computedStr;
        break;
    }

    var okOrTodo = expectedComparisonResult == ExpectComparisonTo.Fail ?
                   todo :
                   ok;

    
    var actualValue = normalize(actualStr);
    if (actualValue === null) {
      ok(false, desc + ": should return a valid result - got " + actualStr);
      return;
    }
    okOrTodo(compare(expectedValue, actualValue, tolerance),
             desc + " - got " + actualStr + ", expected " +
             normalizedToString(expectedValue));

    
    
    if (actualStr === compositorStr) {
      var computedValue = normalize(computedStr);
      if (computedValue === null) {
        ok(false, desc + ": test framework should parse computed style" +
                  " - got " + computedStr);
        return;
      }
      okOrTodo(compare(computedValue, actualValue, 0),
               desc + ": OMTA style and computed style should be equal" +
               " - OMTA " + actualStr + ", computed " + computedStr);
    }
  };

  window.matricesRoughlyEqual = function(a, b, tolerance) {
    tolerance = tolerance || 0.00011;
    for (var i = 0; i < 4; i++) {
      for (var j = 0; j < 4; j++) {
        var diff = Math.abs(a[i][j] - b[i][j]);
        if (diff > tolerance || isNaN(diff))
          return false;
      }
    }
    return true;
  };

  
  
  
  
  
  
  
  window.convertTo3dMatrix = function(matrixLike) {
    if (typeof(matrixLike) == "string") {
      return convertStringTo3dMatrix(matrixLike);
    } else if (Array.isArray(matrixLike)) {
      return convertArrayTo3dMatrix(matrixLike);
    } else if (typeof(matrixLike) == "object") {
      return convertObjectTo3dMatrix(matrixLike);
    } else {
      return null;
    }
  };

  
  
  window.isInvertible = function(matrix) {
    return getDeterminant(matrix) != 0;
  };

  
  
  function convertStringTo3dMatrix(str) {
    if (str == "none")
      return convertArrayTo3dMatrix([1, 0, 0, 1, 0, 0]);
    var result = str.match("^matrix(3d)?\\(");
    if (result === null)
      return null;

    return convertArrayTo3dMatrix(
        str.substring(result[0].length, str.length-1)
           .split(",")
           .map(function(component) {
             return Number(component);
           })
      );
  }

  
  
  
  function convertArrayTo3dMatrix(array) {
    if (array.length == 6) {
      return convertObjectTo3dMatrix(
        { a: array[0], b: array[1],
          c: array[2], d: array[3],
          e: array[4], f: array[5] } );
    } else if (array.length == 16) {
      return [
        array.slice(0, 4),
        array.slice(4, 8),
        array.slice(8, 12),
        array.slice(12, 16)
      ];
    } else {
      return null;
    }
  }

  
  
  function convertObjectTo3dMatrix(obj) {
    return [
      [
        obj.a || obj.sx || obj.m11 || 1,
        obj.b || obj.m12 || 0,
        obj.m13 || 0,
        obj.m14 || 0
      ], [
        obj.c || obj.m21 || 0,
        obj.d || obj.sy || obj.m22 || 1,
        obj.m23 || 0,
        obj.m24 || 0
      ], [
        obj.m31 || 0,
        obj.m32 || 0,
        obj.sz || obj.m33 || 1,
        obj.m34 || 0
      ], [
        obj.e || obj.tx || obj.m41 || 0,
        obj.f || obj.ty || obj.m42 || 0,
        obj.tz || obj.m43 || 0,
        obj.m44 || 1
      ]
    ];
  }

  function convert3dMatrixToString(matrix) {
    if (is2d(matrix)) {
      return "matrix(" +
             [ matrix[0][0], matrix[0][1],
               matrix[1][0], matrix[1][1],
               matrix[3][0], matrix[3][1] ].join(", ") + ")";
    } else {
      return "matrix3d(" +
              matrix.reduce(function(outer, inner) {
                  return outer.concat(inner);
              }).join(", ") + ")";
    }
  }

  function is2d(matrix) {
    return matrix[0][2] === 0 && matrix[0][3] === 0 &&
           matrix[1][2] === 0 && matrix[1][3] === 0 &&
           matrix[2][0] === 0 && matrix[2][1] === 0 &&
           matrix[2][2] === 1 && matrix[2][3] === 0 &&
           matrix[3][2] === 0 && matrix[3][3] === 1;
  }

  function getDeterminant(matrix) {
    if (is2d(matrix)) {
      return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    }

    return   matrix[0][3] * matrix[1][2] * matrix[2][1] * matrix[3][0]
           - matrix[0][2] * matrix[1][3] * matrix[2][1] * matrix[3][0]
           - matrix[0][3] * matrix[1][1] * matrix[2][2] * matrix[3][0]
           + matrix[0][1] * matrix[1][3] * matrix[2][2] * matrix[3][0]
           + matrix[0][2] * matrix[1][1] * matrix[2][3] * matrix[3][0]
           - matrix[0][1] * matrix[1][2] * matrix[2][3] * matrix[3][0]
           - matrix[0][3] * matrix[1][2] * matrix[2][0] * matrix[3][1]
           + matrix[0][2] * matrix[1][3] * matrix[2][0] * matrix[3][1]
           + matrix[0][3] * matrix[1][0] * matrix[2][2] * matrix[3][1]
           - matrix[0][0] * matrix[1][3] * matrix[2][2] * matrix[3][1]
           - matrix[0][2] * matrix[1][0] * matrix[2][3] * matrix[3][1]
           + matrix[0][0] * matrix[1][2] * matrix[2][3] * matrix[3][1]
           + matrix[0][3] * matrix[1][1] * matrix[2][0] * matrix[3][2]
           - matrix[0][1] * matrix[1][3] * matrix[2][0] * matrix[3][2]
           - matrix[0][3] * matrix[1][0] * matrix[2][1] * matrix[3][2]
           + matrix[0][0] * matrix[1][3] * matrix[2][1] * matrix[3][2]
           + matrix[0][1] * matrix[1][0] * matrix[2][3] * matrix[3][2]
           - matrix[0][0] * matrix[1][1] * matrix[2][3] * matrix[3][2]
           - matrix[0][2] * matrix[1][1] * matrix[2][0] * matrix[3][3]
           + matrix[0][1] * matrix[1][2] * matrix[2][0] * matrix[3][3]
           + matrix[0][2] * matrix[1][0] * matrix[2][1] * matrix[3][3]
           - matrix[0][0] * matrix[1][2] * matrix[2][1] * matrix[3][3]
           - matrix[0][1] * matrix[1][0] * matrix[2][2] * matrix[3][3]
           + matrix[0][0] * matrix[1][1] * matrix[2][2] * matrix[3][3];
  }
})();








function waitForPaints() {
  return new Promise(function(resolve, reject) {
    waitForAllPaints(resolve);
  });
}


function waitForPaintsFlushed() {
  return new Promise(function(resolve, reject) {
    waitForAllPaintsFlushed(resolve);
  });
}

function waitForVisitedLinkColoring(visitedLink, waitProperty, waitValue) {
  function checkLink(resolve) {
    if (SpecialPowers.DOMWindowUtils
          .getVisitedDependentComputedStyle(visitedLink, "", waitProperty) ==
        waitValue) {
      
      resolve(true);
    } else {
      
      setTimeout(checkLink, 0, resolve);
    }
  }
  return new Promise(function(resolve, reject) {
    checkLink(resolve);
  });
}
