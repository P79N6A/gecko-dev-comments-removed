



class DateTimeValue(object):
    """
    Interface for setting the value of HTML5 "date" and "time" input elements.

    Simple usage example:

    ::

        element = marionette.find_element("id", "date-test")
        dt_value = DateTimeValue(element)
        dt_value.date = datetime(1998, 6, 2)

    """

    def __init__(self, element):
        self.element = element

    @property
    def date(self):
        """
        Retrieve the element's string value
        """
        return self.element.get_attribute('value')

    
    
    
    @date.setter
    def date(self, date_value):
        self.element.send_keys(date_value.strftime('%Y-%m-%d'))

    @property
    def time(self):
        """
        Retrieve the element's string value
        """
        return self.element.get_attribute('value')

    
    
    
    @time.setter
    def time(self, time_value):
        self.element.send_keys(time_value.strftime('%H:%M:%S'))

