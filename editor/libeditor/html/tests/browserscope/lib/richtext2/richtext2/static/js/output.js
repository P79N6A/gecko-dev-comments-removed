


























function writeFatalError(text) {
  var errorsStart = document.getElementById('errors');
  var divider = document.getElementById('divider');
  if (!errorsStart) {
    errorsStart = document.createElement('hr');
    errorsStart.id = 'errors';
    divider.parentNode.insertBefore(errorsStart, divider);
  }
  var error = document.createElement('div');
  error.className = 'fatalerror';
  error.innerHTML = 'FATAL ERROR: ' + escapeOutput(text);
  errorsStart.parentNode.insertBefore(error, divider);
}









function generateOutputID(suiteID, testID) {
  return commonIDPrefix + '-' + suiteID + '_' + testID;
}







function highlightSelectionMarkers(str) {
  str = str.replace(/\[/g, '<span class="sel">[</span>');
  str = str.replace(/\]/g, '<span class="sel">]</span>');
  str = str.replace(/\^/g, '<span class="sel">^</span>');
  str = str.replace(/{/g,  '<span class="sel">{</span>');
  str = str.replace(/}/g,  '<span class="sel">}</span>');
  str = str.replace(/\|/g, '<b class="sel">|</b>');
  return str;
}
                       






function highlightSelectionMarkersAndTextNodes(str) {
  str = highlightSelectionMarkers(str);
  str = str.replace(/\x60/g, '<span class="txt">');
  str = str.replace(/\xb4/g, '</span>');
  return str;
}
                       






function formatValueOrString(value) {
  if (value === undefined)
    return '<i>undefined</i>';
  if (value === null)
    return '<i>null</i>';
  
  switch (typeof value) {
    case 'boolean':
      return '<i>' + value.toString() + '</i>';
      
    case 'number':
      return value.toString();
      
    case 'string':
      return "'" + escapeOutput(value) + "'";
      
    default:
      return '<i>(' + escapeOutput(value.toString()) + ')</i>';
  } 
}










function formatActualResult(suite, group, test, actual) {
  if (typeof actual != 'string')
    return formatValueOrString(actual);

  actual = escapeOutput(actual);

  
  if (!getTestParameter(suite, group, test, PARAM_CHECK_ATTRIBUTES)) {
    actual = actual.replace(/([^ =]+)=\x22([^\x22]*)\x22/g, '<span class="fade">$1="$2"</span>');
  } else {
    
    if (!getTestParameter(suite, group, test, PARAM_CHECK_CLASS)) {
      actual = actual.replace(/class=\x22([^\x22]*)\x22/g, '<span class="fade">class="$1"</span>');
    }
    if (!getTestParameter(suite, group, test, PARAM_CHECK_STYLE)) {
      actual = actual.replace(/style=\x22([^\x22]*)\x22/g, '<span class="fade">style="$1"</span>');
    }
    if (!getTestParameter(suite, group, test, PARAM_CHECK_ID)) {
      actual = actual.replace(/id=\x22([^\x22]*)\x22/g, '<span class="fade">id="$1"</span>');
    } else {
      
      actual = actual.replace(/id=\x22editor-([^\x22]*)\x22/g, '<span class="fade">id="editor-$1"</span>');
    }
    
    actual = actual.replace(/xmlns=\x22([^\x22]*)\x22/g, '<span class="fade">xmlns="$1"</span>');
    
    actual = actual.replace(/onload=\x22[^\x22]*\x22 ?/g, '');
  }
  
  actual = highlightSelectionMarkersAndTextNodes(actual);

  return actual;
}







function escapeOutput(str) {
  return str ? str.replace(/\</g, '&lt;').replace(/\>/g, '&gt;') : '';
}









function setTD(id, val, ttl, cls) {
  var td = document.getElementById(id);
  if (td) {
    td.innerHTML = val;
    if (ttl) {
      td.title = ttl;
    }
    if (cls) {
      td.className = cls;
    }
  }
}









