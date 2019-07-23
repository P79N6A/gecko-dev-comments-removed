

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
    setTimeout('onevalexecuted(false, "setTimeout(String)", "setTimeout with a string was enabled.");', 10);
  } catch (e) {
    onevalblocked(false, "setTimeout(String)",
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
    eval('onevalexecuted(false, "eval(String)", "eval() was enabled.");');
  } catch (e) {
    onevalblocked(false, "eval(String)",
                  "eval() was blocked");
  }

  
  try {
    eval('onevalexecuted(false, "eval(String,scope)", "eval() was enabled.");',1);
  } catch (e) {
    onevalblocked(false, "eval(String,object)",
                  "eval() with scope was blocked");
  }

  
  try {
    ['onevalexecuted(false, "[String, obj].sort(eval)", "eval() was enabled.");',1].sort(eval);
  } catch (e) {
    onevalblocked(false, "[String, obj].sort(eval)",
                  "eval() with scope via sort was blocked");
  }

  
  try {
    [].sort.call(['onevalexecuted(false, "[String, obj].sort(eval)", "eval() was enabled.");',1], eval);
  } catch (e) {
    onevalblocked(false, "[].sort.call([String, obj], eval)",
                  "eval() with scope via sort/call was blocked");
  }

  
  try {
    var fcn = new Function('onevalexecuted(false, "new Function(String)", "new Function(String) was enabled.");');
    fcn();
  } catch (e) {
    onevalblocked(false, "new Function(String)",
                  "new Function(String) was blocked.");
  }

  
  {
    
    
    var worked = false;

    setTimeout(eval, 0, 'worked = true');
    setTimeout(function(worked) {
                  if (worked) {
                    onevalexecuted(false, "setTimeout(eval, 0, str)",
                                    "setTimeout(eval, 0, string) was enabled.");
                  } else {
                    onevalblocked(false, "setTimeout(eval, 0, str)",
                                        "setTimeout(eval, 0, str) was blocked.");
                  }
                }, 0, worked);
  }

}, false);



