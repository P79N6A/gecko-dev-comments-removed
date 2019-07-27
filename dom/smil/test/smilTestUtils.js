







const SVG_NS = "http://www.w3.org/2000/svg";
const XLINK_NS = "http://www.w3.org/1999/xlink";

const MPATH_TARGET_ID = "smilTestUtilsTestingPath";

function extend(child, supertype)
{
   child.prototype.__proto__ = supertype.prototype;
}


var SMILUtil =
{
  
  getSVGRoot : function()
  {
    return SMILUtil.getFirstElemWithTag("svg");
  },

  
  getFirstElemWithTag : function(aTargetTag)
  {
    var elemList = document.getElementsByTagName(aTargetTag);
    return (elemList.length == 0 ? null : elemList[0]);
  },

  
  getComputedStyleSimple: function(elem, prop)
  {
    return window.getComputedStyle(elem, null).getPropertyValue(prop);
  },

  getAttributeValue: function(elem, attr)
  {
    if (attr.attrName == SMILUtil.getMotionFakeAttributeName()) {
      
      return elem.getCTM();
    }
    if (attr.attrType == "CSS") {
      return SMILUtil.getComputedStyleWrapper(elem, attr.attrName);
    }
    if (attr.attrType == "XML") {
      
      
      return SMILUtil.getComputedStyleWrapper(elem, attr.attrName);
    }
  },

  
  
  getComputedStyleWrapper : function(elem, propName)
  {
    
    
    var computedStyle;
    if (propName == "font") {
      var subProps = ["font-style", "font-variant-caps", "font-weight",
                      "font-size", "line-height", "font-family"];
      for (var i in subProps) {
        var subPropStyle = SMILUtil.getComputedStyleSimple(elem, subProps[i]);
        if (subPropStyle) {
          if (subProps[i] == "line-height") {
            
            subPropStyle = "/ " + subPropStyle;
          }
          if (!computedStyle) {
            computedStyle = subPropStyle;
          } else {
            computedStyle = computedStyle + " " + subPropStyle;
          }
        }
      }
    } else if (propName == "font-variant") {
      
      
      computedStyle = SMILUtil.getComputedStyleSimple(elem, "font-variant-caps");
    } else if (propName == "marker") {
      var subProps = ["marker-end", "marker-mid", "marker-start"];
      for (var i in subProps) {
        if (!computedStyle) {
          computedStyle = SMILUtil.getComputedStyleSimple(elem, subProps[i]);
        } else {
          is(computedStyle, SMILUtil.getComputedStyleSimple(elem, subProps[i]),
             "marker sub-properties should match each other " +
             "(they shouldn't be individually set)");
        }
      }
    } else if (propName == "overflow") {
      var subProps = ["overflow-x", "overflow-y"];
      for (var i in subProps) {
        if (!computedStyle) {
          computedStyle = SMILUtil.getComputedStyleSimple(elem, subProps[i]);
        } else {
          is(computedStyle, SMILUtil.getComputedStyleSimple(elem, subProps[i]),
             "overflow sub-properties should match each other " +
             "(they shouldn't be individually set)");
        }
      }
    } else {
      computedStyle = SMILUtil.getComputedStyleSimple(elem, propName);
    }
    return computedStyle;
  },
  
  
  
  hideSubtree : function(node, hideNodeItself, useXMLAttribute)
  {
    
    if (hideNodeItself) {
      if (useXMLAttribute) {
        if (node.setAttribute) {
          node.setAttribute("display", "none");
        }
      } else if (node.style) {
        node.style.display = "none";
      }
    }

    
    var child = node.firstChild;
    while (child) {
      SMILUtil.hideSubtree(child, true, useXMLAttribute);
      child = child.nextSibling;
    }
  },

  getMotionFakeAttributeName : function() {
    return "_motion";
  },
};