function outputTestResults(suite, clsID, group, test) {
  var suiteID = suite.id;
  var cls = suite[clsID];
  var trID = generateOutputID(suiteID, test.id);
  var testResult = results[suiteID][clsID][test.id];
  var testValOut = VALOUTPUT[testResult.valresult];
  var testSelOut = SELOUTPUT[testResult.selresult];

  var suiteChecksSelOnly = !suiteChecksHTMLOrText(suite);
  var testUsesHTML = !!getTestParameter(suite, group, test, PARAM_EXECCOMMAND) ||
                     !!getTestParameter(suite, group, test, PARAM_FUNCTION);

  
  var td = document.getElementById(trID + IDOUT_TESTID);
  if (td) {
    td.className = (suiteChecksSelOnly && testResult.selresult != SELRESULT_NA) ? testSelOut.css : testValOut.css;
  }

  
  var cmd;
  var cmdOutput = '&nbsp;';
  var valOutput = '&nbsp;';

  if (cmd = getTestParameter(suite, group, test, PARAM_EXECCOMMAND)) {
    cmdOutput = escapeOutput(cmd);
    var val = getTestParameter(suite, group, test, PARAM_VALUE);
    if (val !== undefined) {
      valOutput = formatValueOrString(val);
    }
  } else if (cmd = getTestParameter(suite, group, test, PARAM_FUNCTION)) {
    cmdOutput = '<i>' + escapeOutput(cmd) + '</i>';
  } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDSUPPORTED)) {
    cmdOutput = '<i>queryCommandSupported</i>';
    valOutput = escapeOutput(cmd);
  } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDENABLED)) {
    cmdOutput = '<i>queryCommandEnabled</i>';
    valOutput = escapeOutput(cmd);
  } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDINDETERM)) {
    cmdOutput = '<i>queryCommandIndeterm</i>';
    valOutput = escapeOutput(cmd);
  } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDSTATE)) {
    cmdOutput = '<i>queryCommandState</i>';
    valOutput = escapeOutput(cmd);
  } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDVALUE)) {
    cmdOutput = '<i>queryCommandValue</i>';
    valOutput = escapeOutput(cmd);
  } else {
    cmdOutput = '<i>(none)</i>';
  }
  setTD(trID + IDOUT_COMMAND, cmdOutput);
  setTD(trID + IDOUT_VALUE, valOutput);

  
  if (testUsesHTML) {
    var checkAttrs = getTestParameter(suite, group, test, PARAM_CHECK_ATTRIBUTES);
    var checkStyle = getTestParameter(suite, group, test, PARAM_CHECK_STYLE);

    setTD(trID + IDOUT_CHECKATTRS,
          checkAttrs ? OUTSTR_YES : OUTSTR_NO,
          checkAttrs ? 'attributes must match' : 'attributes are ignored');

    if (checkAttrs && checkStyle) {
      setTD(trID + IDOUT_CHECKSTYLE, OUTSTR_YES, 'style attribute contents must match');
    } else if (checkAttrs) {
      setTD(trID + IDOUT_CHECKSTYLE, OUTSTR_NO, 'style attribute contents is ignored');
    } else {
      setTD(trID + IDOUT_CHECKSTYLE, OUTSTR_NO, 'all attributes (incl. style) are ignored');
    }
  } else {
    setTD(trID + IDOUT_CHECKATTRS, OUTSTR_NA, 'attributes not applicable');
    setTD(trID + IDOUT_CHECKSTYLE, OUTSTR_NA, 'style not applicable');
  }
  
  
  setTD(trID + IDOUT_PAD, highlightSelectionMarkers(escapeOutput(getTestParameter(suite, group, test, PARAM_PAD))));

  
  var expectedOutput = '';
  var expectedArr = getExpectationArray(getTestParameter(suite, group, test, PARAM_EXPECTED));
  for (var idx = 0; idx < expectedArr.length; ++idx) {
    if (expectedOutput) {
      expectedOutput += '\xA0\xA0\xA0<i>or</i><br>';
    }
    expectedOutput += testUsesHTML ? highlightSelectionMarkers(escapeOutput(expectedArr[idx]))
                                   : formatValueOrString(expectedArr[idx]);
  }
  var acceptedArr = getExpectationArray(getTestParameter(suite, group, test, PARAM_ACCEPT));    
  for (var idx = 0; idx < acceptedArr.length; ++idx) {
    expectedOutput += '<span class="accexp">\xA0\xA0\xA0<i>or</i></span><br><span class="accexp">';
    expectedOutput += testUsesHTML ? highlightSelectionMarkers(escapeOutput(acceptedArr[idx]))
                                   : formatValueOrString(acceptedArr[idx]);
    expectedOutput += '</span>';
  }
  
  
  var outerOutput = '';
  expectedArr = getExpectationArray(getContainerParameter(suite, group, test, containers[2], PARAM_EXPECTED_OUTER));
  for (var idx = 0; idx < expectedArr.length; ++idx) {
    if (outerOutput) {
      outerOutput += '\xA0\xA0\xA0<i>or</i><br>';
    }
    outerOutput += testUsesHTML ? highlightSelectionMarkers(escapeOutput(expectedArr[idx]))
                                : formatValueOrString(expectedArr[idx]);
  }
  acceptedArr = getExpectationArray(getContainerParameter(suite, group, test, containers[2], PARAM_ACCEPT_OUTER));
  for (var idx = 0; idx < acceptedArr.length; ++idx) {
    if (outerOutput) {
      outerOutput += '<span class="accexp">\xA0\xA0\xA0<i>or</i></span><br>';
    }
    outerOutput += '<span class="accexp">';
    outerOutput += testUsesHTML ? highlightSelectionMarkers(escapeOutput(acceptedArr[idx]))
                                : formatValueOrString(acceptedArr[idx]);
    outerOutput += '</span>';
  }
  if (outerOutput) {
    expectedOutput += '<hr>' + outerOutput;
  }
  setTD(trID + IDOUT_EXPECTED, expectedOutput);

  
  for (var cntIdx = 0; cntIdx < containers.length; ++cntIdx) {
    var cntID = containers[cntIdx].id;
    var cntTD = document.getElementById(trID + IDOUT_CONTAINER + cntID);
    var cntResult = testResult[cntID];
    var cntValOut = VALOUTPUT[cntResult.valresult];
    var cntSelOut = SELOUTPUT[cntResult.selresult];
    var cssVal = cntValOut.css;
    var cssSel = (!suiteChecksSelOnly || cntResult.selresult != SELRESULT_NA) ? cntSelOut.css : cssVal;
    var cssCnt = cssVal;

    
    setTD(trID + IDOUT_STATUSVAL + cntID, cntValOut.output, cntValOut.title, cssVal);
    
    
    setTD(trID + IDOUT_STATUSSEL + cntID, cntSelOut.output, cntSelOut.title, cssSel);

    
    switch (cntResult.valresult) {
      case VALRESULT_SETUP_EXCEPTION:
        setTD(trID + IDOUT_ACTUAL + cntID,
              SETUP_EXCEPTION + '(mouseover)',
              escapeOutput(cntResult.output),
              cssVal);
        break;

      case VALRESULT_EXECUTION_EXCEPTION:
        setTD(trID + IDOUT_ACTUAL + cntID,
              EXECUTION_EXCEPTION + '(mouseover)',
              escapeOutput(cntResult.output.toString()),
              cssVal);
        break;

      case VALRESULT_VERIFICATION_EXCEPTION:
        setTD(trID + IDOUT_ACTUAL + cntID,
              VERIFICATION_EXCEPTION + '(mouseover)',
              escapeOutput(cntResult.output.toString()),
              cssVal);
        break;

      case VALRESULT_UNSUPPORTED:
        setTD(trID + IDOUT_ACTUAL + cntID,
              escapeOutput(cntResult.output),
              '',
              cssVal);
        break;

      case VALRESULT_CANARY:
        setTD(trID + IDOUT_ACTUAL + cntID,
              highlightSelectionMarkersAndTextNodes(escapeOutput(cntResult.output)),
              '',
              cssVal);
        break;

      case VALRESULT_DIFF:
      case VALRESULT_ACCEPT:
      case VALRESULT_EQUAL:
        if (!testUsesHTML) {
          setTD(trID + IDOUT_ACTUAL + cntID,
                formatValueOrString(cntResult.output),
                '',
                cssVal);
        } else if (cntResult.selresult == SELRESULT_CANARY) {
          cssCnt = suiteChecksSelOnly ? cssSel : cssVal;
          setTD(trID + IDOUT_ACTUAL + cntID, 
                highlightSelectionMarkersAndTextNodes(escapeOutput(cntResult.output)),
                '',
                cssCnt);
        } else {
          cssCnt = suiteChecksSelOnly ? cssSel : cssVal;
          setTD(trID + IDOUT_ACTUAL + cntID, 
                formatActualResult(suite, group, test, cntResult.output),
                '',
                cssCnt);
        }
        break;

      default:
        cssCnt = 'exception';
        setTD(trID + IDOUT_ACTUAL + cntID,
              INTERNAL_ERR + 'UNKNOWN RESULT VALUE',
              '',
              cssCnt);
    }

    if (cntTD) {
      cntTD.className = cssCnt;
    }
  }          
}






