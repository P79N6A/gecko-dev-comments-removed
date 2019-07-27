



from collections import (
    defaultdict,
    namedtuple,
)

from mozlog.structured.structuredlog import log_levels

RunSummary = namedtuple("RunSummary",
                        ("unexpected", "skipped", "log_level_counts", "action_counts"))

class StatusHandler(object):
    """A handler used to determine an overall status for a test run according
    to a sequence of log messages."""

    def __init__(self):
        
        self.unexpected = 0
        
        self.skipped = 0
        
        self.action_counts = defaultdict(int)
        
        self.log_level_counts = defaultdict(int)


    def __call__(self, data):
        action = data['action']
        self.action_counts[action] += 1

        if action == 'log':
            self.log_level_counts[data['level']] += 1

        if action in ('test_status', 'test_end'):
            if 'expected' in data:
                self.unexpected += 1

            if data['status'] == 'SKIP':
                self.skipped += 1

    def summarize(self):
        return RunSummary(
            self.unexpected,
            self.skipped,
            dict(self.log_level_counts),
            dict(self.action_counts),
        )