var CTMUtil =
{
  CTM_COMPONENTS_ALL    : ["a", "b", "c", "d", "e", "f"],
  CTM_COMPONENTS_ROTATE : ["a", "b", "c", "d" ],

  
  
  generateCTM : function(aCtmSummary)
  {
    if (!aCtmSummary || aCtmSummary.length != 3) {
      ok(false, "Unexpected CTM summary tuple length: " + aCtmSummary.length);
    }
    var tX = aCtmSummary[0];
    var tY = aCtmSummary[1];
    var theta = aCtmSummary[2];
    var cosTheta = Math.cos(theta);
    var sinTheta = Math.sin(theta);
    var newCtm = { a : cosTheta,  c: -sinTheta,  e: tX,
                   b : sinTheta,  d:  cosTheta,  f: tY  };
    return newCtm;
  },

  
  isWithinDelta : function(aTestVal, aExpectedVal, aErrMsg, aIsTodo) {
    var testFunc = aIsTodo ? todo : ok;
    const delta = 0.00001; 
    ok(aTestVal >= aExpectedVal - delta &&
       aTestVal <= aExpectedVal + delta,
       aErrMsg + " | got: " + aTestVal + ", expected: " + aExpectedVal);
  },

  assertCTMEqual : function(aLeftCtm, aRightCtm, aComponentsToCheck,
                            aErrMsg, aIsTodo) {
    var foundCTMDifference = false;
    for (var j in aComponentsToCheck) {
      var curComponent = aComponentsToCheck[j];
      if (!aIsTodo) {
        CTMUtil.isWithinDelta(aLeftCtm[curComponent], aRightCtm[curComponent],
                              aErrMsg + " | component: " + curComponent, false);
      } else if (aLeftCtm[curComponent] != aRightCtm[curComponent]) {
        foundCTMDifference = true;
      }
    }

    if (aIsTodo) {
      todo(!foundCTMDifference, aErrMsg + " | (currently marked todo)");
    }
  },

  assertCTMNotEqual : function(aLeftCtm, aRightCtm, aComponentsToCheck,
                               aErrMsg, aIsTodo) {
    
    var foundCTMDifference = false;
    for (var j in aComponentsToCheck) {
      var curComponent = aComponentsToCheck[j];
      if (aLeftCtm[curComponent] != aRightCtm[curComponent]) {
        foundCTMDifference = true;
        break; 
      }
    }

    if (aIsTodo) {
      todo(foundCTMDifference, aErrMsg + " | (currently marked todo)");
    } else {
      ok(foundCTMDifference, aErrMsg);
    }
  },
};



function SMILTimingData(aBegin, aDur)
{
  this._begin = aBegin;
  this._dur = aDur;
}
SMILTimingData.prototype =
{
  _begin: null,
  _dur: null,
  getBeginTime      : function() { return this._begin; },
  getDur            : function() { return this._dur; },
  getEndTime        : function() { return this._begin + this._dur; },
  getFractionalTime : function(aPortion)
  {
    return this._begin + aPortion * this._dur;
  },
}

















function Attribute(aAttrName, aAttrType, aTargetTag,
                   aIsAnimatable, aIsAdditive)
{
  this.attrName = aAttrName;
  this.attrType = aAttrType;
  this.targetTag = aTargetTag;
  this.isAnimatable = aIsAnimatable;
  this.isAdditive = aIsAdditive;
}
Attribute.prototype =
{
  
  attrName     : null,
  attrType     : null,
  isAnimatable : null,
  testcaseList : null,
};




function NonAnimatableAttribute(aAttrName, aAttrType, aTargetTag)
{
  return new Attribute(aAttrName, aAttrType, aTargetTag, false, false);
}
function NonAdditiveAttribute(aAttrName, aAttrType, aTargetTag)
{
  return new Attribute(aAttrName, aAttrType, aTargetTag, true, false);
}
function AdditiveAttribute(aAttrName, aAttrType, aTargetTag)
{
  return new Attribute(aAttrName, aAttrType, aTargetTag, true, true);
}







function TestcaseBundle(aAttribute, aTestcaseList, aSkipReason)
{
  this.animatedAttribute = aAttribute;
  this.testcaseList = aTestcaseList;
  this.skipReason = aSkipReason;
}
TestcaseBundle.prototype =
{
  
  animatedAttribute : null,
  testcaseList      : null,
  skipReason        : null,

  
  go : function(aTimingData) {
    if (this.skipReason) {
      todo(false, "Skipping a bundle for '" + this.animatedAttribute.attrName +
           "' because: " + this.skipReason);
    } else {
      
      if (!this.testcaseList || !this.testcaseList.length) {
        ok(false, "a bundle for '" + this.animatedAttribute.attrName +
           "' has no testcases");
      }

      var targetElem =
        SMILUtil.getFirstElemWithTag(this.animatedAttribute.targetTag);

      if (!targetElem) {
        ok(false, "Error: can't find an element of type '" +
           this.animatedAttribute.targetTag +
           "', so I can't test property '" +
           this.animatedAttribute.attrName + "'");
        return;
      }

      for (var testcaseIdx in this.testcaseList) {
        var testcase = this.testcaseList[testcaseIdx];
        if (testcase.skipReason) {
          todo(false, "Skipping a testcase for '" +
               this.animatedAttribute.attrName +
               "' because: " + testcase.skipReason);
        } else {
          testcase.runTest(targetElem, this.animatedAttribute,
                           aTimingData, false);
          testcase.runTest(targetElem, this.animatedAttribute,
                           aTimingData, true);
        }
      }
    }
  },
};





