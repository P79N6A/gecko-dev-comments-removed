





var MemoryStats = {};









var MEM_STAT_UNKNOWN = 0;
var MEM_STAT_UNSUPPORTED = 1;
var MEM_STAT_SUPPORTED = 2;

MemoryStats._hasMemoryStatistics = {}
MemoryStats._hasMemoryStatistics.vsize = MEM_STAT_UNKNOWN;
MemoryStats._hasMemoryStatistics.vsizeMaxContiguous = MEM_STAT_UNKNOWN;
MemoryStats._hasMemoryStatistics.residentFast = MEM_STAT_UNKNOWN;
MemoryStats._hasMemoryStatistics.heapAllocated = MEM_STAT_UNKNOWN;

MemoryStats._getService = function (className, interfaceName) {
    var service;
    try {
        service = Cc[className].getService(Ci[interfaceName]);
    } catch (e) {
        service = SpecialPowers.Cc[className]
                               .getService(SpecialPowers.Ci[interfaceName]);
    }
    return service;
}

MemoryStats.dump = function (dumpFn,
                             testNumber,
                             testURL,
                             dumpOutputDirectory,
                             dumpAboutMemory,
                             dumpDMD) {
    var mrm = MemoryStats._getService("@mozilla.org/memory-reporter-manager;1",
                                      "nsIMemoryReporterManager");
    for (var stat in MemoryStats._hasMemoryStatistics) {
        var supported = MemoryStats._hasMemoryStatistics[stat];
        var firstAccess = false;
        if (supported == MEM_STAT_UNKNOWN) {
            firstAccess = true;
            try {
                var value = mrm[stat];
                supported = MEM_STAT_SUPPORTED;
            } catch (e) {
                supported = MEM_STAT_UNSUPPORTED;
            }
            MemoryStats._hasMemoryStatistics[stat] = supported;
        }
        if (supported == MEM_STAT_SUPPORTED) {
            dumpFn("TEST-INFO | MEMORY STAT " + stat + " after test: " + mrm[stat]);
        } else if (firstAccess) {
            dumpFn("TEST-INFO | MEMORY STAT " + stat + " not supported in this build configuration.");
        }
    }

    if (dumpAboutMemory) {
        var dumpfile = dumpOutputDirectory + "/about-memory-" + testNumber + ".json.gz";
        dumpFn("TEST-INFO | " + testURL + " | MEMDUMP-START " + dumpfile);
        var md = MemoryStats._getService("@mozilla.org/memory-info-dumper;1",
                                         "nsIMemoryInfoDumper");
        md.dumpMemoryReportsToNamedFile(dumpfile, function () {
            dumpFn("TEST-INFO | " + testURL + " | MEMDUMP-END");
        }, null);

    }

    if (dumpDMD && typeof(DMDReportAndDump) != undefined) {
        var dumpfile = dumpOutputDirectory + "/dmd-" + testNumber + ".txt";
        dumpFn("TEST-INFO | " + testURL + " | DMD-DUMP " + dumpfile);
        DMDReportAndDump(dumpfile);
    }
};
