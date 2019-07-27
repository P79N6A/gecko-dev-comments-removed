































"""Tests for mock module."""


import Queue
import threading
import unittest

import set_sys_path  

from test import mock


class MockConnTest(unittest.TestCase):
    """A unittest for MockConn class."""

    def setUp(self):
        self._conn = mock.MockConn('ABC\r\nDEFG\r\n\r\nHIJK')

    def test_readline(self):
        self.assertEqual('ABC\r\n', self._conn.readline())
        self.assertEqual('DEFG\r\n', self._conn.readline())
        self.assertEqual('\r\n', self._conn.readline())
        self.assertEqual('HIJK', self._conn.readline())
        self.assertEqual('', self._conn.readline())

    def test_read(self):
        self.assertEqual('ABC\r\nD', self._conn.read(6))
        self.assertEqual('EFG\r\n\r\nHI', self._conn.read(9))
        self.assertEqual('JK', self._conn.read(10))
        self.assertEqual('', self._conn.read(10))

    def test_read_and_readline(self):
        self.assertEqual('ABC\r\nD', self._conn.read(6))
        self.assertEqual('EFG\r\n', self._conn.readline())
        self.assertEqual('\r\nHIJK', self._conn.read(9))
        self.assertEqual('', self._conn.readline())

    def test_write(self):
        self._conn.write('Hello\r\n')
        self._conn.write('World\r\n')
        self.assertEqual('Hello\r\nWorld\r\n', self._conn.written_data())


class MockBlockingConnTest(unittest.TestCase):
    """A unittest for MockBlockingConn class."""

    def test_read(self):
        """Tests that data put to MockBlockingConn by put_bytes method can be
        read from it.
        """

        class LineReader(threading.Thread):
            """A test class that launches a thread, calls readline on the
            specified conn repeatedly and puts the read data to the specified
            queue.
            """

            def __init__(self, conn, queue):
                threading.Thread.__init__(self)
                self._queue = queue
                self._conn = conn
                self.setDaemon(True)
                self.start()

            def run(self):
                while True:
                    data = self._conn.readline()
                    self._queue.put(data)

        conn = mock.MockBlockingConn()
        queue = Queue.Queue()
        reader = LineReader(conn, queue)
        self.failUnless(queue.empty())
        conn.put_bytes('Foo bar\r\n')
        read = queue.get()
        self.assertEqual('Foo bar\r\n', read)


class MockTableTest(unittest.TestCase):
    """A unittest for MockTable class."""

    def test_create_from_dict(self):
        table = mock.MockTable({'Key': 'Value'})
        self.assertEqual('Value', table.get('KEY'))
        self.assertEqual('Value', table['key'])

    def test_create_from_list(self):
        table = mock.MockTable([('Key', 'Value')])
        self.assertEqual('Value', table.get('KEY'))
        self.assertEqual('Value', table['key'])

    def test_create_from_tuple(self):
        table = mock.MockTable((('Key', 'Value'),))
        self.assertEqual('Value', table.get('KEY'))
        self.assertEqual('Value', table['key'])

    def test_set_and_get(self):
        table = mock.MockTable()
        self.assertEqual(None, table.get('Key'))
        table['Key'] = 'Value'
        self.assertEqual('Value', table.get('Key'))
        self.assertEqual('Value', table.get('key'))
        self.assertEqual('Value', table.get('KEY'))
        self.assertEqual('Value', table['Key'])
        self.assertEqual('Value', table['key'])
        self.assertEqual('Value', table['KEY'])


if __name__ == '__main__':
    unittest.main()



