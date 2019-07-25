netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
const Cc = Components.classes;
const Ci = Components.interfaces;
const RETURN_CONTINUE = Ci.jsdIExecutionHook.RETURN_CONTINUE;
const DebuggerService = Cc["@mozilla.org/js/jsd/debugger-service;1"];

var jsd = Components.classes['@mozilla.org/js/jsd/debugger-service;1']
                    .getService(Ci.jsdIDebuggerService);
var jsdOnAtStart = false;

function setupJSD(test) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  jsdOnAtStart = jsd.isOn;
  if (jsdOnAtStart) {
      runTest();
  } else {
      jsd.asyncOn({ onDebuggerActivated: function() { runTest(); } });
  }
}





function loadScript(url, element) {
    var script = document.createElement('script');
    script.type = 'text/javascript';
    script.src = url;
    script.defer = false;
    element.appendChild(script);
}

function findScriptByFunction(name) {
    var script;
    jsd.enumerateScripts({ enumerateScript:
                           function(script_) {
                               if (script_.functionName === name) {
                                   script = script_;
                               }
                           }
                         });

    if (typeof(script) === "undefined") {
        throw("Cannot find function named '" + name + "'");
    }

    return script;
}


function breakOnAllLines(script) {
    
    
    var pcs = {};
    for (i = 0; i < script.lineExtent; i++) {
        var jsdLine = script.baseLineNumber + i;
        var pc = script.lineToPc(jsdLine, Ci.jsdIScript.PCMAP_SOURCETEXT);
        pcs[pc] = 1;
    }
        
    
    for (pc in pcs) {
        try {
            script.setBreakpoint(pc);
        } catch(e) {
            alert("Error setting breakpoint: " + e);
        }
    }
}



function breakOnLine(script, lineno) {
    breakOnAbsoluteLine(script, script.baseLineNumber + lineno);
}

function breakOnAbsoluteLine(script, lineno) {
    var pc = script.lineToPc(lineno, Ci.jsdIScript.PCMAP_SOURCETEXT);
    script.setBreakpoint(pc);
}

function loadPage(page) {
    var url;
    if (page.match(/^\w+:/)) {
        
        url = page;
    } else {
        
        url = document.location.href.replace(/\/[^\/]*$/, "/" + page);
    }

    dump("Switching to URL " + url + "\n");

    gURLBar.value = url;
    gURLBar.handleCommand();
}

function breakpointObserver(lines, interesting, callback) {
    jsd.breakpointHook = { onExecute: function(frame, type, rv) {
        breakpoints_hit.push(frame.line);
        if (frame.line in interesting) {
            return callback(frame, type, breakpoints_hit);
        } else {
            return RETURN_CONTINUE;
        }
    } };
}

function dumpStack(frame, msg) {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    dump(msg + ":\n");
    while(frame) {
        var callee = frame.callee;
        if (callee !== null)
          callee = callee.jsClassName;
        dump("  " + frame.script.fileName + ":" + frame.line + " func=" + frame.script.functionName + " ffunc=" + frame.functionName + " callee=" + callee + " pc=" + frame.pc + "\n");
        frame = frame.callingFrame;
    }
}
