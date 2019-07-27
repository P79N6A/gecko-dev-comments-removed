




import os
import re


def _raw_log():
    import logging
    return logging.getLogger(__name__)


def process_single_leak_file(leakLogFileName, processType, leakThreshold,
                             ignoreMissingLeaks, log=None):
    """Process a single leak log.
    """

    
    
    
    
    lineRe = re.compile(r"^\s*\d+ \|"
                        r"(?P<name>[^|]+)\|"
                        r"\s*(?P<size>-?\d+)\s+(?P<bytesLeaked>-?\d+)\s*\|"
                        r"\s*-?\d+\s+(?P<numLeaked>-?\d+)")
    

    log = log or _raw_log()

    processString = "%s process:" % processType
    crashedOnPurpose = False
    totalBytesLeaked = None
    logAsWarning = False
    leakAnalysis = []
    leakedObjectAnalysis = []
    leakedObjectNames = []
    recordLeakedObjects = False
    with open(leakLogFileName, "r") as leaks:
        for line in leaks:
            if line.find("purposefully crash") > -1:
                crashedOnPurpose = True
            matches = lineRe.match(line)
            if not matches:
                
                log.info(line.rstrip())
                continue
            name = matches.group("name").rstrip()
            size = int(matches.group("size"))
            bytesLeaked = int(matches.group("bytesLeaked"))
            numLeaked = int(matches.group("numLeaked"))
            
            
            if numLeaked != 0 or name == "TOTAL":
                log.info(line.rstrip())
            
            
            if name == "TOTAL":
                
                
                
                if totalBytesLeaked != None:
                    leakAnalysis.append("WARNING | leakcheck | %s multiple BloatView byte totals found"
                                        % processString)
                else:
                    totalBytesLeaked = 0
                if bytesLeaked > totalBytesLeaked:
                    totalBytesLeaked = bytesLeaked
                    
                    
                    leakedObjectNames = []
                    leakedObjectAnalysis = []
                    recordLeakedObjects = True
                else:
                    recordLeakedObjects = False
            if size < 0 or bytesLeaked < 0 or numLeaked < 0:
                leakAnalysis.append("TEST-UNEXPECTED-FAIL | leakcheck | %s negative leaks caught!"
                                    % processString)
                logAsWarning = True
                continue
            if name != "TOTAL" and numLeaked != 0 and recordLeakedObjects:
                leakedObjectNames.append(name)
                leakedObjectAnalysis.append("TEST-INFO | leakcheck | %s leaked %d %s (%s bytes)"
                                            % (processString, numLeaked, name, bytesLeaked))

    leakAnalysis.extend(leakedObjectAnalysis)
    if logAsWarning:
        log.warning('\n'.join(leakAnalysis))
    else:
        log.info('\n'.join(leakAnalysis))

    logAsWarning = False

    if totalBytesLeaked is None:
        
        if crashedOnPurpose:
            log.info("TEST-INFO | leakcheck | %s deliberate crash and thus no leak log"
                     % processString)
        elif ignoreMissingLeaks:
            log.info("TEST-INFO | leakcheck | %s ignoring missing output line for total leaks"
                     % processString)
        else:
            log.info("TEST-UNEXPECTED-FAIL | leakcheck | %s missing output line for total leaks!"
                     % processString)
            log.info("TEST-INFO | leakcheck | missing output line from log file %s"
                     % leakLogFileName)
        return

    if totalBytesLeaked == 0:
        log.info("TEST-PASS | leakcheck | %s no leaks detected!" %
                 processString)
        return

    
    if totalBytesLeaked > leakThreshold:
        logAsWarning = True
        
        prefix = "TEST-UNEXPECTED-FAIL"
    else:
        prefix = "WARNING"
    
    
    
    maxSummaryObjects = 5
    leakedObjectSummary = ', '.join(leakedObjectNames[:maxSummaryObjects])
    if len(leakedObjectNames) > maxSummaryObjects:
        leakedObjectSummary += ', ...'

    if logAsWarning:
        log.warning("%s | leakcheck | %s %d bytes leaked (%s)"
                    % (prefix, processString, totalBytesLeaked, leakedObjectSummary))
    else:
        log.info("%s | leakcheck | %s %d bytes leaked (%s)"
                 % (prefix, processString, totalBytesLeaked, leakedObjectSummary))


def process_leak_log(leak_log_file, leak_thresholds=None,
                     ignore_missing_leaks=None, log=None):
    """Process the leak log, including separate leak logs created
    by child processes.

    Use this function if you want an additional PASS/FAIL summary.
    It must be used with the |XPCOM_MEM_BLOAT_LOG| environment variable.

    The base of leak_log_file for a non-default process needs to end with
      _proctype_pid12345.log
    "proctype" is a string denoting the type of the process, which should
    be the result of calling XRE_ChildProcessTypeToString(). 12345 is
    a series of digits that is the pid for the process. The .log is
    optional.

    All other file names are treated as being for default processes.

    leak_thresholds should be a dict mapping process types to leak thresholds,
    in bytes. If a process type is not present in the dict the threshold
    will be 0.

    ignore_missing_leaks should be a list of process types. If a process
    creates a leak log without a TOTAL, then we report an error if it isn't
    in the list ignore_missing_leaks.
    """

    log = log or _raw_log()

    leakLogFile = leak_log_file
    if not os.path.exists(leakLogFile):
        log.info(
            "WARNING | leakcheck | refcount logging is off, so leaks can't be detected!")
        return

    leakThresholds = leak_thresholds or {}
    ignoreMissingLeaks = ignore_missing_leaks or []

    
    
    knownProcessTypes = ["default", "plugin", "tab", "geckomediaplugin"]

    for processType in knownProcessTypes:
        log.info("TEST-INFO | leakcheck | %s process: leak threshold set at %d bytes"
                 % (processType, leakThresholds.get(processType, 0)))

    for processType in leakThresholds:
        if not processType in knownProcessTypes:
            log.info("TEST-UNEXPECTED-FAIL | leakcheck | Unknown process type %s in leakThresholds"
                     % processType)

    (leakLogFileDir, leakFileBase) = os.path.split(leakLogFile)
    if leakFileBase[-4:] == ".log":
        leakFileBase = leakFileBase[:-4]
        fileNameRegExp = re.compile(r"_([a-z]*)_pid\d*.log$")
    else:
        fileNameRegExp = re.compile(r"_([a-z]*)_pid\d*$")

    for fileName in os.listdir(leakLogFileDir):
        if fileName.find(leakFileBase) != -1:
            thisFile = os.path.join(leakLogFileDir, fileName)
            m = fileNameRegExp.search(fileName)
            if m:
                processType = m.group(1)
            else:
                processType = "default"
            if not processType in knownProcessTypes:
                log.info("TEST-UNEXPECTED-FAIL | leakcheck | Leak log with unknown process type %s"
                         % processType)
            leakThreshold = leakThresholds.get(processType, 0)
            process_single_leak_file(thisFile, processType, leakThreshold,
                                     processType in ignoreMissingLeaks,
                                     log=log)