function AnimTestcase() {} 
AnimTestcase.prototype =
{
  
  _animElementTagName : "animate", 
  computedValMap      : null,
  skipReason          : null,
  
  
  










  runTest : function(aTargetElem, aTargetAttr, aTimeData, aIsFreeze)
  {
    
    if (!SMILUtil.getSVGRoot().animationsPaused()) {
      ok(false, "Should start each test with animations paused");
    }
    if (SMILUtil.getSVGRoot().getCurrentTime() != 0) {
      ok(false, "Should start each test at time = 0");
    }

    
    
    var baseVal = SMILUtil.getAttributeValue(aTargetElem, aTargetAttr);

    
    var anim = this.setupAnimationElement(aTargetAttr, aTimeData, aIsFreeze);
    aTargetElem.appendChild(anim);

    
    var seekList = this.buildSeekList(aTargetAttr, baseVal, aTimeData, aIsFreeze);

    
    this.seekAndTest(seekList, aTargetElem, aTargetAttr);

    
    aTargetElem.removeChild(anim);
    SMILUtil.getSVGRoot().setCurrentTime(0);
  },

  
  
  
  setupAnimationElement : function(aAnimAttr, aTimeData, aIsFreeze)
  {
    var animElement = document.createElementNS(SVG_NS,
                                               this._animElementTagName);
    animElement.setAttribute("attributeName", aAnimAttr.attrName);
    animElement.setAttribute("attributeType", aAnimAttr.attrType);
    animElement.setAttribute("begin", aTimeData.getBeginTime());
    animElement.setAttribute("dur", aTimeData.getDur());
    if (aIsFreeze) {
      animElement.setAttribute("fill", "freeze");
    }
    return animElement;
  },

  buildSeekList : function(aAnimAttr, aBaseVal, aTimeData, aIsFreeze)
  {
    if (!aAnimAttr.isAnimatable) {
      return this.buildSeekListStatic(aAnimAttr, aBaseVal, aTimeData,
                                      "defined as non-animatable in SVG spec");
    }
    if (this.computedValMap.noEffect) {
      return this.buildSeekListStatic(aAnimAttr, aBaseVal, aTimeData,
                                      "testcase specified to have no effect");
    }      
    return this.buildSeekListAnimated(aAnimAttr, aBaseVal,
                                      aTimeData, aIsFreeze)
  },

  seekAndTest : function(aSeekList, aTargetElem, aTargetAttr)
  {
    var svg = document.getElementById("svg");
    for (var i in aSeekList) {
      var entry = aSeekList[i];
      SMILUtil.getSVGRoot().setCurrentTime(entry[0]);
      is(SMILUtil.getAttributeValue(aTargetElem, aTargetAttr),
         entry[1], entry[2]);
    }
  },

  
  buildSeekListStatic : function(aAnimAttr, aBaseVal,
                                 aTimeData, aReasonStatic) {},
  buildSeekListAnimated : function(aAnimAttr, aBaseVal,
                                   aTimeData, aIsFreeze) {},
};



