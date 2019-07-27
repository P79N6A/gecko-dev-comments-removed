



























function suiteChecksHTMLOrText(suite) {
  return suite.id[0] != 'S';
}







function suiteChecksSelection(suite) {
  return suite.id[0] != 'Q';
}










function getTestParameter(suite, group, test, param) {
  var val = test[param];
  if (val === undefined) {
    val = group[param];
  }
  if (val === undefined) {
    val = suite[param];
  }
  return val;
}











function getContainerParameter(suite, group, test, container, param) {
  var val = undefined;
  if (test[container.id]) {
    val = test[container.id][param];
  }
  if (val === undefined) {
    val = test[param];
  }
  if (val === undefined) {
    val = group[param];
  }
  if (val === undefined) {
    val = suite[param];
  }
  return val;
}




function initVariables() {
  results = {
      count: 0,
      valscore: 0,
      selscore: 0
  };
}










function runSingleTest(suite, group, test, container) {
  var result = {
    valscore: 0,
    selscore: 0,
    valresult: VALRESULT_NOT_RUN,
    selresult: SELRESULT_NOT_RUN,
    output: ''
  };

  
  try {
    initContainer(suite, group, test, container);
  } catch(ex) {
    result.valresult = VALRESULT_SETUP_EXCEPTION;
    result.selresult = SELRESULT_NA;
    result.output = SETUP_EXCEPTION + ex.toString();
    return result;
  }

  
  var isHTMLTest = false;

  try {
    var cmd = undefined;

    if (cmd = getTestParameter(suite, group, test, PARAM_EXECCOMMAND)) {
      isHTMLTest = true;
      
      
      var value = getTestParameter(suite, group, test, PARAM_VALUE);
      if (value === undefined) {
        value = null;
      }
      container.doc.execCommand(cmd, false, value);
    } else if (cmd = getTestParameter(suite, group, test, PARAM_FUNCTION)) {
      isHTMLTest = true;
      eval(cmd);
    } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDSUPPORTED)) {
      result.output = container.doc.queryCommandSupported(cmd);
    } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDENABLED)) {
      result.output = container.doc.queryCommandEnabled(cmd);
    } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDINDETERM)) {
      result.output = container.doc.queryCommandIndeterm(cmd);
    } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDSTATE)) {
      result.output = container.doc.queryCommandState(cmd);
    } else if (cmd = getTestParameter(suite, group, test, PARAM_QUERYCOMMANDVALUE)) {
      result.output = container.doc.queryCommandValue(cmd);
      if (result.output === false) {
        
        result.valresult = VALRESULT_UNSUPPORTED;
        result.selresult = SELRESULT_NA;
        result.output = UNSUPPORTED;
        return result;
      }
    } else {
      result.valresult = VALRESULT_SETUP_EXCEPTION;
      result.selresult = SELRESULT_NA;
      result.output = SETUP_EXCEPTION + SETUP_NOCOMMAND;
      return result;
    }
  } catch (ex) {
    result.valresult = VALRESULT_EXECUTION_EXCEPTION;
    result.selresult = SELRESULT_NA;
    result.output = EXECUTION_EXCEPTION + ex.toString();
    return result;
  }
  
  
  try {
    if (isHTMLTest) {
      
      prepareHTMLTestResult(container, result);

      
      compareHTMLTestResult(suite, group, test, container, result);

      result.valscore = (result.valresult === VALRESULT_EQUAL) ? 1 : 0;
      result.selscore = (result.selresult === SELRESULT_EQUAL) ? 1 : 0;
    } else {
      compareTextTestResult(suite, group, test, result);

      result.selresult = SELRESULT_NA;
      result.valscore = (result.valresult === VALRESULT_EQUAL) ? 1 : 0;
    }
  } catch (ex) {
    result.valresult = VALRESULT_VERIFICATION_EXCEPTION;
    result.selresult = SELRESULT_NA;
    result.output = VERIFICATION_EXCEPTION + ex.toString();
    return result;
  }
  
  return result;
}







