



from marionette_test import MarionetteTestCase
from errors import ElementNotAccessibleException
from errors import ElementNotVisibleException


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

    def test_invalid_single_tap(self):
        self.setup_accessibility()
        self.run_element_test(self.invalid_elementIDs,
                              lambda button: self.assertRaises(ElementNotAccessibleException,
                                                               button.tap))
        self.run_element_test(self.falsy_elements,
                              lambda button: self.assertRaises(ElementNotAccessibleException,
                                                               button.tap))

    def test_invalid_single_tap_no_exceptions(self):
        self.setup_accessibility(False, True)
        
        self.run_element_test(self.invalid_elementIDs, lambda button: button.tap())
        
        self.run_element_test(self.falsy_elements,
                              lambda button: self.assertRaises(ElementNotVisibleException,
                                                               button.tap))
