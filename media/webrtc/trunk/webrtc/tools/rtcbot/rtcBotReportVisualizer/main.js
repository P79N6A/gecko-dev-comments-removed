







google.load("visualization", "1", {packages:["corechart"]});

function openFiles(event) {
  var files = event.target.files;
  readAndAnalyzeFiles(files)
}

function readAndAnalyzeFiles(files) {
  if(!files) {
    alert("No files have been selected!");
    return;
  }

  var reports = [];
  var filesNames = [];
  missingFiles = files.length;

  for(var i = 0; i < files.length; i++) {
    var reader = new FileReader();
    reader.onload = onReaderLoad.bind(reader, files[i].name);
    reader.readAsText(files[i]);
  }

  function onReaderLoad(fileName) {
    reports.push(JSON.parse(this.result));
    filesNames.push(fileName);

    missingFiles--;
    if(missingFiles == 0) {
      analyzeReports_(reports, filesNames);
    }
  }
}



function analyzeReports_(reports, filesNames) {
  filesNames.unshift(""); 

  
  analyzeRttData(reports, filesNames, "bot1");
  analyzeRttData(reports, filesNames, "bot2");

  
  analyzePacketsLostData(reports, filesNames, "bot1");
  analyzePacketsLostData(reports, filesNames, "bot2");

  
  analyzeData(reports, filesNames, "Available Send Bandwidth-bot1", "bot1",
      "bweforvideo", "googAvailableSendBandwidth");
  analyzeData(reports, filesNames, "Available Send Bandwidth-bot2", "bot2",
      "bweforvideo", "googAvailableSendBandwidth");

   
   analyzeData(reports, filesNames, "Available Receive Bandwidth-bot1", "bot1",
       "bweforvideo", "googAvailableReceiveBandwidth");
   analyzeData(reports, filesNames, "Available Receive Bandwidth-bot2", "bot2",
     "bweforvideo", "googAvailableReceiveBandwidth");

  drawSeparatorLine();
}

function analyzeRttData(reports, filesNames, botName) {
  var outPut = [];
  outPut.push(filesNames);

  var avergaData = ['Average Rtt x10'];
  var maxData = ['Max Rtt'];

  var average;
  var max;
  for(var index in reports) {
    average = getStateAverage(reports[index], botName, "Conn-audio-1-0",
      "googRtt");
    avergaData.push(average*10);

    max = getStateMax(reports[index], botName, "Conn-audio-1-0",
      "googRtt");
    maxData.push(max);
  }
  outPut.push(avergaData);
  outPut.push(maxData);

  drawChart("Rtt-" + botName, outPut);
}

function analyzePacketsLostData(reports, filesNames, botName) {
  var outPut = [];
  outPut.push(filesNames);

  var maxData = ['Max Send PacketsLost'];
  var max;
  for(var index in reports) {
    max = getStateMax(reports[index], botName, "ssrc_[0-9]+_send",
        "packetsLost");
    maxData.push(max);
  }
  outPut.push(maxData);

  drawChart("Send PacketsLost-" + botName, outPut);
}

function analyzeData(reports, filesNames, chartName, botName, reportId,
    statName) {
  var outPut = [];
  outPut.push(filesNames);

  var avergaData = ['Average ' + statName];
  var maxData = ['Max ' + statName];

  var average;
  var max;
  for(var index in reports) {
    average = getStateAverage(reports[index], botName, reportId, statName);
    avergaData.push(average);

    max = getStateMax(reports[index], botName, reportId, statName);
    maxData.push(max);
  }
  outPut.push(avergaData);
  outPut.push(maxData);

  drawChart(chartName, outPut);
}

function getStateAverage(reports, botName, reportId, statName) {
  var sum = 0;
  var count = 0;

  for (var index in reports) {
    var data = reports[index].data;
    if(index == 0 || !data.hasOwnProperty(botName))
      continue;

    var stats = data[botName];
    for (var key in stats) {
      if(key.search(reportId) != -1) {
        var value = parseInt(stats[key][statName]);
        sum += value;
        count++;
      }
    }
  }
  return Math.round(sum/count);
}

function getStateMax(reports, botName, reportId, statName) {
  var max = -1;

  for (var index in reports) {
    var data = reports[index].data;
    if(index == 0 || !data.hasOwnProperty(botName))
      continue;

    var stats = data[botName];
    for (var key in stats) {
      if(key.search(reportId) != -1) {
        var value = parseInt(stats[key][statName]);
        max = Math.max(value, max);
      }
    }
  }
  return max;
}

function drawChart(title, data) {
  var dataTable = google.visualization.arrayToDataTable(data);

  var options = {
    title: title,
  };

  var div = document.createElement('div');
  document.body.appendChild(div);

  var chart = new google.visualization.ColumnChart(div);
  chart.draw(dataTable, options);
}

function drawSeparatorLine()  {
  var hr = document.createElement('hr');
  document.body.appendChild(hr);
}
