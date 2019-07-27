





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

MemoryStats._nsIFile = function (pathname) {
    var f;
    var contractID = "@mozilla.org/file/local;1";
    try {
        f = Cc[contractID].createInstance(Ci.nsIFile);
    } catch(e) {
        f = SpecialPowers.Cc[contractID].createInstance(SpecialPowers.Ci.nsIFile);
    }
    f.initWithPath(pathname);
    return f;
}

MemoryStats.constructPathname = function (directory, basename) {
    var d = MemoryStats._nsIFile(directory);
    d.append(basename);
    return d.path;
}

MemoryStats.dump = function (logger,
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
            logger.info("MEMORY STAT " + stat + " after test: " + mrm[stat]);
        } else if (firstAccess) {
            logger.info("MEMORY STAT " + stat + " not supported in this build configuration.");
        }
    }

    if (dumpAboutMemory) {
        var basename = "about-memory-" + testNumber + ".json.gz";
        var dumpfile = MemoryStats.constructPathname(dumpOutputDirectory,
                                                     basename);
        logger.info(testURL + " | MEMDUMP-START " + dumpfile);
        var md = MemoryStats._getService("@mozilla.org/memory-info-dumper;1",
                                         "nsIMemoryInfoDumper");
        md.dumpMemoryReportsToNamedFile(dumpfile, function () {
            logger.info("TEST-INFO | " + testURL + " | MEMDUMP-END");
        }, null,  false);
    }

    if (dumpDMD && typeof(DMDReportAndDump) != undefined) {
        var basename = "dmd-" + testNumber + ".txt";
        var dumpfile = MemoryStats.constructPathname(dumpOutputDirectory,
                                                     basename);
        logger.info(testURL + " | DMD-DUMP " + dumpfile);
        DMDReportAndDump(dumpfile);
    }
};
