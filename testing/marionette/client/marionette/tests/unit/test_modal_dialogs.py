



from marionette_test import MarionetteTestCase, skip_if_e10s
from errors import NoAlertPresentException, ElementNotVisibleException
from marionette import Alert
from wait import Wait

class TestTabModals(MarionetteTestCase):

    def setUp(self):
        MarionetteTestCase.setUp(self)
        self.marionette.enforce_gecko_prefs({"prompts.tab_modal.enabled": True})
        self.marionette.navigate(self.marionette.absolute_url('modal_dialogs.html'))

    def alert_present(self):
        try:
            Alert(self.marionette).text
            return True
        except NoAlertPresentException:
            return False

    def tearDown(self):
        
        Wait(self.marionette).until(lambda _: not self.alert_present())
        self.marionette.execute_script("window.onbeforeunload = null;")

    def wait_for_alert(self):
        Wait(self.marionette).until(lambda _: self.alert_present())

    def test_no_alert_raises(self):
        self.assertRaises(NoAlertPresentException, Alert(self.marionette).accept)
        self.assertRaises(NoAlertPresentException, Alert(self.marionette).dismiss)

    def test_alert_accept(self):
        self.marionette.find_element('id', 'modal-alert').click()
        self.wait_for_alert()
        Alert(self.marionette).accept()

    def test_alert_dismiss(self):
        self.marionette.find_element('id', 'modal-alert').click()
        self.wait_for_alert()
        Alert(self.marionette).dismiss()

    def test_confirm_accept(self):
        self.marionette.find_element('id', 'modal-confirm').click()
        self.wait_for_alert()
        Alert(self.marionette).accept()
        self.wait_for_condition(lambda mn: mn.find_element('id', 'confirm-result').text == 'true')

    def test_confirm_dismiss(self):
        self.marionette.find_element('id', 'modal-confirm').click()
        self.wait_for_alert()
        Alert(self.marionette).dismiss()
        self.wait_for_condition(lambda mn: mn.find_element('id', 'confirm-result').text == 'false')

    def test_prompt_accept(self):
        self.marionette.find_element('id', 'modal-prompt').click()
        self.wait_for_alert()
        Alert(self.marionette).accept()
        self.wait_for_condition(lambda mn: mn.find_element('id', 'prompt-result').text == '')

    def test_prompt_dismiss(self):
        self.marionette.find_element('id', 'modal-prompt').click()
        self.wait_for_alert()
        Alert(self.marionette).dismiss()
        self.wait_for_condition(lambda mn: mn.find_element('id', 'prompt-result').text == 'null')

    def test_alert_text(self):
        with self.assertRaises(NoAlertPresentException):
            Alert(self.marionette).text
        self.marionette.find_element('id', 'modal-alert').click()
        self.wait_for_alert()
        self.assertEqual(Alert(self.marionette).text, 'Marionette alert')
        Alert(self.marionette).accept()

    def test_prompt_text(self):
        with self.assertRaises(NoAlertPresentException):
            Alert(self.marionette).text
        self.marionette.find_element('id', 'modal-prompt').click()
        self.wait_for_alert()
        self.assertEqual(Alert(self.marionette).text, 'Marionette prompt')
        Alert(self.marionette).accept()

    def test_confirm_text(self):
        with self.assertRaises(NoAlertPresentException):
            Alert(self.marionette).text
        self.marionette.find_element('id', 'modal-confirm').click()
        self.wait_for_alert()
        self.assertEqual(Alert(self.marionette).text, 'Marionette confirm')
        Alert(self.marionette).accept()

    def test_set_text_throws(self):
        self.assertRaises(NoAlertPresentException, Alert(self.marionette).send_keys, "Foo")
        self.marionette.find_element('id', 'modal-alert').click()
        self.wait_for_alert()
        self.assertRaises(ElementNotVisibleException, Alert(self.marionette).send_keys, "Foo")
        Alert(self.marionette).accept()

    def test_set_text_accept(self):
        self.marionette.find_element('id', 'modal-prompt').click()
        self.wait_for_alert()
        Alert(self.marionette).send_keys("Some text!");
        Alert(self.marionette).accept()
        self.wait_for_condition(lambda mn: mn.find_element('id', 'prompt-result').text == 'Some text!')

    def test_set_text_dismiss(self):
        self.marionette.find_element('id', 'modal-prompt').click()
        self.wait_for_alert()
        Alert(self.marionette).send_keys("Some text!");
        Alert(self.marionette).dismiss()
        self.wait_for_condition(lambda mn: mn.find_element('id', 'prompt-result').text == 'null')

    def test_onbeforeunload_dismiss(self):
        start_url = self.marionette.get_url()
        self.marionette.find_element('id', 'onbeforeunload-handler').click()
        self.wait_for_condition(
            lambda mn: mn.execute_script("""
              return window.onbeforeunload !== null;
            """))
        self.marionette.navigate("about:blank")
        self.wait_for_alert()
        alert_text = Alert(self.marionette).text
        self.assertTrue(alert_text.startswith("This page is asking you to confirm"))
        Alert(self.marionette).dismiss()
        self.assertTrue(self.marionette.get_url().startswith(start_url))

    def test_onbeforeunload_accept(self):
        self.marionette.find_element('id', 'onbeforeunload-handler').click()
        self.wait_for_condition(
            lambda mn: mn.execute_script("""
              return window.onbeforeunload !== null;
            """))
        self.marionette.navigate("about:blank")
        self.wait_for_alert()
        alert_text = Alert(self.marionette).text
        self.assertTrue(alert_text.startswith("This page is asking you to confirm"))
        Alert(self.marionette).accept()
        self.assertEqual(self.marionette.get_url(), "about:blank")

    @skip_if_e10s
    def test_unrelated_command_when_alert_present(self):
        click_handler = self.marionette.find_element('id', 'click-handler')
        text = self.marionette.find_element('id', 'click-result').text
        self.assertEqual(text, '')

        self.marionette.find_element('id', 'modal-alert').click()
        self.wait_for_alert()

        
        
        text = self.marionette.find_element('id', 'click-result').text
        self.assertEqual(text, '')
        click_handler.click()
        text = self.marionette.find_element('id', 'click-result').text
        self.assertEqual(text, '')

        Alert(self.marionette).accept()

        Wait(self.marionette).until(lambda _: not self.alert_present())

        click_handler.click()
        text = self.marionette.find_element('id', 'click-result').text
        self.assertEqual(text, 'result')


class TestGlobalModals(TestTabModals):

    def setUp(self):
        MarionetteTestCase.setUp(self)
        self.marionette.enforce_gecko_prefs({"prompts.tab_modal.enabled": False})
        self.marionette.navigate(self.marionette.absolute_url('modal_dialogs.html'))

    def test_unrelated_command_when_alert_present(self):
        
        
        pass
