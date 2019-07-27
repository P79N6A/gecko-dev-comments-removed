



from collections import defaultdict
from mozlog.structured.structuredlog import log_levels

class StatusHandler(object):
    """A handler used to determine an overall status for a test run according
    to a sequence of log messages."""

    def __init__(self):
        
        self.unexpected = 0
        
        self.skipped = 0
        
        self.test_count = 0
        
        self.level_counts = defaultdict(int)


    def __call__(self, data):
        action = data['action']
        if action == 'log':
            self.level_counts[data['level']] += 1

        if action == 'test_end':
            self.test_count += 1

        if action in ('test_status', 'test_end'):
            if 'expected' in data:
                self.unexpected += 1

            if data['status'] == 'SKIP':
                self.skipped += 1


    def evaluate(self):
        status = 'OK'

        if self.unexpected:
            status = 'FAIL'

        if not self.test_count:
            status = 'ERROR'

        for level in self.level_counts:
            if log_levels[level] <= log_levels['ERROR']:
                status = 'ERROR'

        summary = {
            'status': status,
            'unexpected': self.unexpected,
            'skipped': self.skipped,
            'test_count': self.test_count,
            'level_counts': dict(self.level_counts),
        }
        return summary
