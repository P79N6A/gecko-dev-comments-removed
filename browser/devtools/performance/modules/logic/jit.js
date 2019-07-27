


"use strict";


const SUCCESSFUL_OUTCOMES = [
  "GenericSuccess", "Inlined", "DOM", "Monomorphic", "Polymorphic"
];











































































































const OptimizationSite = function (id, opts) {
  this.id = id;
  this.data = opts;
  this.samples = 1;
};









OptimizationSite.prototype.hasSuccessfulOutcome = function () {
  let attempts = this.getAttempts();
  let lastOutcome = attempts[attempts.length - 1].outcome;
  return OptimizationSite.isSuccessfulOutcome(lastOutcome);
};







OptimizationSite.prototype.getAttempts = function () {
  return this.data.attempts;
};







OptimizationSite.prototype.getIonTypes = function () {
  return this.data.types;
};













const JITOptimizations = function (rawSites, stringTable) {
  
  let sites = [];

  for (let rawSite of rawSites) {
    let existingSite = sites.find((site) => site.data === rawSite);
    if (existingSite) {
      existingSite.samples++;
    } else {
      sites.push(new OptimizationSite(sites.length, rawSite));
    }
  }

  
  for (let site of sites) {
    let data = site.data;
    let STRATEGY_SLOT = data.attempts.schema.strategy;
    let OUTCOME_SLOT = data.attempts.schema.outcome;

    site.data = {
      attempts: data.attempts.data.map((a) => {
        return {
          strategy: stringTable[a[STRATEGY_SLOT]],
          outcome: stringTable[a[OUTCOME_SLOT]]
        }
      }),

      types: data.types.map((t) => {
        return {
          typeset: maybeTypeset(t.typeset, stringTable),
          site: stringTable[t.site],
          mirType: stringTable[t.mirType]
        };
      }),

      propertyName: maybeString(stringTable, data.propertyName),
      line: data.line,
      column: data.column
    };
  }

  this.optimizationSites = sites.sort((a, b) => b.samples - a.samples);
};




JITOptimizations.prototype = {
  [Symbol.iterator]: function *() {
    yield* this.optimizationSites;
  },

  get length() {
    return this.optimizationSites.length;
  }
};








OptimizationSite.isSuccessfulOutcome = JITOptimizations.isSuccessfulOutcome = function (outcome) {
  return !!~SUCCESSFUL_OUTCOMES.indexOf(outcome);
};

function maybeString(stringTable, index) {
  return index ? stringTable[index] : undefined;
}

function maybeTypeset(typeset, stringTable) {
  if (!typeset) {
    return undefined;
  }
  return typeset.map((ty) => {
    return {
      keyedBy: maybeString(stringTable, ty.keyedBy),
      name: maybeString(stringTable, ty.name),
      location: maybeString(stringTable, ty.location),
      line: ty.line
    };
  });
}


const IMPLEMENTATION_MAP = {
  "interpreter": 0,
  "baseline": 1,
  "ion": 2
};
const IMPLEMENTATION_NAMES = Object.keys(IMPLEMENTATION_MAP);




















function createTierGraphDataFromFrameNode (frameNode, sampleTimes, { startTime, endTime, resolution }) {
  if (!frameNode.hasOptimizations()) {
    return;
  }

  let tierData = frameNode.getOptimizationTierData();
  let duration = endTime - startTime;
  let stringTable = frameNode._stringTable;
  let output = [];
  let implEnum;

  let tierDataIndex = 0;
  let nextOptSample = tierData[tierDataIndex];

  
  let samplesInCurrentBucket = 0;
  let currentBucketStartTime = sampleTimes[0];
  let bucket = [];
  
  let bucketSize = Math.ceil(duration / resolution);

  
  for (let i = 0; i <= sampleTimes.length; i++) {
    let sampleTime = sampleTimes[i];

    
    
    if (sampleTime >= (currentBucketStartTime + bucketSize) ||
        i >= sampleTimes.length) {

      let dataPoint = {};
      dataPoint.ys = [];
      dataPoint.x = currentBucketStartTime;

      
      
      for (let j = 0; j < IMPLEMENTATION_NAMES.length; j++) {
        dataPoint.ys[j] = (bucket[j] || 0) / (samplesInCurrentBucket || 1);
      }
      output.push(dataPoint);

      
      currentBucketStartTime += bucketSize;
      samplesInCurrentBucket = 0;
      bucket = [];
    }

    
    if (nextOptSample && nextOptSample.time === sampleTime) {
      
      implEnum = IMPLEMENTATION_MAP[stringTable[nextOptSample.implementation] || "interpreter"];
      bucket[implEnum] = (bucket[implEnum] || 0) + 1;
      nextOptSample = tierData[++tierDataIndex];
    }

    samplesInCurrentBucket++;
  }

  return output;
}

exports.createTierGraphDataFromFrameNode = createTierGraphDataFromFrameNode;
exports.OptimizationSite = OptimizationSite;
exports.JITOptimizations = JITOptimizations;
