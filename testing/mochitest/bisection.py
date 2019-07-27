import math
import mozinfo


class Bisect(object):

    "Class for creating, bisecting and summarizing for --bisect-chunk option."

    def __init__(self, harness):
        super(Bisect, self).__init__()
        self.summary = []
        self.contents = {}
        self.testRoot = harness.testRoot
        self.testRootAbs = harness.testRootAbs
        self.repeat = 10
        self.failcount = 0
        self.max_failures = 3

    def setup(self, tests):
        "This method is used to initialize various variables that are required for test bisection"
        status = 0
        self.contents.clear()
        
        self.contents['totalTests'] = tests
        self.contents['tests'] = tests
        self.contents['loop'] = 0
        return status

    def reset(self, expectedError, result):
        "This method is used to initialize self.expectedError and self.result for each loop in runtests."
        self.expectedError = expectedError
        self.result = result

    def get_test_chunk(self, options, tests):
        "This method is used to return the chunk of test that is to be run"
        if not options.totalChunks or not options.thisChunk:
            return tests

        
        
        if options.chunkByDir:
            tests_by_dir = {}
            test_dirs = []
            for test in tests:
                directory = test.split("/")
                directory = directory[
                    0:min(
                        options.chunkByDir,
                        len(directory) -
                        1)]
                directory = "/".join(directory)

                if directory not in tests_by_dir:
                    tests_by_dir[directory] = [test]
                    test_dirs.append(directory)
                else:
                    tests_by_dir[directory].append(test)

            tests_per_chunk = float(len(test_dirs)) / options.totalChunks
            start = int(round((options.thisChunk - 1) * tests_per_chunk))
            end = int(round((options.thisChunk) * tests_per_chunk))
            test_dirs = test_dirs[start:end]
            return_tests = []
            for directory in test_dirs:
                return_tests += tests_by_dir[directory]

        else:
            tests_per_chunk = float(len(tests)) / options.totalChunks
            start = int(round((options.thisChunk - 1) * tests_per_chunk))
            end = int(round(options.thisChunk * tests_per_chunk))
            return_tests = tests[start:end]

        options.totalChunks = None
        options.thisChunk = None
        options.chunkByDir = None
        return return_tests

    def get_tests_for_bisection(self, options, tests):
        "Make a list of tests for bisection from a given list of tests"
        tests = self.get_test_chunk(options, tests)
        bisectlist = []
        for test in tests:
            bisectlist.append(test)
            if test.endswith(options.bisectChunk):
                break

        return bisectlist

    def pre_test(self, options, tests, status):
        "This method is used to call other methods for setting up variables and getting the list of tests for bisection."
        if options.bisectChunk == "default":
            return tests
        
        
        elif 'loop' not in self.contents or not self.contents['tests'][-1].endswith(options.bisectChunk):
            tests = self.get_tests_for_bisection(options, tests)
            status = self.setup(tests)

        return self.next_chunk_binary(options, status)

    def post_test(self, options, expectedError, result):
        "This method is used to call other methods to summarize results and check whether a sanity check is done or not."
        self.reset(expectedError, result)
        status = self.summarize_chunk(options)
        
        
        if status == -1 and options.bisectChunk in self.expectedError:
            
            
            if mozinfo.info['debug']:
                return status

            testBleedThrough = self.contents['testsToRun'][0]
            tests = self.contents['totalTests']
            tests.remove(testBleedThrough)
            
            
            if options.bisectChunk in testBleedThrough:
                return status

            status = self.setup(tests)
            self.summary.append("Sanity Check:")

        return status

    def next_chunk_reverse(self, options, status):
        "This method is used to bisect the tests in a reverse search fashion."

        
        if self.contents['loop'] <= 1:
            self.contents['testsToRun'] = self.contents['tests']
            if self.contents['loop'] == 1:
                self.contents['testsToRun'] = [self.contents['tests'][-1]]
            self.contents['loop'] += 1
            return self.contents['testsToRun']

        if 'result' in self.contents:
            if self.contents['result'] == "PASS":
                chunkSize = self.contents['end'] - self.contents['start']
                self.contents['end'] = self.contents['start'] - 1
                self.contents['start'] = self.contents['end'] - chunkSize

        
            elif self.contents['result'] == "FAIL":
                self.contents['tests'] = self.contents['testsToRun']
                status = 1  

        
        if status:
            totalTests = len(self.contents['tests'])
            chunkSize = int(math.ceil(totalTests / 10.0))
            self.contents['start'] = totalTests - chunkSize - 1
            self.contents['end'] = totalTests - 2

        start = self.contents['start']
        end = self.contents['end'] + 1
        self.contents['testsToRun'] = self.contents['tests'][start:end]
        self.contents['testsToRun'].append(self.contents['tests'][-1])
        self.contents['loop'] += 1

        return self.contents['testsToRun']

    def next_chunk_binary(self, options, status):
        "This method is used to bisect the tests in a binary search fashion."

        
        if self.contents['loop'] <= 1:
            self.contents['testsToRun'] = self.contents['tests']
            if self.contents['loop'] == 1:
                self.contents['testsToRun'] = [self.contents['tests'][-1]]
            self.contents['loop'] += 1
            return self.contents['testsToRun']

        
        if status:
            totalTests = len(self.contents['tests'])
            self.contents['start'] = 0
            self.contents['end'] = totalTests - 2

        mid = (self.contents['start'] + self.contents['end']) / 2
        if 'result' in self.contents:
            if self.contents['result'] == "PASS":
                self.contents['end'] = mid

            elif self.contents['result'] == "FAIL":
                self.contents['start'] = mid + 1

        mid = (self.contents['start'] + self.contents['end']) / 2
        start = mid + 1
        end = self.contents['end'] + 1
        self.contents['testsToRun'] = self.contents['tests'][start:end]
        if not self.contents['testsToRun']:
            self.contents['testsToRun'].append(self.contents['tests'][mid])
        self.contents['testsToRun'].append(self.contents['tests'][-1])
        self.contents['loop'] += 1

        return self.contents['testsToRun']

    def summarize_chunk(self, options):
        "This method is used summarize the results after the list of tests is run."
        if options.bisectChunk == "default":
            
            
            if len(self.expectedError) == 0:
                return -1
            options.bisectChunk = self.expectedError.keys()[0]
            self.summary.append(
                "\tFound Error in test: %s" %
                options.bisectChunk)
            return 0

        
        
        if options.bisectChunk not in self.result:
            return -1

        self.summary.append("\tPass %d:" % self.contents['loop'])
        if len(self.contents['testsToRun']) > 1:
            self.summary.append(
                "\t\t%d test files(start,end,failing). [%s, %s, %s]" % (len(
                    self.contents['testsToRun']),
                    self.contents['testsToRun'][0],
                    self.contents['testsToRun'][
                    -2],
                    self.contents['testsToRun'][
                    -1]))
        else:
            self.summary.append(
                "\t\t1 test file [%s]" %
                self.contents['testsToRun'][0])
            return self.check_for_intermittent(options)

        if self.result[options.bisectChunk] == "PASS":
            self.summary.append("\t\tno failures found.")
            if self.contents['loop'] == 1:
                status = -1
            else:
                self.contents['result'] = "PASS"
                status = 0

        elif self.result[options.bisectChunk] == "FAIL":
            if 'expectedError' not in self.contents:
                self.summary.append("\t\t%s failed." %
                                    self.contents['testsToRun'][-1])
                self.contents['expectedError'] = self.expectedError[
                    options.bisectChunk]
                status = 0

            elif self.expectedError[options.bisectChunk] == self.contents['expectedError']:
                self.summary.append(
                    "\t\t%s failed with expected error." % self.contents['testsToRun'][-1])
                self.contents['result'] = "FAIL"
                status = 0

                
                
                numberOfTests = len(self.contents['testsToRun'])
                if numberOfTests < 3:
                    
                    
                    
                    self.summary.append(
                        "TEST-UNEXPECTED-FAIL | %s | Bleedthrough detected, this test is the root cause for many of the above failures" %
                        self.contents['testsToRun'][0])
                    status = -1
            else:
                self.summary.append(
                    "\t\t%s failed with different error." % self.contents['testsToRun'][-1])
                status = -1

        return status

    def check_for_intermittent(self, options):
        "This method is used to check whether a test is an intermittent."
        if self.result[options.bisectChunk] == "PASS":
            self.summary.append(
                "\t\tThe test %s passed." %
                self.contents['testsToRun'][0])
            if self.repeat > 0:
                
                self.contents['loop'] = 1
                self.repeat -= 1
                return 0
            else:
                if self.failcount > 0:
                    
                    return -1
                
                
                self.contents['loop'] = 2
                return 1
        elif self.result[options.bisectChunk] == "FAIL":
            self.summary.append(
                "\t\tThe test %s failed." %
                self.contents['testsToRun'][0])
            self.failcount += 1
            self.contents['loop'] = 1
            self.repeat -= 1
            
            
            
            if self.failcount < self.max_failures:
                if self.repeat == 0:
                    
                    return -1
                return 0
            else:
                self.summary.append(
                    "TEST-UNEXPECTED-FAIL | %s | Bleedthrough detected, this test is the root cause for many of the above failures" %
                    self.contents['testsToRun'][0])
                return -1

    def print_summary(self):
        "This method is used to print the recorded summary."
        print "Bisection summary:"
        for line in self.summary:
            print line
