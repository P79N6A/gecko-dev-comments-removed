























function StatTracker(pc, pollInterval) {
  pollInterval = pollInterval || 250;

  var dataTable = new google.visualization.DataTable();
  var timeColumnIndex = dataTable.addColumn('datetime', 'Time');
  var recording = true;

  
  
  
  var samplingFunctions = {};

  
  this.dataTable = function() { return dataTable; }

  
  
  
  this.recordStat = function (varName, recordName, statName) {
    var columnIndex = dataTable.addColumn('number', varName);
    samplingFunctions[varName] = function (report, rowIndex) {
      var sample;
      var record = report.namedItem(recordName);
      if (record) sample = record.stat(statName);
      dataTable.setCell(rowIndex, columnIndex, sample);
    }
  }

  
  this.stop = function() {
    recording = false;
  }

  
  
  
  
  function poll() {
    pc.getStats(function (report) {
      if (!recording) return;
      setTimeout(poll, pollInterval);
      var result = report.result();
      if (result.length < 1) return;

      var rowIndex = dataTable.addRow();
      dataTable.setCell(rowIndex, timeColumnIndex, result[0].timestamp);
      for (var v in samplingFunctions)
        samplingFunctions[v](report, rowIndex);
    });
  }
  setTimeout(poll, pollInterval);
}




function mergeDataTable(dataTable1, dataTable2) {
  function allColumns(cols) {
    var a = [];
    for (var i = 1; i < cols; ++i) a.push(i);
    return a;
  }
  return google.visualization.data.join(
      dataTable1,
      dataTable2,
      'full',
      [[0, 0]],
      allColumns(dataTable1.getNumberOfColumns()),
      allColumns(dataTable2.getNumberOfColumns()));
}
