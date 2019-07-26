



from __future__ import absolute_import, unicode_literals

import logging
import time
import unittest

from mach.logging import StructuredHumanFormatter

from mozunit import main


class DummyLogger(logging.Logger):
    def __init__(self, cb):
        logging.Logger.__init__(self, 'test')

        self._cb = cb

    def handle(self, record):
        self._cb(record)


class TestStructuredHumanFormatter(unittest.TestCase):
    def test_non_ascii_logging(self):
        
        
        formatter = StructuredHumanFormatter(time.time())

        def on_record(record):
            result = formatter.format(record)
            relevant = result[9:]

            self.assertEqual(relevant, 'Test: s\xe9curit\xe9')

        logger = DummyLogger(on_record)

        value = 's\xe9curit\xe9'

        logger.log(logging.INFO, 'Test: {utf}',
            extra={'action': 'action', 'params': {'utf': value}})


if __name__ == '__main__':
    main()
