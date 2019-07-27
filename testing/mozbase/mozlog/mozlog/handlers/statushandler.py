



from collections import (
    defaultdict,
    namedtuple,
)

from mozlog.structuredlog import log_levels

RunSummary = namedtuple("RunSummary",
                        ("unexpected_statuses",
                         "expected_statuses",
                         "log_level_counts",
                         "action_counts"))

class StatusHandler(object):
    """A handler used to determine an overall status for a test run according
    to a sequence of log messages."""

    def __init__(self):
        
        self.unexpected_statuses = defaultdict(int)
        
        self.expected_statuses = defaultdict(int)
        
        self.action_counts = defaultdict(int)
        
        self.log_level_counts = defaultdict(int)


    def __call__(self, data):
        action = data['action']
        self.action_counts[action] += 1

        if action == 'log':
            self.log_level_counts[data['level']] += 1

        if action in ('test_status', 'test_end'):
            status = data['status']
            if 'expected' in data:
                self.unexpected_statuses[status] += 1
            else:
                self.expected_statuses[status] += 1


    def summarize(self):
        return RunSummary(
            dict(self.unexpected_statuses),
            dict(self.expected_statuses),
            dict(self.log_level_counts),
            dict(self.action_counts),
        )
