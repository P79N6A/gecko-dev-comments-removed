



"use strict";

#ifndef MERGED_COMPARTMENT

this.EXPORTED_SYMBOLS = [
  "Measurement",
  "Provider",
];

const {utils: Cu} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

#endif

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");





































this.Measurement = function () {
  if (!this.name) {
    throw new Error("Measurement must have a name.");
  }

  if (!this.version) {
    throw new Error("Measurement must have a version.");
  }

  if (!Number.isInteger(this.version)) {
    throw new Error("Measurement's version must be an integer: " + this.version);
  }

  if (!this.fields) {
    throw new Error("Measurement must define fields.");
  }

  for (let [name, info] in Iterator(this.fields)) {
    if (!info) {
      throw new Error("Field does not contain metadata: " + name);
    }

    if (!info.type) {
      throw new Error("Field is missing required type property: " + name);
    }
  }

  this._log = Log4Moz.repository.getLogger("Services.Metrics.Measurement." + this.name);

  this.id = null;
  this.storage = null;
  this._fields = {};

  this._serializers = {};
  this._serializers[this.SERIALIZE_JSON] = {
    singular: this._serializeJSONSingular.bind(this),
    daily: this._serializeJSONDay.bind(this),
  };
}

Measurement.prototype = Object.freeze({
  SERIALIZE_JSON: "json",

  






























  serializer: function (format) {
    if (!(format in this._serializers)) {
      throw new Error("Don't know how to serialize format: " + format);
    }

    return this._serializers[format];
  },

  







  hasField: function (name) {
    return name in this.fields;
  },

  







  fieldID: function (name) {
    let entry = this._fields[name];

    if (!entry) {
      throw new Error("Unknown field: " + name);
    }

    return entry[0];
  },

  fieldType: function (name) {
    let entry = this._fields[name];

    if (!entry) {
      throw new Error("Unknown field: " + name);
    }

    return entry[1];
  },

  _configureStorage: function () {
    return Task.spawn(function configureFields() {
      for (let [name, info] in Iterator(this.fields)) {
        this._log.debug("Registering field: " + name + " " + info.type);

        let id = yield this.storage.registerField(this.id, name, info.type);
        this._fields[name] = [id, info.type];
      }
    }.bind(this));
  },

  
  
  
  
  
  
  
  
  
  
  

  














  incrementDailyCounter: function (field, date=new Date()) {
    return this.storage.incrementDailyCounterFromFieldID(this.fieldID(field),
                                                         date);
  },

  











  addDailyDiscreteNumeric: function (field, value, date=new Date()) {
    return this.storage.addDailyDiscreteNumericFromFieldID(
                          this.fieldID(field), value, date);
  },

  




  addDailyDiscreteText: function (field, value, date=new Date()) {
    return this.storage.addDailyDiscreteTextFromFieldID(
                          this.fieldID(field), value, date);
  },

  











  setLastNumeric: function (field, value, date=new Date()) {
    return this.storage.setLastNumericFromFieldID(this.fieldID(field), value,
                                                  date);
  },

  




  setLastText: function (field, value, date=new Date()) {
    return this.storage.setLastTextFromFieldID(this.fieldID(field), value,
                                               date);
  },

  











  setDailyLastNumeric: function (field, value, date=new Date()) {
    return this.storage.setDailyLastNumericFromFieldID(this.fieldID(field),
                                                       value, date);
  },

  




  setDailyLastText: function (field, value, date=new Date()) {
    return this.storage.setDailyLastTextFromFieldID(this.fieldID(field),
                                                    value, date);
  },

  
  
  

  









  getValues: function () {
    return this.storage.getMeasurementValues(this.id);
  },

  deleteLastNumeric: function (field) {
    return this.storage.deleteLastNumericFromFieldID(this.fieldID(field));
  },

  deleteLastText: function (field) {
    return this.storage.deleteLastTextFromFieldID(this.fieldID(field));
  },

  













  shouldIncludeField: function (field) {
    return field in this._fields;
  },

  _serializeJSONSingular: function (data) {
    let result = {"_v": this.version};

    for (let [field, data] of data) {
      
      if (!this.shouldIncludeField(field)) {
        continue;
      }

      let type = this.fieldType(field);

      switch (type) {
        case this.storage.FIELD_LAST_NUMERIC:
        case this.storage.FIELD_LAST_TEXT:
          result[field] = data[1];
          break;

        case this.storage.FIELD_DAILY_COUNTER:
        case this.storage.FIELD_DAILY_DISCRETE_NUMERIC:
        case this.storage.FIELD_DAILY_DISCRETE_TEXT:
        case this.storage.FIELD_DAILY_LAST_NUMERIC:
        case this.storage.FIELD_DAILY_LAST_TEXT:
          continue;

        default:
          throw new Error("Unknown field type: " + type);
      }
    }

    return result;
  },

  _serializeJSONDay: function (data) {
    let result = {"_v": this.version};

    for (let [field, data] of data) {
      if (!this.shouldIncludeField(field)) {
        continue;
      }

      let type = this.fieldType(field);

      switch (type) {
        case this.storage.FIELD_DAILY_COUNTER:
        case this.storage.FIELD_DAILY_DISCRETE_NUMERIC:
        case this.storage.FIELD_DAILY_DISCRETE_TEXT:
        case this.storage.FIELD_DAILY_LAST_NUMERIC:
        case this.storage.FIELD_DAILY_LAST_TEXT:
          result[field] = data;
          break;

        case this.storage.FIELD_LAST_NUMERIC:
        case this.storage.FIELD_LAST_TEXT:
          continue;

        default:
          throw new Error("Unknown field type: " + type);
      }
    }

    return result;
  },
});































































