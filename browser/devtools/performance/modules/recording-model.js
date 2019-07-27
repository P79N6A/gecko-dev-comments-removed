


"use strict";

const { PerformanceIO } = require("devtools/performance/io");

const RECORDING_IN_PROGRESS = exports.RECORDING_IN_PROGRESS = -1;
const RECORDING_UNAVAILABLE = exports.RECORDING_UNAVAILABLE = null;





const RecordingModel = function (options={}) {
  this._front = options.front;
  this._performance = options.performance;
  this._label = options.label || "";
};

RecordingModel.prototype = {
  _localStartTime: RECORDING_UNAVAILABLE,
  _startTime: RECORDING_UNAVAILABLE,
  _endTime: RECORDING_UNAVAILABLE,
  _markers: [],
  _frames: [],
  _ticks: [],
  _memory: [],
  _profilerData: {},
  _label: "",
  _imported: false,
  _isRecording: false,

  





  importRecording: Task.async(function *(file) {
    let recordingData = yield PerformanceIO.loadRecordingFromFile(file);

    this._imported = true;
    this._label = recordingData.profilerData.profilerLabel || "";
    this._startTime = recordingData.interval.startTime;
    this._endTime = recordingData.interval.endTime;
    this._markers = recordingData.markers;
    this._frames = recordingData.frames;
    this._memory = recordingData.memory;
    this._ticks = recordingData.ticks;
    this._profilerData = recordingData.profilerData;

    return recordingData;
  }),

  





  exportRecording: Task.async(function *(file) {
    let recordingData = this.getAllData();
    yield PerformanceIO.saveRecordingToFile(recordingData, file);
  }),

  



  startRecording: Task.async(function *() {
    
    
    
    
    this._localStartTime = this._performance.now();

    let { startTime } = yield this._front.startRecording({
      withTicks: true,
      withMemory: true
    });
    this._isRecording = true;

    this._startTime = startTime;
    this._endTime = RECORDING_IN_PROGRESS;
    this._markers = [];
    this._frames = [];
    this._memory = [];
    this._ticks = [];
  }),

  



  stopRecording: Task.async(function *() {
    let results = yield this._front.stopRecording();
    this._isRecording = false;

    
    if (!results.endTime) {
      results.endTime = this._startTime + this.getLocalElapsedTime();
    }

    this._endTime = results.endTime;
    this._profilerData = results.profilerData;
    this._markers = this._markers.sort((a,b) => (a.start > b.start));

    return results;
  }),

  


  getLabel: function () {
    return this._label;
  },

  


  getLocalElapsedTime: function () {
    return this._performance.now() - this._localStartTime;
  },

  


  getDuration: function () {
    let { startTime, endTime } = this.getInterval();
    return endTime - startTime;
  },

  



  getInterval: function() {
    let startTime = this._startTime;
    let endTime = this._endTime;

    
    
    
    if (endTime == RECORDING_IN_PROGRESS) {
      endTime = startTime + this.getLocalElapsedTime();
    }

    return { startTime, endTime };
  },

  



  getMarkers: function() {
    return this._markers;
  },

  



  getFrames: function() {
    return this._frames;
  },

  



  getMemory: function() {
    return this._memory;
  },

  



  getTicks: function() {
    return this._ticks;
  },

  



  getProfilerData: function() {
    return this._profilerData;
  },

  


  getAllData: function() {
    let interval = this.getInterval();
    let markers = this.getMarkers();
    let frames = this.getFrames();
    let memory = this.getMemory();
    let ticks = this.getTicks();
    let profilerData = this.getProfilerData();
    return { interval, markers, frames, memory, ticks, profilerData };
  },

  



  isRecording: function () {
    return this._isRecording;
  },

  


  addTimelineData: function (eventName, ...data) {
    
    
    if (!this.isRecording()) {
      return;
    }

    switch (eventName) {
      
      case "markers":
        let [markers] = data;
        Array.prototype.push.apply(this._markers, markers);
        break;
      
      case "frames":
        let [, frames] = data;
        Array.prototype.push.apply(this._frames, frames);
        break;
      
      case "memory":
        let [delta, measurement] = data;
        this._memory.push({ delta, value: measurement.total / 1024 / 1024 });
        break;
      
      case "ticks":
        let [, timestamps] = data;
        this._ticks = timestamps;
        break;
    }
  }
};

exports.RecordingModel = RecordingModel;
