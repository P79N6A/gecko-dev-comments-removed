



let EXPORTED_SYMBOLS = ["TelemetryTimestamps"];

let TelemetryTimestamps = {
  timeStamps: {},
  add: function TT_add(name, value) {
    
    if (value == null)
      value = Date.now();

    if (isNaN(value))
      throw new Error("Value must be a timestamp");

    
    if (this.timeStamps.hasOwnProperty(name))
      return;

    this.timeStamps[name] = value;
  },
  get: function TT_get() {
    return JSON.parse(JSON.stringify(this.timeStamps));
  }
};
