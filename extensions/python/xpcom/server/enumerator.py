




































from xpcom import components







class SimpleEnumerator:
    _com_interfaces_ = [components.interfaces.nsISimpleEnumerator]

    def __init__(self, data):
        self._data = data
        self._index = 0

    def hasMoreElements(self):
        return self._index < len(self._data)
    
    def getNext(self):
        self._index = self._index + 1
        return self._data[self._index-1]
