


"use strict";


let mochitestDir = getRootDirectory(gTestPath).replace('/mochiperf', '/mochitest');
Services.scriptloader.loadSubScript(mochitestDir + "head.js", this);


const kInfoHeader = "PERF-TEST | ";
const kDeclareId = "DECLARE ";
const kResultsId = "RESULTS ";


const kDataSetVersion = "1";





var PerfTest = {
  _userStartTime: 0,
  _userStopTime: 0,

  



  














  declareTest: function declareTest(aUUID, aName, aCategory, aSubCategory, aDescription) {
    this._uid = aUUID;
    this._print(kDeclareId, this._toJsonStr({
      id: aUUID,
      version: kDataSetVersion,
      name: aName,
      category: aCategory,
      subcategory: aSubCategory,
      description: aDescription,
      buildid: Services.appinfo.appBuildID,
    }));
  },

  







  declareNumericalResult: function declareNumericalResult(aValue, aDescription) {
    this._print(kResultsId, this._toJsonStr({
      id: this._uid,
      version: kDataSetVersion,
      results: {
        r0: {
          value: aValue,
          desc: aDescription
        }
      },
    }));
  },

  








  declareFrameRateResult: function declareFrameRateResult(aFrameCount, aRunMs, aDescription) {
    this._print(kResultsId, this._toJsonStr({
      id: this._uid,
      version: kDataSetVersion,
      results: {
        r0: {
          value: (aFrameCount / (aRunMs / 1000.0)),
          desc: aDescription
        }
      },
    }));
  },

  












  declareNumericalResults: function declareNumericalResults(aArray) {
    let collection = new Object();
    for (let idx = 0; idx < aArray.length; idx++) {
      collection['r' + idx] = { value: aArray[idx].value, desc: aArray[idx].desc };
      if (aArray[idx].shareAxis != undefined) {
        collection['r' + idx].shareAxis = aArray[idx].shareAxis;
      }
    }
    let dataset = {
      id: this._uid,
      version: kDataSetVersion,
      results: collection
    };
    this._print(kResultsId, this._toJsonStr(dataset));
  },

  



  perfBoundsCheck: function perfBoundsCheck(aLow, aHigh, aValue, aTestMessage) {
    ok(aValue < aLow || aValue > aHigh, aTestMessage);
  },

  



  computeMedian: function computeMedian(aArray, aOptions) {
    aArray.sort(function (a, b) {
      return a - b;
    });
 
    var idx = Math.floor(aArray.length / 2);
 
    if(aArray.length % 2) {
        return aArray[idx];
    } else {
        return (aArray[idx-1] + aArray[idx]) / 2;
    }
  },

  computeAverage: function computeAverage(aArray, aOptions) {
    let idx;
    let count = 0, total = 0;
    let highIdx = -1, lowIdx = -1;
    let high = 0, low = 0;
    if (aOptions.stripOutliers) {
      for (idx = 0; idx < aArray.length; idx++) {
        if (high < aArray[idx]) {
          highIdx = idx;
          high = aArray[idx];
        }
        if (low > aArray[idx]) {
          lowIdx = idx;
          low = aArray[idx];
        }
      }
    }
    for (idx = 0; idx < aArray.length; idx++) {
      if (idx != high && idx != low) {
        total += aArray[idx];
        count++;
      }
    }
    return total / count;
  },

  



  _print: function _print() {
    let str = kInfoHeader;
    for (let idx = 0; idx < arguments.length; idx++) {
      str += arguments[idx];
    }
    info(str);
  },

  _toJsonStr: function _toJsonStr(aTable) {
    return window.JSON.stringify(aTable);
  },
};





function StopWatch(aStart) {
  if (aStart) {
    this.start();
  }
}

StopWatch.prototype = {
  


  start: function start() {
    this.reset();
    this._userStartTime = window.performance.now();
  },

  


  stop: function stop() {
    this._userStopTime = window.performance.now();
    return this.time();
  },

  


  reset: function reset() {
    this._userStartTime = this._userStopTime = 0;
  },

  



  time: function time() {
    if (!this._userStartTime) {
      return 0;
    }
    if (!this._userStopTime) {
      return (window.performance.now() - this._userStartTime);
    }
    return this._userStopTime - this._userStartTime;
  },
};
