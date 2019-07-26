# -*- coding: utf-8 -*-




import string

from by import By
from marionette import Actions
from marionette_test import MarionetteTestCase


class TouchCaretTest(MarionetteTestCase):
    _input_selector = (By.ID, 'input')
    _textarea_selector = (By.ID, 'textarea')
    _contenteditable_selector = (By.ID, 'contenteditable')

    def setUp(self):
        
        MarionetteTestCase.setUp(self)
        self.actions = Actions(self.marionette)

    def openTestHtml(self, enabled=True):
        '''Open html for testing and locate elements, and enable/disable touch
        caret.'''
        self.marionette.execute_script(
            'SpecialPowers.setBoolPref("touchcaret.enabled", %s);' %
            ('true' if enabled else 'false'))

        test_html = self.marionette.absolute_url('test_touchcaret.html')
        self.marionette.navigate(test_html)

        self._input = self.marionette.find_element(*self._input_selector)
        self._textarea = self.marionette.find_element(*self._textarea_selector)
        self._contenteditable = self.marionette.find_element(*self._contenteditable_selector)

    def is_input_or_textarea(self, element):
        '''Return True if element is either <input> or <textarea>'''
        return element.tag_name in ('input', 'textarea')

    def get_js_selection_cmd(self, element):
        '''Return a command snippet to get selection object.

        If the element is <input> or <textarea>, return the selection object
        associated with it. Otherwise, return the current selection object.

        Note: "element" must be provided as the first argument to
        execute_script().

        '''
        if self.is_input_or_textarea(element):
            
            
            return '''var sel = SpecialPowers.wrap(arguments[0]).editor.selection;
                   sel = SpecialPowers.unwrap(sel);'''
        else:
            return '''var sel = window.getSelection();'''

    def caret_rect(self, element):
        '''Return the caret's DOMRect object.

        If the element is either <input> or <textarea>, return the caret's
        DOMRect within the element. Otherwise, return the DOMRect of the
        current selected caret.

        '''
        cmd = self.get_js_selection_cmd(element) +\
            '''return sel.getRangeAt(0).getClientRects()[0];'''
        return self.marionette.execute_script(cmd, script_args=[element])

    def caret_location(self, element):
        '''Return caret's center location by the number of characters offset
        within the given element.

        Return (x, y) coordinates of the caret's center by the number of
        characters offset relative to the top left-hand corner of the given
        element.

        '''
        rect = self.caret_rect(element)
        x = rect['left'] + rect['width'] / 2.0 - element.location['x']
        y = rect['top'] + rect['height'] / 2.0 - element.location['y']
        return x, y

    def touch_caret_location(self, element):
        '''Return touch caret's location (based on current caret location).

        Return (x, y) coordinates of the touch caret's tip relative to the top
        left-hand corner of the given element.

        '''
        rect = self.caret_rect(element)
        x = rect['left'] - element.location['x']

        
        
        y = rect['bottom'] + 5 - element.location['y']

        return x, y

    def move_caret_by_offset(self, element, offset, backward=False):
        '''Move caret in the element by offset.'''
        cmd = self.get_js_selection_cmd(element) +\
            '''sel.modify("move", arguments[1], "character");'''
        direction = 'backward' if backward else 'forward'

        for i in range(offset):
            self.marionette.execute_script(
                cmd, script_args=[element, direction])

    def move_caret_to_front(self, element):
        if self.is_input_or_textarea(element):
            cmd = '''arguments[0].setSelectionRange(0, 0);'''
        else:
            cmd = '''var sel = window.getSelection();
                  sel.collapse(arguments[0].firstChild, 0);'''

        self.marionette.execute_script(cmd, script_args=[element])

    def move_caret_to_end(self, element):
        if self.is_input_or_textarea(element):
            cmd = '''var len = arguments[0].value.length;
                  arguments[0].setSelectionRange(len, len);'''
        else:
            cmd = '''var sel = window.getSelection();
                  sel.collapse(arguments[0].lastChild, arguments[0].lastChild.length);'''

        self.marionette.execute_script(cmd, script_args=[element])

    def get_content(self, element):
        '''Return the content of the element.'''
        if self.is_input_or_textarea(element):
            return element.get_attribute('value')
        else:
            return element.text

    def _test_move_caret_to_the_right_by_one_character(self, el, assertFunc):
        content_to_add = '!'
        target_content = self.get_content(el)
        target_content = target_content[:1] + content_to_add + target_content[1:]

        
        self.move_caret_to_front(el)
        caret0_x, caret0_y = self.caret_location(el)
        touch_caret0_x, touch_caret0_y = self.touch_caret_location(el)
        self.move_caret_by_offset(el, 1)
        touch_caret1_x, touch_caret1_y = self.touch_caret_location(el)

        
        el.tap(caret0_x, caret0_y)

        
        self.actions.flick(el, touch_caret0_x, touch_caret0_y,
                           touch_caret1_x, touch_caret1_y).perform()

        el.send_keys(content_to_add)
        assertFunc(target_content, self.get_content(el))

    def _test_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self, el, assertFunc):
        content_to_add = '!'
        target_content = self.get_content(el) + content_to_add

        
        self.move_caret_to_front(el)
        el.tap(*self.caret_location(el))

        
        src_x, src_y = self.touch_caret_location(el)
        dest_x, dest_y = el.size['width'], el.size['height']
        self.actions.flick(el, src_x, src_y, dest_x, dest_y).perform()

        el.send_keys(content_to_add)
        assertFunc(target_content, self.get_content(el))

    def _test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self, el, assertFunc):
        content_to_add = '!'
        target_content = content_to_add + self.get_content(el)

        
        
        
        self.move_caret_to_end(el)
        self.move_caret_by_offset(el, 1, backward=True)
        el.tap(*self.caret_location(el))

        
        src_x, src_y = self.touch_caret_location(el)
        dest_x, dest_y = 0, 0
        self.actions.flick(el, src_x, src_y, dest_x, dest_y).perform()

        el.send_keys(content_to_add)
        assertFunc(target_content, self.get_content(el))

    def _test_touch_caret_timeout_by_dragging_it_to_top_left_corner_after_timout(self, el, assertFunc):
        content_to_add = '!'
        non_target_content = content_to_add + self.get_content(el)

        
        timeout = self.marionette.execute_script(
            'return SpecialPowers.getIntPref("touchcaret.expiration.time");')
        timeout /= 1000.0

        
        
        
        self.move_caret_to_end(el)
        self.move_caret_by_offset(el, 1, backward=True)
        el.tap(*self.caret_location(el))

        
        
        src_x, src_y = self.touch_caret_location(el)
        dest_x, dest_y = 0, 0
        self.actions.wait(timeout).flick(el, src_x, src_y, dest_x, dest_y).perform()

        el.send_keys(content_to_add)
        assertFunc(non_target_content, self.get_content(el))

    def _test_scroll_by_dragging_touch_caret_to_bottom_right_corner(self, el, assertFunc):
        content_to_add = '!'
        target_content = string.ascii_letters + content_to_add

        
        el.clear()
        el.send_keys(string.ascii_letters)

        
        el.tap()

        
        
        src_x, src_y = self.touch_caret_location(el)
        dest_x, dest_y = el.size['width'] + 100, el.size['height']
        self.actions.flick(el, src_x, src_y, dest_x, dest_y).perform()

        el.send_keys(content_to_add)
        assertFunc(target_content, self.get_content(el))

    
    
    
    def test_input_move_caret_to_the_right_by_one_character(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_the_right_by_one_character(self._input, self.assertEqual)

    def test_input_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self._input, self.assertEqual)

    def test_input_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self._input, self.assertEqual)

    def test_input_touch_caret_timeout(self):
        self.openTestHtml(enabled=True)
        self._test_touch_caret_timeout_by_dragging_it_to_top_left_corner_after_timout(self._input, self.assertNotEqual)

    def test_input_scroll_by_dragging_touch_caret_to_bottom_right_corner(self):
        self.openTestHtml(enabled=True)
        self._test_scroll_by_dragging_touch_caret_to_bottom_right_corner(self._input, self.assertEqual)

    
    
    
    def test_input_move_caret_to_the_right_by_one_character_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_caret_to_the_right_by_one_character(self._input, self.assertNotEqual)

    def test_input_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self._input, self.assertNotEqual)

    
    
    
    def test_textarea_move_caret_to_the_right_by_one_character(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_the_right_by_one_character(self._textarea, self.assertEqual)

    def test_textarea_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self._textarea, self.assertEqual)

    def test_textarea_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self._textarea, self.assertEqual)

    def test_textarea_touch_caret_timeout(self):
        self.openTestHtml(enabled=True)
        self._test_touch_caret_timeout_by_dragging_it_to_top_left_corner_after_timout(self._textarea, self.assertNotEqual)

    def test_textarea_scroll_by_dragging_touch_caret_to_bottom_right_corner(self):
        self.openTestHtml(enabled=True)
        self._test_scroll_by_dragging_touch_caret_to_bottom_right_corner(self._textarea, self.assertEqual)

    
    
    
    def test_textarea_move_caret_to_the_right_by_one_character_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_caret_to_the_right_by_one_character(self._textarea, self.assertNotEqual)

    def test_textarea_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self._textarea, self.assertNotEqual)

    
    
    
    def test_contenteditable_move_caret_to_the_right_by_one_character(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_the_right_by_one_character(self._contenteditable, self.assertEqual)

    def test_contenteditable_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_end_by_dragging_touch_caret_to_bottom_right_corner(self._contenteditable, self.assertEqual)

    def test_contenteditable_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self):
        self.openTestHtml(enabled=True)
        self._test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self._contenteditable, self.assertEqual)

    def test_contenteditable_touch_caret_timeout(self):
        self.openTestHtml(enabled=True)
        self._test_touch_caret_timeout_by_dragging_it_to_top_left_corner_after_timout(self._contenteditable, self.assertNotEqual)

    def test_contenteditable_scroll_by_dragging_touch_caret_to_bottom_right_corner(self):
        self.openTestHtml(enabled=True)
        self._test_scroll_by_dragging_touch_caret_to_bottom_right_corner(self._contenteditable, self.assertEqual)

    
    
    
    def test_contenteditable_move_caret_to_the_right_by_one_character_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_caret_to_the_right_by_one_character(self._contenteditable, self.assertNotEqual)

    def test_contenteditable_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_caret_to_front_by_dragging_touch_caret_to_top_left_corner(self._contenteditable, self.assertNotEqual)
