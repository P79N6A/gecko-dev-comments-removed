



import unittest

from mozunit import main

from mozbuild.util import (
    List,
    ReadOnlyDefaultDict,
    ReadOnlyDict,
)

class TestReadOnlyDict(unittest.TestCase):
    def test_basic(self):
        original = {'foo': 1, 'bar': 2}

        test = ReadOnlyDict(original)

        self.assertEqual(original, test)
        self.assertEqual(test['foo'], 1)

        with self.assertRaises(KeyError):
            value = test['missing']

        with self.assertRaises(Exception):
            test['baz'] = True


class TestReadOnlyDefaultDict(unittest.TestCase):
    def test_simple(self):
        original = {'foo': 1, 'bar': 2}

        test = ReadOnlyDefaultDict(bool, original)

        self.assertEqual(original, test)

        self.assertEqual(test['foo'], 1)

    def test_assignment(self):
        test = ReadOnlyDefaultDict(bool, {})

        with self.assertRaises(Exception):
            test['foo'] = True

    def test_defaults(self):
        test = ReadOnlyDefaultDict(bool, {'foo': 1})

        self.assertEqual(test['foo'], 1)

        self.assertEqual(test['qux'], False)


class TestList(unittest.TestCase):
    def test_add_list(self):
        test = List([1, 2, 3])

        test += [4, 5, 6]

        self.assertEqual(test, [1, 2, 3, 4, 5, 6])

    def test_add_string(self):
        test = List([1, 2, 3])

        with self.assertRaises(ValueError):
            test += 'string'


if __name__ == '__main__':
    main()
