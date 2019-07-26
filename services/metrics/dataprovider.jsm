



"use strict";

this.EXPORTED_SYMBOLS = [
  "MetricsCollectionResult",
  "MetricsMeasurement",
  "MetricsProvider",
];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource://services-common/log4moz.js");





































this.MetricsMeasurement = function MetricsMeasurement(name, version) {
  if (!this.fields) {
    throw new Error("fields not defined on instance. You are likely using " +
                    "this type incorrectly.");
  }

  if (!name) {
    throw new Error("Must define a name for this measurement.");
  }

  if (!version) {
    throw new Error("Must define a version for this measurement.");
  }

  if (!Number.isInteger(version)) {
    throw new Error("version must be an integer: " + version);
  }

  this.name = name;
  this.version = version;

  this.values = new Map();
}

MetricsMeasurement.prototype = {
  




  TYPE_UINT32: {
    validate: function validate(value) {
      if (!Number.isInteger(value)) {
        throw new Error("UINT32 field expects an integer. Got " + value);
      }

      if (value < 0) {
        throw new Error("UINT32 field expects a positive integer. Got " + value);
      }

      if (value >= 0xffffffff) {
        throw new Error("Value is too large to fit within 32 bits: " + value);
      }
    },
  },

  




  TYPE_STRING: {
    validate: function validate(value) {
      if (typeof(value) != "string") {
        throw new Error("STRING field expects a string. Got " + typeof(value));
      }
    },
  },

  













  setValue: function setValue(name, value) {
    if (!this.fields[name]) {
      throw new Error("Attempting to set unknown field: " + name);
    }

    let type = this.fields[name].type;

    if (!(type in this)) {
      throw new Error("Unknown field type: " + type);
    }

    this[type].validate(value);
    this.values.set(name, value);
  },

  





  getValue: function getValue(name) {
    return this.values.get(name);
  },

  





  validate: function validate() {
    for (let field in this.fields) {
      let spec = this.fields[field];

      if (!spec.optional && !(field in this.values)) {
        throw new Error("Required field not defined: " + field);
      }
    }
  },

  toJSON: function toJSON() {
    let fields = {};
    for (let [k, v] of this.values) {
      fields[k] = v;
    }

    return {
      name: this.name,
      version: this.version,
      fields: fields,
    };
  },
};

Object.freeze(MetricsMeasurement.prototype);









































this.MetricsProvider = function MetricsProvider(name) {
  if (!name) {
    throw new Error("MetricsProvider must have a name.");
  }

  if (typeof(name) != "string") {
    throw new Error("name must be a string. Got: " + typeof(name));
  }

  this._log = Log4Moz.repository.getLogger("Services.Metrics.MetricsProvider");

  this.name = name;
}

MetricsProvider.prototype = {
  






  collectConstantMeasurements: function collectConstantMeasurements() {
    return null;
  },

  


  createResult: function createResult() {
    return new MetricsCollectionResult(this.name);
  },
};

Object.freeze(MetricsProvider.prototype);

































this.MetricsCollectionResult = function MetricsCollectionResult(name) {
  if (!name || typeof(name) != "string") {
    throw new Error("Must provide name argument to MetricsCollectionResult.");
  }

  this._log = Log4Moz.repository.getLogger("Services.Metrics.MetricsCollectionResult");

  this.name = name;

  this.measurements = new Map();
  this.expectedMeasurements = new Set();
  this.errors = [];

  this.populate = function populate() {
    throw new Error("populate() must be defined on MetricsCollectionResult " +
                    "instance.");
  };

  this._deferred = Promise.defer();
}

MetricsCollectionResult.prototype = {
  


  get missingMeasurements() {
    let missing = new Set();

    for (let name of this.expectedMeasurements) {
      if (this.measurements.has(name)) {
        continue;
      }

      missing.add(name);
    }

    return missing;
  },

  








  expectMeasurement: function expectMeasurement(name) {
    this.expectedMeasurements.add(name);
  },

  


  addMeasurement: function addMeasurement(data) {
    if (!(data instanceof MetricsMeasurement)) {
      throw new Error("addMeasurement expects a MetricsMeasurement instance.");
    }

    if (!this.expectedMeasurements.has(data.name)) {
      throw new Error("Not expecting this measurement: " + data.name);
    }

    if (this.measurements.has(data.name)) {
      throw new Error("Measurement of this name already present: " + data.name);
    }

    this.measurements.set(data.name, data);
  },

  





















  setValue: function setValue(name, field, value, rethrow=false) {
    let m = this.measurements.get(name);
    if (!m) {
      throw new Error("Attempting to operate on an undefined measurement: " +
                      name);
    }

    try {
      m.setValue(field, value);
      return true;
    } catch (ex) {
      this.addError(ex);

      if (rethrow) {
        throw ex;
      }

      return false;
    }
  },

  


  addError: function addError(error) {
    this.errors.push(error);
  },

  





  aggregate: function aggregate(other) {
    if (!(other instanceof MetricsCollectionResult)) {
      throw new Error("aggregate expects a MetricsCollectionResult instance.");
    }

    if (this.name != other.name) {
      throw new Error("Can only aggregate MetricsCollectionResult from " +
                      "the same provider. " + this.name + " != " + other.name);
    }

    for (let name of other.expectedMeasurements) {
      this.expectedMeasurements.add(name);
    }

    for (let [name, m] of other.measurements) {
      if (this.measurements.has(name)) {
        throw new Error("Incoming result has same measurement as us: " + name);
      }

      this.measurements.set(name, m);
    }

    this.errors = this.errors.concat(other.errors);
  },

  toJSON: function toJSON() {
    let o = {
      measurements: {},
      missing: [],
      errors: [],
    };

    for (let [name, value] of this.measurements) {
      o.measurements[name] = value;
    }

    for (let missing of this.missingMeasurements) {
      o.missing.push(missing);
    }

    for (let error of this.errors) {
      if (error.message) {
        o.errors.push(error.message);
      } else {
        o.errors.push(error);
      }
    }

    return o;
  },

  




  finish: function finish() {
    this._deferred.resolve(this);
  },

  







  onFinished: function onFinished(onFulfill, onError) {
    return this._deferred.promise.then(onFulfill, onError);
  },
};

Object.freeze(MetricsCollectionResult.prototype);