this.Provider = function () {
  if (!this.name) {
    throw new Error("Provider must define a name.");
  }

  if (!Array.isArray(this.measurementTypes)) {
    throw new Error("Provider must define measurement types.");
  }

  this._log = Log4Moz.repository.getLogger("Services.Metrics.Provider." + this.name);

  this.measurements = null;
  this.storage = null;
}

Provider.prototype = Object.freeze({
  













  pullOnly: false,

  




  getMeasurement: function (name, version) {
    if (!Number.isInteger(version)) {
      throw new Error("getMeasurement expects an integer version. Got: " + version);
    }

    let m = this.measurements.get([name, version].join(":"));

    if (!m) {
      throw new Error("Unknown measurement: " + name + " v" + version);
    }

    return m;
  },

  init: function (storage) {
    if (this.storage !== null) {
      throw new Error("Provider() not called. Did the sub-type forget to call it?");
    }

    if (this.storage) {
      throw new Error("Provider has already been initialized.");
    }

    this.measurements = new Map();
    this.storage = storage;

    let self = this;
    return Task.spawn(function init() {
      for (let measurementType of self.measurementTypes) {
        let measurement = new measurementType();

        measurement.provider = self;
        measurement.storage = self.storage;

        let id = yield storage.registerMeasurement(self.name, measurement.name,
                                                   measurement.version);

        measurement.id = id;

        yield measurement._configureStorage();

        self.measurements.set([measurement.name, measurement.version].join(":"),
                              measurement);
      }

      let promise = self.onInit();

      if (!promise || typeof(promise.then) != "function") {
        throw new Error("onInit() does not return a promise.");
      }

      yield promise;
    });
  },

  shutdown: function () {
    let promise = this.onShutdown();

    if (!promise || typeof(promise.then) != "function") {
      throw new Error("onShutdown implementation does not return a promise.");
    }

    return promise;
  },

  








  onInit: function () {
    return CommonUtils.laterTickResolvingPromise();
  },

  








  onShutdown: function () {
    return CommonUtils.laterTickResolvingPromise();
  },

  







  collectConstantData: function () {
    return CommonUtils.laterTickResolvingPromise();
  },

  











  collectDailyData: function () {
    return CommonUtils.laterTickResolvingPromise();
  },

  











  enqueueStorageOperation: function (func) {
    return this.storage.enqueueOperation(func);
  },

  

















  getState: function (key) {
    return this.storage.getProviderState(this.name, key);
  },

  





  setState: function (key, value) {
    return this.storage.setProviderState(this.name, key, value);
  },

  _dateToDays: function (date) {
    return Math.floor(date.getTime() / MILLISECONDS_PER_DAY);
  },

  _daysToDate: function (days) {
    return new Date(days * MILLISECONDS_PER_DAY);
  },
});

