
























var RESULT_DIFF  = 0;  
var RESULT_SEL   = 1;  
var RESULT_EQUAL = 2;  







function getExpectationArray(expected) {
  if (expected === undefined) {
    return [];
  }
  if (expected === null) {
    return [null];
  }
  switch (typeof expected) {
    case 'string':
    case 'boolean':
    case 'number':
      return [expected];
  }
  
  return expected;
}











function compareHTMLToSingleExpectation(expected, actual) {
  
  
  if (expected == actual) {
    return RESULT_EQUAL;
  }

  
  expected = expected.replace(/ [{}\|]>/g, '>');     
  expected = expected.replace(/[\[\]\^{}\|]/g, '');  
  actual = actual.replace(/ [{}\|]>/g, '>');         
  actual = actual.replace(/[\[\]\^{}\|]/g, '');      

  return (expected == actual) ? RESULT_SEL : RESULT_DIFF;
}










function compareHTMLToExpectation(actual, expected, emitFlags) {
  
  var expectedArr = getExpectationArray(expected);
  var count = expectedArr ? expectedArr.length : 0;
  var best = RESULT_DIFF;

  for (var idx = 0; idx < count && best < RESULT_EQUAL; ++idx) {
    var expected = expectedArr[idx];
    expected = canonicalizeSpaces(expected);
    expected = canonicalizeElementsAndAttributes(expected, emitFlags);

    var singleResult = compareHTMLToSingleExpectation(expected, actual);

    best = Math.max(best, singleResult);
  }
  return best;
}










function compareHTMLTestResultTo(expected, accepted, actual, emitFlags, result) {
  actual = actual.replace(/[\x60\xb4]/g, '');
  actual = canonicalizeElementsAndAttributes(actual, emitFlags);

  var bestExpected = compareHTMLToExpectation(actual, expected, emitFlags);

  if (bestExpected == RESULT_EQUAL) {
    
    result.valresult = VALRESULT_EQUAL;
    result.selresult = SELRESULT_EQUAL;
    return;
  }

  var bestAccepted = compareHTMLToExpectation(actual, accepted, emitFlags);

  switch (bestExpected) {
    case RESULT_SEL:
      switch (bestAccepted) {
        case RESULT_EQUAL:
          
          
          
          result.valresult = VALRESULT_EQUAL;
          result.selresult = SELRESULT_ACCEPT;
          return;

        case RESULT_SEL:
        case RESULT_DIFF:
          
          
          result.valresult = VALRESULT_EQUAL;
          result.selresult = SELRESULT_DIFF;
          return;
      }
      break;

    case RESULT_DIFF:
      switch (bestAccepted) {
        case RESULT_EQUAL:
          result.valresult = VALRESULT_ACCEPT;
          result.selresult = SELRESULT_EQUAL;
          return;

        case RESULT_SEL:
          result.valresult = VALRESULT_ACCEPT;
          result.selresult = SELRESULT_DIFF;
          return;

        case RESULT_DIFF:
          result.valresult = VALRESULT_DIFF;
          result.selresult = SELRESULT_NA;
          return;
      }
      break;
  }
  
  throw INTERNAL_ERR + HTML_COMPARISON;
}








function verifyCanaries(container, result) {
  if (!container.canary) {
    return true;
  }

  var str = canonicalizeElementsAndAttributes(result.bodyInnerHTML, emitFlagsForCanary);

  if (str.length < 2 * container.canary.length) {
    result.valresult = VALRESULT_CANARY;
    result.selresult = SELRESULT_NA;
    result.output = result.bodyOuterHTML;
    return false;
  }

  var strBefore = str.substr(0, container.canary.length);
  var strAfter  = str.substr(str.length - container.canary.length);

  
  if (SELECTION_MARKERS.test(strBefore) || SELECTION_MARKERS.test(strAfter)) {
    str = str.replace(SELECTION_MARKERS, '');
    if (str.length < 2 * container.canary.length) {
      result.valresult = VALRESULT_CANARY;
      result.selresult = SELRESULT_NA;
      result.output = result.bodyOuterHTML;
      return false;
    }

    
    result.selresult = SELRESULT_CANARY;
    strBefore = str.substr(0, container.canary.length);
    strAfter  = str.substr(str.length - container.canary.length);
  }

  if (strBefore !== container.canary || strAfter !== container.canary) {
    result.valresult = VALRESULT_CANARY;
    result.selresult = SELRESULT_NA;
    result.output = result.bodyOuterHTML;
    return false;
  }

  return true;
}












function compareHTMLTestResult(suite, group, test, container, result) {
  if (!verifyCanaries(container, result)) {
    return;
  }

  var emitFlags = {
      emitAttrs:         getTestParameter(suite, group, test, PARAM_CHECK_ATTRIBUTES),
      emitStyle:         getTestParameter(suite, group, test, PARAM_CHECK_STYLE),
      emitClass:         getTestParameter(suite, group, test, PARAM_CHECK_CLASS),
      emitID:            getTestParameter(suite, group, test, PARAM_CHECK_ID),
      lowercase:         true,
      canonicalizeUnits: true
  };

  
  
  var openingTagEnd = result.outerHTML.indexOf('>') + 1;
  var openingTag = result.outerHTML.substr(0, openingTagEnd);

  openingTag = canonicalizeElementsAndAttributes(openingTag, emitFlags);
  var tagCmp = compareHTMLToExpectation(openingTag, container.tagOpen, emitFlags);

  if (tagCmp == RESULT_EQUAL) {
    result.output = result.innerHTML;
    compareHTMLTestResultTo(
        getTestParameter(suite, group, test, PARAM_EXPECTED),
        getTestParameter(suite, group, test, PARAM_ACCEPT),
        result.innerHTML,
        emitFlags,
        result)
  } else {
    result.output = result.outerHTML;
    compareHTMLTestResultTo(
        getContainerParameter(suite, group, test, container, PARAM_EXPECTED_OUTER),
        getContainerParameter(suite, group, test, container, PARAM_ACCEPT_OUTER),
        result.outerHTML,
        emitFlags,
        result)
  }
}









