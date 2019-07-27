


"use strict";


const SUCCESSFUL_OUTCOMES = [
  "GenericSuccess", "Inlined", "DOM", "Monomorphic", "Polymorphic"
];








































































































const OptimizationSite = exports.OptimizationSite = function (optimizations, optsIndex) {
  this.id = optsIndex;
  this.data = optimizations[optsIndex];
  this.samples = 0;
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










const JITOptimizations = exports.JITOptimizations = function (optimizations) {
  this._opts = optimizations;
  
  this._optSites = {};
};










JITOptimizations.prototype.addOptimizationSite = function (optsIndex) {
  let op = this._optSites[optsIndex] || (this._optSites[optsIndex] = new OptimizationSite(this._opts, optsIndex));
  op.samples++;
};







JITOptimizations.prototype.getOptimizationSites = function () {
  let opts = [];
  for (let opt of Object.keys(this._optSites)) {
    opts.push(this._optSites[opt]);
  }
  return opts.sort((a, b) => b.samples - a.samples);
};








OptimizationSite.isSuccessfulOutcome = JITOptimizations.isSuccessfulOutcome = function (outcome) {
  return !!~SUCCESSFUL_OUTCOMES.indexOf(outcome);
};
