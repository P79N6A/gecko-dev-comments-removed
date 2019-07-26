



"use strict";

#ifndef MERGED_COMPARTMENT
this.EXPORTED_SYMBOLS = ["ProviderManager"];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/services/metrics/dataprovider.jsm");
#endif

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/utils.js");








this.ProviderManager = function (storage) {
  this._log = Log4Moz.repository.getLogger("Services.Metrics.ProviderManager");

  this._providers = new Map();
  this._storage = storage;

  this._providerInitQueue = [];
  this._providerInitializing = false;
}

this.ProviderManager.prototype = Object.freeze({
  get providers() {
    let providers = [];
    for (let [name, entry] of this._providers) {
      providers.push(entry.provider);
    }

    return providers;
  },

  


  getProvider: function (name) {
    let provider = this._providers.get(name);

    if (!provider) {
      return null;
    }

    return provider.provider;
  },

  
















  registerProvider: function (provider) {
    if (!(provider instanceof Provider)) {
      throw new Error("Argument must be a Provider instance.");
    }

    if (this._providers.has(provider.name)) {
      return Promise.resolve();
    }

    let deferred = Promise.defer();
    this._providerInitQueue.push([provider, deferred]);

    if (this._providerInitQueue.length == 1) {
      this._popAndInitProvider();
    }

    return deferred.promise;
  },

  





  unregisterProvider: function (name) {
    this._providers.delete(name);
  },

  _popAndInitProvider: function () {
    if (!this._providerInitQueue.length || this._providerInitializing) {
      return;
    }

    let [provider, deferred] = this._providerInitQueue.shift();
    this._providerInitializing = true;

    this._log.info("Initializing provider with storage: " + provider.name);

    Task.spawn(function initProvider() {
      try {
        let result = yield provider.init(this._storage);
        this._log.info("Provider successfully initialized: " + provider.name);

        this._providers.set(provider.name, {
          provider: provider,
          constantsCollected: false,
        });

        deferred.resolve(result);
      } catch (ex) {
        this._recordProviderError(provider.name, "Failed to initialize", ex);
        deferred.reject(ex);
      } finally {
        this._providerInitializing = false;
        this._popAndInitProvider();
      }
    }.bind(this));
  },

  







  collectConstantData: function () {
    let entries = [];

    for (let [name, entry] of this._providers) {
      if (entry.constantsCollected) {
        this._log.trace("Provider has already provided constant data: " +
                        name);
        continue;
      }

      entries.push(entry);
    }

    let onCollect = function (entry, result) {
      entry.constantsCollected = true;
    };

    return this._callCollectOnProviders(entries, "collectConstantData",
                                        onCollect);
  },

  


  collectDailyData: function () {
    return this._callCollectOnProviders(this._providers.values(),
                                        "collectDailyData");
  },

  _callCollectOnProviders: function (entries, fnProperty, onCollect=null) {
    let promises = [];

    for (let entry of entries) {
      let provider = entry.provider;
      let collectPromise;
      try {
        collectPromise = provider[fnProperty].call(provider);
      } catch (ex) {
        this._recordProviderError(provider.name, "Exception when calling " +
                                  "collect function: " + fnProperty, ex);
        continue;
      }

      if (!collectPromise) {
        this._recordProviderError(provider.name, "Does not return a promise " +
                                  "from " + fnProperty + "()");
        continue;
      }

      let promise = collectPromise.then(function onCollected(result) {
        if (onCollect) {
          try {
            onCollect(entry, result);
          } catch (ex) {
            this._log.warn("onCollect callback threw: " +
                           CommonUtils.exceptionStr(ex));
          }
        }

        return Promise.resolve(result);
      });

      promises.push([provider.name, promise]);
    }

    return this._handleCollectionPromises(promises);
  },

  








  _handleCollectionPromises: function (promises) {
    return Task.spawn(function waitForPromises() {
      for (let [name, promise] of promises) {
        try {
          yield promise;
          this._log.debug("Provider collected successfully: " + name);
        } catch (ex) {
          this._recordProviderError(name, "Failed to collect", ex);
        }
      }

      throw new Task.Result(this);
    }.bind(this));
  },

  


  _recordProviderError: function (name, msg, ex) {
    let msg = "Provider error: " + name + ": " + msg;
    if (ex) {
      msg += ": " + ex.message;
    }
    this._log.warn(msg);

    if (this.onProviderError) {
      try {
        this.onProviderError(msg);
      } catch (callError) {
        this._log.warn("Exception when calling onProviderError callback: " +
                       CommonUtils.exceptionStr(callError));
      }
    }
  },
});