function AnimTestcaseFrom() {} 
AnimTestcaseFrom.prototype =
{
  
  from           : null,

  
  setupAnimationElement : function(aAnimAttr, aTimeData, aIsFreeze)
  {
    
    var animElem = AnimTestcase.prototype.setupAnimationElement.apply(this,
                                         [aAnimAttr, aTimeData, aIsFreeze]);
    animElem.setAttribute("from", this.from)
    return animElem;
  },

  buildSeekListStatic : function(aAnimAttr, aBaseVal, aTimeData, aReasonStatic)
  {
    var seekList = new Array();
    var msgPrefix = aAnimAttr.attrName +
      ": shouldn't be affected by animation ";
    seekList.push([aTimeData.getBeginTime(), aBaseVal,
                   msgPrefix + "(at animation begin) - " + aReasonStatic]);
    seekList.push([aTimeData.getFractionalTime(1/2), aBaseVal,
                   msgPrefix + "(at animation mid) - " + aReasonStatic]);
    seekList.push([aTimeData.getEndTime(), aBaseVal,
                   msgPrefix + "(at animation end) - " + aReasonStatic]);
    seekList.push([aTimeData.getEndTime() + aTimeData.getDur(), aBaseVal,
                   msgPrefix + "(after animation end) - " + aReasonStatic]);
    return seekList;
  },

  buildSeekListAnimated : function(aAnimAttr, aBaseVal, aTimeData, aIsFreeze)
  {
    var seekList = new Array();
    var msgPrefix = aAnimAttr.attrName + ": ";
    if (aTimeData.getBeginTime() > 0.1) {
      seekList.push([aTimeData.getBeginTime() - 0.1,
                    aBaseVal,
                     msgPrefix + "checking that base value is set " +
                     "before start of animation"]);
    }

    seekList.push([aTimeData.getBeginTime(),
                   this.computedValMap.fromComp || this.from,
                   msgPrefix + "checking that 'from' value is set " +
                   "at start of animation"]);
    seekList.push([aTimeData.getFractionalTime(1/2),
                   this.computedValMap.midComp ||
                   this.computedValMap.toComp || this.to,
                   msgPrefix + "checking value halfway through animation"]);

    var finalMsg;
    var expectedEndVal;
    if (aIsFreeze) {
      expectedEndVal = this.computedValMap.toComp || this.to;
      finalMsg = msgPrefix + "[freeze-mode] checking that final value is set ";
    } else {
      expectedEndVal = aBaseVal;
      finalMsg = msgPrefix +
        "[remove-mode] checking that animation is cleared ";
    }
    seekList.push([aTimeData.getEndTime(),
                   expectedEndVal, finalMsg + "at end of animation"]);
    seekList.push([aTimeData.getEndTime() + aTimeData.getDur(),
                   expectedEndVal, finalMsg + "after end of animation"]);
    return seekList;
  },
}
extend(AnimTestcaseFrom, AnimTestcase);



















function AnimTestcaseFromTo(aFrom, aTo, aComputedValMap, aSkipReason)
{
  this.from           = aFrom;
  this.to             = aTo;
  this.computedValMap = aComputedValMap || {}; 
  this.skipReason     = aSkipReason;
}
AnimTestcaseFromTo.prototype =
{
  
  to : null,

  
  setupAnimationElement : function(aAnimAttr, aTimeData, aIsFreeze)
  {
    
    var animElem = AnimTestcaseFrom.prototype.setupAnimationElement.apply(this,
                                            [aAnimAttr, aTimeData, aIsFreeze]);
    animElem.setAttribute("to", this.to)
    return animElem;
  },
}
extend(AnimTestcaseFromTo, AnimTestcaseFrom);





















function AnimTestcaseFromBy(aFrom, aBy, aComputedValMap, aSkipReason)
{
  this.from           = aFrom;
  this.by             = aBy;
  this.computedValMap = aComputedValMap;
  this.skipReason     = aSkipReason;
  if (this.computedValMap &&
      !this.computedValMap.noEffect && !this.computedValMap.toComp) {
    ok(false, "AnimTestcaseFromBy needs expected computed final value");
  }
}
AnimTestcaseFromBy.prototype =
{
  
  by : null,

  
  setupAnimationElement : function(aAnimAttr, aTimeData, aIsFreeze)
  {
    
    var animElem = AnimTestcaseFrom.prototype.setupAnimationElement.apply(this,
                                            [aAnimAttr, aTimeData, aIsFreeze]);
    animElem.setAttribute("by", this.by)
    return animElem;
  },
  buildSeekList : function(aAnimAttr, aBaseVal, aTimeData, aIsFreeze)
  {
    if (!aAnimAttr.isAdditive) {
      return this.buildSeekListStatic(aAnimAttr, aBaseVal, aTimeData,
                                      "defined as non-additive in SVG spec");
    }
    
    return AnimTestcaseFrom.prototype.buildSeekList.apply(this,
                                [aAnimAttr, aBaseVal, aTimeData, aIsFreeze]);
  },
}
extend(AnimTestcaseFromBy, AnimTestcaseFrom);

























