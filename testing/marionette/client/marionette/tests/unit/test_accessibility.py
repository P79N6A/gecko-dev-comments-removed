



from marionette import MarionetteTestCase
from marionette_driver.errors import (ElementNotAccessibleException,
                                     ElementNotVisibleException)


class TestAccessibility(MarionetteTestCase):

    
    valid_elementIDs = [
        
        
        "button1",
        
        
        "button2"
    ]

    
    invalid_elementIDs = [
        
        "button3",
        
        "button4",
        
        
        "button5",
        
        "button6",
        
        
        "button7",
        
        
        "button8"
    ]

    
    
    falsy_elements = [
        
        
        "button9",
        
        "button10"
    ]

    displayed_elementIDs = [
        "button1", "button2", "button3", "button4", "button5", "button6",
        "button9", "no_accessible_but_displayed"
    ]

    displayed_but_a11y_hidden_elementIDs = ["button7", "button8"]

    disabled_elementIDs = ["button11", "no_accessible_but_disabled"]

    def run_element_test(self, ids, testFn):
        for id in ids:
            element = self.marionette.find_element("id", id)
            testFn(element)

    def setup_accessibility(self, raisesAccessibilityExceptions=True, navigate=True):
        self.marionette.delete_session()
        self.marionette.start_session(
            {"raisesAccessibilityExceptions": raisesAccessibilityExceptions})
        
        if navigate:
            test_accessibility = self.marionette.absolute_url("test_accessibility.html")
            self.marionette.navigate(test_accessibility)

    def test_valid_single_tap(self):
        self.setup_accessibility()
        
        self.run_element_test(self.valid_elementIDs, lambda button: button.tap())

    def test_single_tap_raises_element_not_accessible(self):
        self.setup_accessibility()
        self.run_element_test(self.invalid_elementIDs,
                              lambda button: self.assertRaises(ElementNotAccessibleException,
                                                               button.tap))
        self.run_element_test(self.falsy_elements,
                              lambda button: self.assertRaises(ElementNotAccessibleException,
                                                               button.tap))

    def test_single_tap_raises_no_exceptions(self):
        self.setup_accessibility(False, True)
        
        self.run_element_test(self.invalid_elementIDs, lambda button: button.tap())
        
        self.run_element_test(self.falsy_elements,
                              lambda button: self.assertRaises(ElementNotVisibleException,
                                                               button.tap))

    def test_valid_click(self):
        self.setup_accessibility()
        
        self.run_element_test(self.valid_elementIDs, lambda button: button.click())

    def test_click_raises_element_not_accessible(self):
        self.setup_accessibility()
        self.run_element_test(self.invalid_elementIDs,
                              lambda button: self.assertRaises(ElementNotAccessibleException,
                                                               button.click))
        self.run_element_test(self.falsy_elements,
                              lambda button: self.assertRaises(ElementNotAccessibleException,
                                                               button.click))

    def test_click_raises_no_exceptions(self):
        self.setup_accessibility(False, True)
        
        self.run_element_test(self.invalid_elementIDs, lambda button: button.click())
        
        self.run_element_test(self.falsy_elements,
                              lambda button: self.assertRaises(ElementNotVisibleException,
                                                               button.click))

    def test_element_visible_but_not_visible_to_accessbility(self):
        self.setup_accessibility()
        
        self.run_element_test(self.displayed_but_a11y_hidden_elementIDs,
                              lambda element: self.assertRaises(ElementNotAccessibleException,
                                                                element.is_displayed))

    def test_element_is_visible_to_accessibility(self):
        self.setup_accessibility()
        
        self.run_element_test(self.displayed_elementIDs, lambda element: element.is_displayed())

    def test_is_element_is_not_enabled_to_accessbility(self):
        self.setup_accessibility()
        
        self.assertRaises(ElementNotAccessibleException,
                          self.marionette.find_element("id", "button12").is_enabled)

    def test_element_is_enabled_to_accessibility(self):
        self.setup_accessibility()
        
        self.run_element_test(self.disabled_elementIDs, lambda element: element.is_enabled())
