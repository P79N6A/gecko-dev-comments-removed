



function MarionettePerfData() {
  this.perfData = {};
}
MarionettePerfData.prototype = {
  
















  addPerfData: function Marionette__addPerfData(testSuite, testName, data) {
    if (this.perfData[testSuite]) {
      if (this.perfData[testSuite][testName]) {
        this.perfData[testSuite][testName].push(data);
      }
      else {
        this.perfData[testSuite][testName.toString()] = [data];
      }
    }
    else {
      this.perfData[testSuite] = {}
      this.perfData[testSuite][testName.toString()] = [data];
    }
  },

  





  appendPerfData: function Marionette__appendPerfData(data) {
    for (var suite in data) {
      if (data.hasOwnProperty(suite)) {
        if (this.perfData[suite]) {
          for (var test in data[suite]) {
            if (this.perfData[suite][test]) {
              this.perfData[suite][test] = this.perfData[suite][test].concat(data[suite][test]);
            }
            else {
              this.perfData[suite][test] = data[suite][test];
            }
          }
        }
        else {
          this.perfData[suite] = data[suite];
        }
      }
    }
  },

  





  getPerfData: function Marionette__getPerfData() {
    return this.perfData;
  },

  


  clearPerfData: function Marionette_clearPerfData() {
    this.perfData = {};
  },
}
