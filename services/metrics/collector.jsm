



"use strict";

this.EXPORTED_SYMBOLS = ["MetricsCollector"];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://gre/modules/services/metrics/dataprovider.jsm");








this.MetricsCollector = function MetricsCollector() {
  this._log = Log4Moz.repository.getLogger("Metrics.MetricsCollector");

  this._providers = [];
  this.collectionResults = new Map();
}

MetricsCollector.prototype = {
  








  registerProvider: function registerProvider(provider) {
    if (!(provider instanceof MetricsProvider)) {
      throw new Error("argument must be a MetricsProvider instance.");
    }

    for (let p of this._providers) {
      if (p.provider == provider) {
        return;
      }
    }

    this._providers.push({
      provider: provider,
      constantsCollected: false,
    });
  },

  







  collectConstantMeasurements: function collectConstantMeasurements() {
    let promises = [];

    for (let provider of this._providers) {
      if (provider.constantsCollected) {
        this._log.trace("Provider has already provided constant data: " +
                        provider.name);
        continue;
      }

      let result = provider.provider.collectConstantMeasurements();
      if (!result) {
        this._log.trace("Provider does not provide constant data: " +
                        provider.name);
        continue;
      }

      
      let promise = result.onFinished(function onFinished(result) {
        provider.constantsCollected = true;

        return Promise.resolve(result);
      });

      promises.push(promise);
    }

    return this._handleCollectionPromises(promises);
  },

  





  _handleCollectionPromises: function _handleCollectionPromises(promises) {
    if (!promises.length) {
      return Promise.resolve(this);
    }

    let deferred = Promise.defer();
    let finishedCount = 0;

    let onResult = function onResult(result) {
      try {
        this._log.debug("Got result for " + result.name);

        if (this.collectionResults.has(result.name)) {
          this.collectionResults.get(result.name).aggregate(result);
        } else {
          this.collectionResults.set(result.name, result);
        }
      } finally {
        finishedCount++;
        if (finishedCount >= promises.length) {
          deferred.resolve(this);
        }
      }
    }.bind(this);

    let onError = function onError(error) {
      this._log.warn("Error when handling result: " +
                     CommonUtils.exceptionStr(error));
      deferred.reject(error);
    }.bind(this);

    for (let promise of promises) {
      promise.then(onResult, onError);
    }

    return deferred.promise;
  },
};

Object.freeze(MetricsCollector.prototype);