function insertSelectionIndicator(node, offs, textInd, elemInd) {
  switch (node.nodeType) {
    case DOM_NODE_TYPE_TEXT:
      
      var text = node.data;
      node.data = text.substring(0, offs) + textInd + text.substring(offs);
      break;
      
    case DOM_NODE_TYPE_ELEMENT:
      var child = node.firstChild;
      try {
        
        var comment = document.createComment(elemInd);
        while (child && offs) {
          --offs;
          child = child.nextSibling;
        }
        if (child) {
          node.insertBefore(comment, child);
        } else {
          node.appendChild(comment);
        }
      } catch (ex) {
        
        switch (elemInd) {
          case '|':
            node.setAttribute(ATTRNAME_SEL_START, '1');
            node.setAttribute(ATTRNAME_SEL_END, '1');
            break;

          case '{':
            node.setAttribute(ATTRNAME_SEL_START, '1');
            break;

          case '}':
            node.setAttribute(ATTRNAME_SEL_END, '1');
            break;
        }
      }
      break;
  }
}








function encloseTextNodesWithQuotes(node) {
  switch (node.nodeType) {
    case DOM_NODE_TYPE_ELEMENT:
      for (var i = 0; i < node.childNodes.length; ++i) {
        encloseTextNodesWithQuotes(node.childNodes[i]);
      }
      break;
      
    case DOM_NODE_TYPE_TEXT:
      node.data = '\x60' + node.data + '\xb4';
      break;
  }
}








function prepareHTMLTestResult(container, result) {
  
  result.innerHTML = '';
  result.outerHTML = '';

  
  var selRange = createFromWindow(container.win);
  if (selRange) {
    
    var node1 = selRange.getAnchorNode();
    var offs1 = selRange.getAnchorOffset();
    var node2 = selRange.getFocusNode();
    var offs2 = selRange.getFocusOffset();

    
    if (node1 && node1 == node2 && offs1 == offs2) {
      
      insertSelectionIndicator(node1, offs1, '^', '|');
    } else {
      
      if (node1) {
        insertSelectionIndicator(node1, offs1, '[', '{');
      }

      if (node2) {
        if (node1 == node2 && offs1 < offs2) {
          
          
          ++offs2;
        }
        insertSelectionIndicator(node2, offs2, ']', '}');
      }
    }
  }

  
  encloseTextNodesWithQuotes(container.editor);
  
  
  result.innerHTML = initialCanonicalizationOf(container.editor.innerHTML);
  result.bodyInnerHTML = initialCanonicalizationOf(container.body.innerHTML);
  if (goog.userAgent.IE) {
    result.outerHTML = initialCanonicalizationOf(container.editor.outerHTML);
    result.bodyOuterHTML = initialCanonicalizationOf(container.body.outerHTML);
    result.outerHTML = result.outerHTML.replace(/^\s+/, '');
    result.outerHTML = result.outerHTML.replace(/\s+$/, '');
    result.bodyOuterHTML = result.bodyOuterHTML.replace(/^\s+/, '');
    result.bodyOuterHTML = result.bodyOuterHTML.replace(/\s+$/, '');
  } else {
    result.outerHTML = initialCanonicalizationOf(new XMLSerializer().serializeToString(container.editor));
    result.bodyOuterHTML = initialCanonicalizationOf(new XMLSerializer().serializeToString(container.body));
  }
}











function compareTextTestResultWith(suite, group, test, actual, expected) {
  var expectedArr = getExpectationArray(expected);
  
  var count = expectedArr.length;

  
  for (var idx = 0; idx < count; ++idx) {
    if (actual === expectedArr[idx])
      return true;
  }
  
  
  
  
  
  
  
  
  switch (getTestParameter(suite, group, test, PARAM_QUERYCOMMANDVALUE)) {
    case 'backcolor':
    case 'forecolor':
    case 'hilitecolor':
      for (var idx = 0; idx < count; ++idx) {
        if (new Color(actual).compare(new Color(expectedArr[idx])))
          return true;
      }
      return false;
    
    case 'fontname':
      for (var idx = 0; idx < count; ++idx) {
        if (new FontName(actual).compare(new FontName(expectedArr[idx])))
          return true;
      }
      return false;
    
    case 'fontsize':
      for (var idx = 0; idx < count; ++idx) {
        if (new FontSize(actual).compare(new FontSize(expectedArr[idx])))
          return true;
      }
      return false;
  }
  
  return false;
}












function compareTextTestResult(suite, group, test, result) {
  var expected = getTestParameter(suite, group, test, PARAM_EXPECTED);
  if (compareTextTestResultWith(suite, group, test, result.output, expected)) {
    result.valresult = VALRESULT_EQUAL;
    return;
  }
  var accepted = getTestParameter(suite, group, test, PARAM_ACCEPT);
  if (accepted && compareTextTestResultWith(suite, group, test, result.output, accepted)) {
    result.valresult = VALRESULT_ACCEPT;
    return;
  }
  result.valresult = VALRESULT_DIFF;
}