function AnimTestcasePaced(aValuesString, aComputedValMap, aSkipReason)
{
  this.valuesString   = aValuesString;
  this.computedValMap = aComputedValMap;
  this.skipReason     = aSkipReason;
  if (this.computedValMap &&
      (!this.computedValMap.comp0 ||
       !this.computedValMap.comp1_6 ||
       !this.computedValMap.comp1_3 ||
       !this.computedValMap.comp2_3 ||
       !this.computedValMap.comp1)) {
    ok(false, "This AnimTestcasePaced has an incomplete computed value map");
  }
}
AnimTestcasePaced.prototype =
{
  
  valuesString : null,
  
  
  setupAnimationElement : function(aAnimAttr, aTimeData, aIsFreeze)
  {
    
    var animElem = AnimTestcase.prototype.setupAnimationElement.apply(this,
                                            [aAnimAttr, aTimeData, aIsFreeze]);
    animElem.setAttribute("values", this.valuesString)
    animElem.setAttribute("calcMode", "paced");
    return animElem;
  },
  buildSeekListAnimated : function(aAnimAttr, aBaseVal, aTimeData, aIsFreeze)
  {
    var seekList = new Array();
    var msgPrefix = aAnimAttr.attrName + ": checking value ";
    seekList.push([aTimeData.getBeginTime(),
                   this.computedValMap.comp0,
                   msgPrefix + "at start of animation"]);
    seekList.push([aTimeData.getFractionalTime(1/6),
                   this.computedValMap.comp1_6,
                   msgPrefix + "1/6 of the way through animation."]);
    seekList.push([aTimeData.getFractionalTime(1/3),
                   this.computedValMap.comp1_3,
                   msgPrefix + "1/3 of the way through animation."]);
    seekList.push([aTimeData.getFractionalTime(2/3),
                   this.computedValMap.comp2_3,
                   msgPrefix + "2/3 of the way through animation."]);

    var finalMsg;
    var expectedEndVal;
    if (aIsFreeze) {
      expectedEndVal = this.computedValMap.comp1;
      finalMsg = aAnimAttr.attrName +
        ": [freeze-mode] checking that final value is set ";
    } else {
      expectedEndVal = aBaseVal;
      finalMsg = aAnimAttr.attrName +
        ": [remove-mode] checking that animation is cleared ";
    }
    seekList.push([aTimeData.getEndTime(),
                   expectedEndVal, finalMsg + "at end of animation"]);
    seekList.push([aTimeData.getEndTime() + aTimeData.getDur(),
                   expectedEndVal, finalMsg + "after end of animation"]);
    return seekList;
  },
  buildSeekListStatic : function(aAnimAttr, aBaseVal, aTimeData, aReasonStatic)
  {
    var seekList = new Array();
    var msgPrefix =
      aAnimAttr.attrName + ": shouldn't be affected by animation ";
    seekList.push([aTimeData.getBeginTime(), aBaseVal,
                   msgPrefix + "(at animation begin) - " + aReasonStatic]);
    seekList.push([aTimeData.getFractionalTime(1/6), aBaseVal,
                   msgPrefix + "(1/6 of the way through animation) - " +
                   aReasonStatic]);
    seekList.push([aTimeData.getFractionalTime(1/3), aBaseVal,
                   msgPrefix + "(1/3 of the way through animation) - " +
                   aReasonStatic]);
    seekList.push([aTimeData.getFractionalTime(2/3), aBaseVal,
                   msgPrefix + "(2/3 of the way through animation) - " +
                   aReasonStatic]);
    seekList.push([aTimeData.getEndTime(), aBaseVal,
                   msgPrefix + "(at animation end) - " + aReasonStatic]);
    seekList.push([aTimeData.getEndTime() + aTimeData.getDur(), aBaseVal,
                   msgPrefix + "(after animation end) - " + aReasonStatic]);
    return seekList;
  },
};
extend(AnimTestcasePaced, AnimTestcase);



