function outputTestSuiteResults(suite) {
  var suiteID = suite.id;
  var span;

  span = document.getElementById(suiteID + '-score');
  if (span) {
    span.innerHTML = results[suiteID].valscore + '/' + results[suiteID].count;
  }
  span = document.getElementById(suiteID + '-selscore');
  if (span) {
    span.innerHTML = results[suiteID].selscore + '/' + results[suiteID].count;
  }
  span = document.getElementById(suiteID + '-time');
  if (span) {
    span.innerHTML = results[suiteID].time;
  }
  span = document.getElementById(suiteID + '-progress');
  if (span) {
    span.style.color = 'green';
  }

  for (var clsIdx = 0; clsIdx < testClassCount; ++clsIdx) {
    var clsID = testClassIDs[clsIdx];
    var cls = suite[clsID];
    if (!cls)
      continue;

    span = document.getElementById(suiteID + '-' + clsID + '-score');
    if (span) {
      span.innerHTML = results[suiteID][clsID].valscore + '/' + results[suiteID][clsID].count;
    }
    span = document.getElementById(suiteID + '-' + clsID + '-selscore');
    if (span) {
      span.innerHTML = results[suiteID][clsID].selscore + '/' + results[suiteID][clsID].count;
    }

    var groupCount = cls.length;
    
    for (var groupIdx = 0; groupIdx < groupCount; ++groupIdx) {
      var group = cls[groupIdx];
      var testCount = group.tests.length;

      for (var testIdx = 0; testIdx < testCount; ++testIdx) {
        var test = group.tests[testIdx];

        outputTestResults(suite, clsID, group, test);
      }
    }
  }
}
