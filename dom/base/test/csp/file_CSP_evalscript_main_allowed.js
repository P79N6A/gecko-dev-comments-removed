



function logResult(str, passed) {
  var elt = document.createElement('div');
  var color = passed ? "#cfc;" : "#fcc";
  elt.setAttribute('style', 'background-color:' + color + '; width:100%; border:1px solid black; padding:3px; margin:4px;');
  elt.innerHTML = str;
  document.body.appendChild(elt);
}


var onevalexecuted = (function(window) {
    return function(shouldrun, what, data) {
      window.parent.scriptRan(shouldrun, what, data);
      logResult((shouldrun ? "PASS: " : "FAIL: ") + what + " : " + data, shouldrun);
    };})(window);


var onevalblocked = (function(window) {
    return function(shouldrun, what, data) {
      window.parent.scriptBlocked(shouldrun, what, data);
      logResult((shouldrun ? "FAIL: " : "PASS: ") + what + " : " + data, !shouldrun);
    };})(window);




addEventListener('load', function() {
  
  try {
    setTimeout('onevalexecuted(true, "setTimeout(String)", "setTimeout with a string was enabled.");', 10);
  } catch (e) {
    onevalblocked(true, "setTimeout(String)",
                  "setTimeout with a string was blocked");
  }

  
  try {
    setTimeout(function() {
          onevalexecuted(true, "setTimeout(function)",
                        "setTimeout with a function was enabled.")
        }, 10);
  } catch (e) {
    onevalblocked(true, "setTimeout(function)",
                  "setTimeout with a function was blocked");
  }

  
  try {
    eval('onevalexecuted(true, "eval(String)", "eval() was enabled.");');
  } catch (e) {
    onevalblocked(true, "eval(String)",
                  "eval() was blocked");
  }

  
  try {
    eval('onevalexecuted(true, "eval(String,scope)", "eval() was enabled.");',1);
  } catch (e) {
    onevalblocked(true, "eval(String,object)",
                  "eval() with scope was blocked");
  }

  
  try {
    ['onevalexecuted(true, "[String, obj].sort(eval)", "eval() was enabled.");',1].sort(eval);
  } catch (e) {
    onevalblocked(true, "[String, obj].sort(eval)",
                  "eval() with scope via sort was blocked");
  }

  
  try {
    [].sort.call(['onevalexecuted(true, "[String, obj].sort(eval)", "eval() was enabled.");',1], eval);
  } catch (e) {
    onevalblocked(true, "[].sort.call([String, obj], eval)",
                  "eval() with scope via sort/call was blocked");
  }

  
  try {
    var fcn = new Function('onevalexecuted(true, "new Function(String)", "new Function(String) was enabled.");');
    fcn();
  } catch (e) {
    onevalblocked(true, "new Function(String)",
                  "new Function(String) was blocked.");
  }

  function checkResult() {
    
    if (bar) {
      onevalexecuted(true, "setTimeout(eval, 0, str)",
                      "setTimeout(eval, 0, string) was enabled.");
    } else {
      onevalblocked(true, "setTimeout(eval, 0, str)",
                          "setTimeout(eval, 0, str) was blocked.");
    }
  }

  var bar = false;

  function foo() {
    bar = true;
  }

  window.foo = foo;

  

  

  setTimeout(eval, 0, 'window.foo();');

  setTimeout(checkResult.bind(this), 0);

}, false);