function AnimMotionTestcase(aAttrValueHash, aCtmMap, aSkipReason)
{
  this.attrValueHash = aAttrValueHash;
  this.ctmMap        = aCtmMap;
  this.skipReason    = aSkipReason;
  if (this.ctmMap &&
      (!this.ctmMap.ctm0 ||
       !this.ctmMap.ctm1_6 ||
       !this.ctmMap.ctm1_3 ||
       !this.ctmMap.ctm2_3 ||
       !this.ctmMap.ctm1)) {
    ok(false, "This AnimMotionTestcase has an incomplete CTM map");
  }
}
AnimMotionTestcase.prototype =
{
  
  _animElementTagName : "animateMotion",
  
  
  
  setupAnimationElement : function(aAnimAttr, aTimeData, aIsFreeze)
  {
    var animElement = document.createElementNS(SVG_NS,
                                               this._animElementTagName);
    animElement.setAttribute("begin", aTimeData.getBeginTime());
    animElement.setAttribute("dur", aTimeData.getDur());
    if (aIsFreeze) {
      animElement.setAttribute("fill", "freeze");
    }
    for (var attrName in this.attrValueHash) {
      if (attrName == "mpath") {
        this.createPath(this.attrValueHash[attrName]);
        this.createMpath(animElement);
      } else {
        animElement.setAttribute(attrName, this.attrValueHash[attrName]);
      }
    }
    return animElement;
  },

  createPath : function(aPathDescription)
  {
    var path = document.createElementNS(SVG_NS, "path");
    path.setAttribute("d", aPathDescription);
    path.setAttribute("id", MPATH_TARGET_ID);
    return SMILUtil.getSVGRoot().appendChild(path);
  },

  createMpath : function(aAnimElement)
  {
    var mpath = document.createElementNS(SVG_NS, "mpath");
    mpath.setAttributeNS(XLINK_NS, "href", "#" + MPATH_TARGET_ID);
    return aAnimElement.appendChild(mpath);
  },

  
  
  
  buildSeekList : function(aAnimAttr, aBaseVal, aTimeData, aIsFreeze)
  {
    var seekList = new Array();
    var msgPrefix = "CTM mismatch ";
    seekList.push([aTimeData.getBeginTime(),
                   CTMUtil.generateCTM(this.ctmMap.ctm0),
                   msgPrefix + "at start of animation"]);
    seekList.push([aTimeData.getFractionalTime(1/6),
                   CTMUtil.generateCTM(this.ctmMap.ctm1_6),
                   msgPrefix + "1/6 of the way through animation."]);
    seekList.push([aTimeData.getFractionalTime(1/3),
                   CTMUtil.generateCTM(this.ctmMap.ctm1_3),
                   msgPrefix + "1/3 of the way through animation."]);
    seekList.push([aTimeData.getFractionalTime(2/3),
                   CTMUtil.generateCTM(this.ctmMap.ctm2_3),
                   msgPrefix + "2/3 of the way through animation."]);

    var finalMsg;
    var expectedEndVal;
    if (aIsFreeze) {
      expectedEndVal = CTMUtil.generateCTM(this.ctmMap.ctm1);
      finalMsg = aAnimAttr.attrName +
        ": [freeze-mode] checking that final value is set ";
    } else {
      expectedEndVal = aBaseVal;
      finalMsg = aAnimAttr.attrName +
        ": [remove-mode] checking that animation is cleared ";
    }
    seekList.push([aTimeData.getEndTime(),
                   expectedEndVal, finalMsg + "at end of animation"]);
    seekList.push([aTimeData.getEndTime() + aTimeData.getDur(),
                   expectedEndVal, finalMsg + "after end of animation"]);
    return seekList;
  },

  
  
  
  seekAndTest : function(aSeekList, aTargetElem, aTargetAttr)
  {
    var svg = document.getElementById("svg");
    for (var i in aSeekList) {
      var entry = aSeekList[i];
      SMILUtil.getSVGRoot().setCurrentTime(entry[0]);
      CTMUtil.assertCTMEqual(aTargetElem.getCTM(), entry[1],
                             CTMUtil.CTM_COMPONENTS_ALL, entry[2], false);
    }
  },

  
  
  runTest : function(aTargetElem, aTargetAttr, aTimeData, aIsFreeze)
  {
    AnimTestcase.prototype.runTest.apply(this,
                             [aTargetElem, aTargetAttr, aTimeData, aIsFreeze]);
    var pathElem = document.getElementById(MPATH_TARGET_ID);
    if (pathElem) {
      SMILUtil.getSVGRoot().removeChild(pathElem);
    }
  }
};
extend(AnimMotionTestcase, AnimTestcase);


function testBundleList(aBundleList, aTimingData)
{
  for (var bundleIdx in aBundleList) {
    aBundleList[bundleIdx].go(aTimingData);
  }
}
