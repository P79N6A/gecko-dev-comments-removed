



"use strict";

this.EXPORTED_SYMBOLS = ["convertToRTCStatsReport"];

function convertToRTCStatsReport(dict) {
  function appendStats(stats, report) {
    stats.forEach(function(stat) {
        report[stat.id] = stat;
      });
  }
  let report = {};
  appendStats(dict.inboundRTPStreamStats, report);
  appendStats(dict.outboundRTPStreamStats, report);
  appendStats(dict.mediaStreamTrackStats, report);
  appendStats(dict.mediaStreamStats, report);
  appendStats(dict.transportStats, report);
  appendStats(dict.iceComponentStats, report);
  appendStats(dict.iceCandidatePairStats, report);
  appendStats(dict.iceCandidateStats, report);
  appendStats(dict.codecStats, report);
  return report;
}

this.convertToRTCStatsReport = convertToRTCStatsReport;
