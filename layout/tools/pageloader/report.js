








































function Report(pages) {
  this.pages = pages;
  this.timeVals = new Array(pages.length);  
  for (var i = 0; i < this.timeVals.length; ++i) {
    this.timeVals[i] = new Array();
  }
  this.totalCCTime = 0;
  this.showTotalCCTime = false;
}


function findCommonPrefixLength(strs) {
  if (strs.length < 2)
    return 0;

  var len = 0;
  do {
    var newlen = len + 1;
    var newprefix = null;
    var failed = false;
    for (var i = 0; i < strs.length; i++) {
      if (newlen > strs[i].length) {
	failed = true;
	break;
      }

      var s = strs[i].substr(0, newlen);
      if (newprefix == null) {
	newprefix = s;
      } else if (newprefix != s) {
	failed = true;
	break;
      }
    }

    if (failed)
      break;

    len++;
  } while (true);
  return len;
}

function compareNumbers(a, b) {
  return a - b;
}









function getArrayStats(ary) {
  var r = {};
  r.min = ary[0];
  r.max = ary[0];
  r.indexOfMax = 0;
  var sum = 0;
  for (var i = 0; i < ary.length; ++i) {
    if (ary[i] < r.min) {
      r.min = ary[i];
    } else if (ary[i] > r.max) {
      r.max = ary[i];
      r.indexOfMax = i;
    }
    sum = sum + ary[i];
  }

  
  if (ary.length > 1) {
      sorted_ary = ary.concat();
      sorted_ary.sort(compareNumbers);
      
      sorted_ary.pop();
      if (sorted_ary.length%2) {
        r.median = sorted_ary[(sorted_ary.length-1)/2]; 
      }else{
        var n = Math.floor(sorted_ary.length / 2);
        if (n >= sorted_ary.length)
          r.median = sorted_ary[n];
        else
          r.median = (sorted_ary[n-1] + sorted_ary[n]) / 2;
      }
  }else{
    r.median = ary[0];
  }

  
  if (ary.length > 1)
    r.mean = (sum - r.max) / (ary.length - 1);
  else
    r.mean = ary[0];

  r.vari = 0;
  for (var i = 0; i < ary.length; ++i) {
    if (i == r.indexOfMax)
      continue;
    var d = r.mean - ary[i];
    r.vari = r.vari + d * d;
  }

  if (ary.length > 1) {
    r.vari = r.vari / (ary.length - 1);
    r.stdd = Math.sqrt(r.vari);
  } else {
    r.vari = 0.0;
    r.stdd = 0.0;
  }
  return r;
}

function strPad(o, len, left) {
  var str = o.toString();
  if (!len)
    len = 6;
  if (left == null)
    left = true;

  if (str.length < len) {
    len -= str.length;
    while (--len) {
      if (left)
	str = " " + str;
      else
	str += " ";
    }
  }

  str += " ";
  return str;
}

function strPadFixed(n, len, left) {
  return strPad(n.toFixed(0), len, left);
}

Report.prototype.getReport = function(format) {
  
  var avgs = new Array();
  var medians = new Array();
  for (var i = 0; i < this.timeVals.length; ++i) {
     avgs[i] = getArrayStats(this.timeVals[i]).mean;
     medians[i] = getArrayStats(this.timeVals[i]).median;
  }
  var avg = getArrayStats(avgs).mean;
  var avgmed = getArrayStats(medians).mean;

  var report;

  var prefixLen = findCommonPrefixLength(this.pages);

  if (format == "js") {
    
    
    report = "([";
    for (var i = 0; i < this.timeVals.length; i++) {
      var stats = getArrayStats(this.timeVals[i]);
      report += uneval({ page: this.pages[i].substr(prefixLen), value: stats.mean, stddev: stats.stdd});
      report += ",";
    }
    report += "])";
  } else if (format == "jsfull") {
    
  } else if (format == "text") {
    
    report = "============================================================\n";
    report += "    " + strPad("Page", 40, false) + strPad("mean") + strPad("stdd") + strPad("min") + strPad("max") + "raw" + "\n";
    for (var i = 0; i < this.timeVals.length; i++) {
      var stats = getArrayStats(this.timeVals[i]);
      report +=
        strPad(i, 4, true) +
        strPad(this.pages[i].substr(prefixLen), 40, false) +
        strPadFixed(stats.mean) +
        strPadFixed(stats.stdd) +
        strPadFixed(stats.min) +
        strPadFixed(stats.max) +
        this.timeVals[i] +
        "\n";
    }
    if (this.showTotalCCTime) {
      report += "Cycle collection: " + this.totalCCTime + "\n"
    }
    report += "============================================================\n";
  } else if (format == "tinderbox") {
    report = "__start_tp_report\n";
    report += "_x_x_mozilla_page_load,"+avgmed+",NaN,NaN\n";  
    report += "_x_x_mozilla_page_load_details,avgmedian|"+avgmed+"|average|"+avg.toFixed(2)+"|minimum|NaN|maximum|NaN|stddev|NaN\n";
    report += "|i|pagename|median|mean|min|max|runs|\n";

    for (var i = 0; i < this.timeVals.length; i++) {
      var r = getArrayStats(this.timeVals[i]);
      report += '|'+
        i + ';'+
        this.pages[i].substr(prefixLen) + ';'+
        r.median + ';'+
        r.mean + ';'+
        r.min + ';'+
        r.max + ';'+
        this.timeVals[i].join(";") +
        "\n";
    }
    report += "__end_tp_report\n";
    if (this.showTotalCCTime) {
      report += "__start_cc_report\n";
      report += "_x_x_mozilla_cycle_collect," + this.totalCCTime + "\n";
      report += "__end_cc_report\n";
    }
    var now = (new Date()).getTime();
    report += "__startTimestamp" + now + "__endTimestamp\n"; 
  } else {
    report = "Unknown report format";
  }

  return report;
}

Report.prototype.recordTime = function(pageIndex, ms) {
  this.timeVals[pageIndex].push(ms);
}

Report.prototype.recordCCTime = function(ms) {
  this.totalCCTime += ms;
  this.showTotalCCTime = true;
}