function initTestSuiteResults(suite) {
  var suiteID = suite.id;

  
  results[suiteID] = {
      count: 0,
      valscore: 0,
      selscore: 0,
      time: 0
  };
  var totalTestCount = 0;

  for (var clsIdx = 0; clsIdx < testClassCount; ++clsIdx) {
    var clsID = testClassIDs[clsIdx];
    var cls = suite[clsID];
    if (!cls)
      continue;

    results[suiteID][clsID] = {
        count: 0,
        valscore: 0,
        selscore: 0
    };
    var clsTestCount = 0;

    var groupCount = cls.length;
    for (var groupIdx = 0; groupIdx < groupCount; ++groupIdx) {
      var group = cls[groupIdx];
      var testCount = group.tests.length;

      clsTestCount += testCount;
      totalTestCount += testCount;

      for (var testIdx = 0; testIdx < testCount; ++testIdx) {
        var test = group.tests[testIdx];
        
        results[suiteID][clsID ][test.id] = {
            valscore: 0,
            selscore: 0,
            valresult: VALRESULT_NOT_RUN,
            selresult: SELRESULT_NOT_RUN
        };
        for (var cntIdx = 0; cntIdx < containers.length; ++cntIdx) {
          var cntID = containers[cntIdx].id;

          results[suiteID][clsID][test.id][cntID] = {
            valscore: 0,
            selscore: 0,
            valresult: VALRESULT_NOT_RUN,
            selresult: SELRESULT_NOT_RUN,
            output: ''
          }
        }
      }
    }
    results[suiteID][clsID].count = clsTestCount;
  }
  results[suiteID].count = totalTestCount;
}






function runTestSuite(suite) {
  var suiteID = suite.id;
  var suiteStartTime = new Date().getTime();

  initTestSuiteResults(suite);

  for (var clsIdx = 0; clsIdx < testClassCount; ++clsIdx) {
    var clsID = testClassIDs[clsIdx];
    var cls = suite[clsID];
    if (!cls)
      continue;

    var groupCount = cls.length;

    for (var groupIdx = 0; groupIdx < groupCount; ++groupIdx) {
      var group = cls[groupIdx];
      var testCount = group.tests.length;

      for (var testIdx = 0; testIdx < testCount; ++testIdx) {
        var test = group.tests[testIdx];

        var valscore = 1;
        var selscore = 1;
        var valresult = VALRESULT_EQUAL;
        var selresult = SELRESULT_EQUAL;

        for (var cntIdx = 0; cntIdx < containers.length; ++cntIdx) {
          var container = containers[cntIdx];
          var cntID = container.id;

          var result = runSingleTest(suite, group, test, container);

          results[suiteID][clsID][test.id][cntID] = result;

          valscore = Math.min(valscore, result.valscore);
          selscore = Math.min(selscore, result.selscore);
          valresult = Math.min(valresult, result.valresult);
          selresult = Math.min(selresult, result.selresult);

          resetContainer(container);
        }          

        results[suiteID][clsID][test.id].valscore = valscore;
        results[suiteID][clsID][test.id].selscore = selscore;
        results[suiteID][clsID][test.id].valresult = valresult;
        results[suiteID][clsID][test.id].selresult = selresult;

        results[suiteID][clsID].valscore += valscore;
        results[suiteID][clsID].selscore += selscore;
        results[suiteID].valscore += valscore;
        results[suiteID].selscore += selscore;
        results.valscore += valscore;
        results.selscore += selscore;
      }
    }
  }

  results[suiteID].time = new Date().getTime() - suiteStartTime;
}







function runAndOutputTestSuite(suite) {
  runTestSuite(suite);
  outputTestSuiteResults(suite);
}




function fillResults() {
  
  categoryTotals = [
    'selection='        + results['S'].selscore,
    'apply='            + results['A'].valscore,
    'applyCSS='         + results['AC'].valscore,
    'change='           + results['C'].valscore,
    'changeCSS='        + results['CC'].valscore,
    'unapply='          + results['U'].valscore,
    'unapplyCSS='       + results['UC'].valscore,
    'delete='           + results['D'].valscore,
    'forwarddelete='    + results['FD'].valscore,
    'insert='           + results['I'].valscore,
    'selectionResult='  + (results['A'].selscore +
                           results['AC'].selscore +
                           results['C'].selscore +
                           results['CC'].selscore +
                           results['U'].selscore +
                           results['UC'].selscore +
                           results['D'].selscore +
                           results['FD'].selscore +
                           results['I'].selscore),
    'querySupported='   + results['Q'].valscore,
    'queryEnabled='     + results['QE'].valscore,
    'queryIndeterm='    + results['QI'].valscore,
    'queryState='       + results['QS'].valscore,
    'queryStateCSS='    + results['QSC'].valscore,
    'queryValue='       + results['QV'].valscore,
    'queryValueCSS='    + results['QVC'].valscore
  ];
  
  
  beacon = categoryTotals.slice(0);
}

