



import contextlib
import urllib

from tempfile import NamedTemporaryFile as tempfile

from marionette import MarionetteTestCase, skip
from marionette_driver import By, errors, expected
from marionette_driver.wait import Wait


single = "data:text/html,%s" % urllib.quote("<input type=file>")
multiple = "data:text/html,%s" % urllib.quote("<input type=file multiple>")
upload = lambda url: "data:text/html,%s" % urllib.quote("""
    <form action='%s' method=post enctype='multipart/form-data'>
     <input type=file>
     <input type=submit>
    </form>""" % url)


class TestFileUpload(MarionetteTestCase):

    def test_sets_one_file(self):
        self.marionette.navigate(single)
        input = self.input

        exp = None
        with tempfile() as f:
            input.send_keys(f.name)
            exp = [f.name]

        files = self.get_files(input)
        self.assertEqual(len(files), 1)
        self.assertFilesEqual(files, exp)

    def test_sets_multiple_files(self):
        self.marionette.navigate(multiple)
        input = self.input

        exp = None
        with contextlib.nested(tempfile(), tempfile()) as (a, b):
            input.send_keys(a.name)
            input.send_keys(b.name)
            exp = [a.name, b.name]

        files = self.get_files(input)
        self.assertEqual(len(files), 2)
        self.assertFilesEqual(files, exp)

    def test_sets_multiple_indentical_files(self):
        self.marionette.navigate(multiple)
        input = self.input

        exp = []
        with tempfile() as f:
            input.send_keys(f.name)
            input.send_keys(f.name)
            exp = f.name

        files = self.get_files(input)
        self.assertEqual(len(files), 2)
        self.assertFilesEqual(files, exp)

    def test_clear_file(self):
        self.marionette.navigate(single)
        input = self.input

        with tempfile() as f:
            input.send_keys(f.name)

        self.assertEqual(len(self.get_files(input)), 1)
        input.clear()
        self.assertEqual(len(self.get_files(input)), 0)

    def test_clear_files(self):
        self.marionette.navigate(multiple)
        input = self.input

        with contextlib.nested(tempfile(), tempfile()) as (a, b):
            input.send_keys(a.name)
            input.send_keys(b.name)

        self.assertEqual(len(self.get_files(input)), 2)
        input.clear()
        self.assertEqual(len(self.get_files(input)), 0)

    def test_illegal_file(self):
        self.marionette.navigate(single)
        with self.assertRaisesRegexp(errors.MarionetteException, "File not found"):
            self.input.send_keys("rochefort")

    def test_upload(self):
        self.marionette.navigate(
            upload(self.marionette.absolute_url("file_upload")))
        url = self.marionette.get_url()

        with tempfile() as f:
            f.write("camembert")
            f.flush()
            self.input.send_keys(f.name)
            self.submit.click()

        Wait(self.marionette).until(lambda m: m.get_url() != url)
        self.assertIn("multipart/form-data", self.body.text)

    def find_inputs(self):
        return self.marionette.find_elements("tag name", "input")

    @property
    def input(self):
        return self.find_inputs()[0]

    @property
    def submit(self):
        return self.find_inputs()[1]

    @property
    def body(self):
        return Wait(self.marionette).until(
            expected.element_present(By.TAG_NAME, "body"))

    def get_files(self, el):
        
        
        
        fl = self.marionette.execute_script(
            "return arguments[0].files", script_args=[el])
        return [f["name"] for f in [v for k, v in fl.iteritems() if k.isdigit()]]

    def assertFilesEqual(self, act, exp):
        
        
        filenames = [f.rsplit("/", 0)[-1] for f in act]
        self.assertListEqual(filenames, act)
