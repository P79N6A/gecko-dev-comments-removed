from __future__ import absolute_import, division, unicode_literals
from six import text_type
from six.moves import http_client

import codecs
import re

from .constants import EOF, spaceCharacters, asciiLetters, asciiUppercase
from .constants import encodings, ReparseException
from . import utils

from io import StringIO

try:
    from io import BytesIO
except ImportError:
    BytesIO = StringIO

try:
    from io import BufferedIOBase
except ImportError:
    class BufferedIOBase(object):
        pass


spaceCharactersBytes = frozenset([item.encode("ascii") for item in spaceCharacters])
asciiLettersBytes = frozenset([item.encode("ascii") for item in asciiLetters])
asciiUppercaseBytes = frozenset([item.encode("ascii") for item in asciiUppercase])
spacesAngleBrackets = spaceCharactersBytes | frozenset([b">", b"<"])

invalid_unicode_re = re.compile("[\u0001-\u0008\u000B\u000E-\u001F\u007F-\u009F\uD800-\uDFFF\uFDD0-\uFDEF\uFFFE\uFFFF\U0001FFFE\U0001FFFF\U0002FFFE\U0002FFFF\U0003FFFE\U0003FFFF\U0004FFFE\U0004FFFF\U0005FFFE\U0005FFFF\U0006FFFE\U0006FFFF\U0007FFFE\U0007FFFF\U0008FFFE\U0008FFFF\U0009FFFE\U0009FFFF\U000AFFFE\U000AFFFF\U000BFFFE\U000BFFFF\U000CFFFE\U000CFFFF\U000DFFFE\U000DFFFF\U000EFFFE\U000EFFFF\U000FFFFE\U000FFFFF\U0010FFFE\U0010FFFF]")

non_bmp_invalid_codepoints = set([0x1FFFE, 0x1FFFF, 0x2FFFE, 0x2FFFF, 0x3FFFE,
                                  0x3FFFF, 0x4FFFE, 0x4FFFF, 0x5FFFE, 0x5FFFF,
                                  0x6FFFE, 0x6FFFF, 0x7FFFE, 0x7FFFF, 0x8FFFE,
                                  0x8FFFF, 0x9FFFE, 0x9FFFF, 0xAFFFE, 0xAFFFF,
                                  0xBFFFE, 0xBFFFF, 0xCFFFE, 0xCFFFF, 0xDFFFE,
                                  0xDFFFF, 0xEFFFE, 0xEFFFF, 0xFFFFE, 0xFFFFF,
                                  0x10FFFE, 0x10FFFF])

ascii_punctuation_re = re.compile("[\u0009-\u000D\u0020-\u002F\u003A-\u0040\u005B-\u0060\u007B-\u007E]")


charsUntilRegEx = {}


class BufferedStream(object):
    """Buffering for streams that do not have buffering of their own

    The buffer is implemented as a list of chunks on the assumption that
    joining many strings will be slow since it is O(n**2)
    """

    def __init__(self, stream):
        self.stream = stream
        self.buffer = []
        self.position = [-1, 0]  

    def tell(self):
        pos = 0
        for chunk in self.buffer[:self.position[0]]:
            pos += len(chunk)
        pos += self.position[1]
        return pos

    def seek(self, pos):
        assert pos <= self._bufferedBytes()
        offset = pos
        i = 0
        while len(self.buffer[i]) < offset:
            offset -= len(self.buffer[i])
            i += 1
        self.position = [i, offset]

    def read(self, bytes):
        if not self.buffer:
            return self._readStream(bytes)
        elif (self.position[0] == len(self.buffer) and
              self.position[1] == len(self.buffer[-1])):
            return self._readStream(bytes)
        else:
            return self._readFromBuffer(bytes)

    def _bufferedBytes(self):
        return sum([len(item) for item in self.buffer])

    def _readStream(self, bytes):
        data = self.stream.read(bytes)
        self.buffer.append(data)
        self.position[0] += 1
        self.position[1] = len(data)
        return data

    def _readFromBuffer(self, bytes):
        remainingBytes = bytes
        rv = []
        bufferIndex = self.position[0]
        bufferOffset = self.position[1]
        while bufferIndex < len(self.buffer) and remainingBytes != 0:
            assert remainingBytes > 0
            bufferedData = self.buffer[bufferIndex]

            if remainingBytes <= len(bufferedData) - bufferOffset:
                bytesToRead = remainingBytes
                self.position = [bufferIndex, bufferOffset + bytesToRead]
            else:
                bytesToRead = len(bufferedData) - bufferOffset
                self.position = [bufferIndex, len(bufferedData)]
                bufferIndex += 1
            rv.append(bufferedData[bufferOffset:bufferOffset + bytesToRead])
            remainingBytes -= bytesToRead

            bufferOffset = 0

        if remainingBytes:
            rv.append(self._readStream(remainingBytes))

        return b"".join(rv)


def HTMLInputStream(source, encoding=None, parseMeta=True, chardet=True):
    if isinstance(source, http_client.HTTPResponse):
        
        
        isUnicode = False
    elif hasattr(source, "read"):
        isUnicode = isinstance(source.read(0), text_type)
    else:
        isUnicode = isinstance(source, text_type)

    if isUnicode:
        if encoding is not None:
            raise TypeError("Cannot explicitly set an encoding with a unicode string")

        return HTMLUnicodeInputStream(source)
    else:
        return HTMLBinaryInputStream(source, encoding, parseMeta, chardet)


class HTMLUnicodeInputStream(object):
    """Provides a unicode stream of characters to the HTMLTokenizer.

    This class takes care of character encoding and removing or replacing
    incorrect byte-sequences and also provides column and line tracking.

    """

    _defaultChunkSize = 10240

    def __init__(self, source):
        """Initialises the HTMLInputStream.

        HTMLInputStream(source, [encoding]) -> Normalized stream from source
        for use by html5lib.

        source can be either a file-object, local filename or a string.

        The optional encoding parameter must be a string that indicates
        the encoding.  If specified, that encoding will be used,
        regardless of any BOM or later declaration (such as in a meta
        element)

        parseMeta - Look for a <meta> element containing encoding information

        """

        
        if len("\U0010FFFF") == 1:
            self.reportCharacterErrors = self.characterErrorsUCS4
            self.replaceCharactersRegexp = re.compile("[\uD800-\uDFFF]")
        else:
            self.reportCharacterErrors = self.characterErrorsUCS2
            self.replaceCharactersRegexp = re.compile("([\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF])")

        
        self.newLines = [0]

        self.charEncoding = ("utf-8", "certain")
        self.dataStream = self.openStream(source)

        self.reset()

    def reset(self):
        self.chunk = ""
        self.chunkSize = 0
        self.chunkOffset = 0
        self.errors = []

        
        self.prevNumLines = 0
        
        self.prevNumCols = 0

        
        self._bufferedCharacter = None

    def openStream(self, source):
        """Produces a file object from source.

        source can be either a file object, local filename or a string.

        """
        
        if hasattr(source, 'read'):
            stream = source
        else:
            stream = StringIO(source)

        return stream

    def _position(self, offset):
        chunk = self.chunk
        nLines = chunk.count('\n', 0, offset)
        positionLine = self.prevNumLines + nLines
        lastLinePos = chunk.rfind('\n', 0, offset)
        if lastLinePos == -1:
            positionColumn = self.prevNumCols + offset
        else:
            positionColumn = offset - (lastLinePos + 1)
        return (positionLine, positionColumn)

    def position(self):
        """Returns (line, col) of the current position in the stream."""
        line, col = self._position(self.chunkOffset)
        return (line + 1, col)

    def char(self):
        """ Read one character from the stream or queue if available. Return
            EOF when EOF is reached.
        """
        
        if self.chunkOffset >= self.chunkSize:
            if not self.readChunk():
                return EOF

        chunkOffset = self.chunkOffset
        char = self.chunk[chunkOffset]
        self.chunkOffset = chunkOffset + 1

        return char

    def readChunk(self, chunkSize=None):
        if chunkSize is None:
            chunkSize = self._defaultChunkSize

        self.prevNumLines, self.prevNumCols = self._position(self.chunkSize)

        self.chunk = ""
        self.chunkSize = 0
        self.chunkOffset = 0

        data = self.dataStream.read(chunkSize)

        
        if self._bufferedCharacter:
            data = self._bufferedCharacter + data
            self._bufferedCharacter = None
        elif not data:
            
            return False

        if len(data) > 1:
            lastv = ord(data[-1])
            if lastv == 0x0D or 0xD800 <= lastv <= 0xDBFF:
                self._bufferedCharacter = data[-1]
                data = data[:-1]

        self.reportCharacterErrors(data)

        
        
        data = self.replaceCharactersRegexp.sub("\ufffd", data)

        data = data.replace("\r\n", "\n")
        data = data.replace("\r", "\n")

        self.chunk = data
        self.chunkSize = len(data)

        return True

    def characterErrorsUCS4(self, data):
        for i in range(len(invalid_unicode_re.findall(data))):
            self.errors.append("invalid-codepoint")

    def characterErrorsUCS2(self, data):
        
        
        skip = False
        for match in invalid_unicode_re.finditer(data):
            if skip:
                continue
            codepoint = ord(match.group())
            pos = match.start()
            
            if utils.isSurrogatePair(data[pos:pos + 2]):
                
                char_val = utils.surrogatePairToCodepoint(data[pos:pos + 2])
                if char_val in non_bmp_invalid_codepoints:
                    self.errors.append("invalid-codepoint")
                skip = True
            elif (codepoint >= 0xD800 and codepoint <= 0xDFFF and
                  pos == len(data) - 1):
                self.errors.append("invalid-codepoint")
            else:
                skip = False
                self.errors.append("invalid-codepoint")

    def charsUntil(self, characters, opposite=False):
        """ Returns a string of characters from the stream up to but not
        including any character in 'characters' or EOF. 'characters' must be
        a container that supports the 'in' method and iteration over its
        characters.
        """

        
        try:
            chars = charsUntilRegEx[(characters, opposite)]
        except KeyError:
            if __debug__:
                for c in characters:
                    assert(ord(c) < 128)
            regex = "".join(["\\x%02x" % ord(c) for c in characters])
            if not opposite:
                regex = "^%s" % regex
            chars = charsUntilRegEx[(characters, opposite)] = re.compile("[%s]+" % regex)

        rv = []

        while True:
            
            m = chars.match(self.chunk, self.chunkOffset)
            if m is None:
                
                
                if self.chunkOffset != self.chunkSize:
                    break
            else:
                end = m.end()
                
                
                if end != self.chunkSize:
                    rv.append(self.chunk[self.chunkOffset:end])
                    self.chunkOffset = end
                    break
            
            
            rv.append(self.chunk[self.chunkOffset:])
            if not self.readChunk():
                
                break

        r = "".join(rv)
        return r

    def unget(self, char):
        
        
        if char is not None:
            if self.chunkOffset == 0:
                
                
                
                
                
                self.chunk = char + self.chunk
                self.chunkSize += 1
            else:
                self.chunkOffset -= 1
                assert self.chunk[self.chunkOffset] == char


class HTMLBinaryInputStream(HTMLUnicodeInputStream):
    """Provides a unicode stream of characters to the HTMLTokenizer.

    This class takes care of character encoding and removing or replacing
    incorrect byte-sequences and also provides column and line tracking.

    """

    def __init__(self, source, encoding=None, parseMeta=True, chardet=True):
        """Initialises the HTMLInputStream.

        HTMLInputStream(source, [encoding]) -> Normalized stream from source
        for use by html5lib.

        source can be either a file-object, local filename or a string.

        The optional encoding parameter must be a string that indicates
        the encoding.  If specified, that encoding will be used,
        regardless of any BOM or later declaration (such as in a meta
        element)

        parseMeta - Look for a <meta> element containing encoding information

        """
        
        
        self.rawStream = self.openStream(source)

        HTMLUnicodeInputStream.__init__(self, self.rawStream)

        self.charEncoding = (codecName(encoding), "certain")

        
        
        
        self.numBytesMeta = 512
        
        self.numBytesChardet = 100
        
        self.defaultEncoding = "windows-1252"

        
        if (self.charEncoding[0] is None):
            self.charEncoding = self.detectEncoding(parseMeta, chardet)

        
        self.reset()

    def reset(self):
        self.dataStream = codecs.getreader(self.charEncoding[0])(self.rawStream,
                                                                 'replace')
        HTMLUnicodeInputStream.reset(self)

    def openStream(self, source):
        """Produces a file object from source.

        source can be either a file object, local filename or a string.

        """
        
        if hasattr(source, 'read'):
            stream = source
        else:
            stream = BytesIO(source)

        try:
            stream.seek(stream.tell())
        except:
            stream = BufferedStream(stream)

        return stream

    def detectEncoding(self, parseMeta=True, chardet=True):
        
        
        encoding = self.detectBOM()
        confidence = "certain"
        
        
        if encoding is None and parseMeta:
            encoding = self.detectEncodingMeta()
            confidence = "tentative"
        
        if encoding is None and chardet:
            confidence = "tentative"
            try:
                try:
                    from charade.universaldetector import UniversalDetector
                except ImportError:
                    from chardet.universaldetector import UniversalDetector
                buffers = []
                detector = UniversalDetector()
                while not detector.done:
                    buffer = self.rawStream.read(self.numBytesChardet)
                    assert isinstance(buffer, bytes)
                    if not buffer:
                        break
                    buffers.append(buffer)
                    detector.feed(buffer)
                detector.close()
                encoding = detector.result['encoding']
                self.rawStream.seek(0)
            except ImportError:
                pass
        
        if encoding is None:
            confidence = "tentative"
            encoding = self.defaultEncoding

        
        encodingSub = {"iso-8859-1": "windows-1252"}

        if encoding.lower() in encodingSub:
            encoding = encodingSub[encoding.lower()]

        return encoding, confidence

    def changeEncoding(self, newEncoding):
        assert self.charEncoding[1] != "certain"
        newEncoding = codecName(newEncoding)
        if newEncoding in ("utf-16", "utf-16-be", "utf-16-le"):
            newEncoding = "utf-8"
        if newEncoding is None:
            return
        elif newEncoding == self.charEncoding[0]:
            self.charEncoding = (self.charEncoding[0], "certain")
        else:
            self.rawStream.seek(0)
            self.reset()
            self.charEncoding = (newEncoding, "certain")
            raise ReparseException("Encoding changed from %s to %s" % (self.charEncoding[0], newEncoding))

    def detectBOM(self):
        """Attempts to detect at BOM at the start of the stream. If
        an encoding can be determined from the BOM return the name of the
        encoding otherwise return None"""
        bomDict = {
            codecs.BOM_UTF8: 'utf-8',
            codecs.BOM_UTF16_LE: 'utf-16-le', codecs.BOM_UTF16_BE: 'utf-16-be',
            codecs.BOM_UTF32_LE: 'utf-32-le', codecs.BOM_UTF32_BE: 'utf-32-be'
        }

        
        string = self.rawStream.read(4)
        assert isinstance(string, bytes)

        
        encoding = bomDict.get(string[:3])         
        seek = 3
        if not encoding:
            
            encoding = bomDict.get(string)         
            seek = 4
            if not encoding:
                encoding = bomDict.get(string[:2])  
                seek = 2

        
        
        self.rawStream.seek(encoding and seek or 0)

        return encoding

    def detectEncodingMeta(self):
        """Report the encoding declared by the meta element
        """
        buffer = self.rawStream.read(self.numBytesMeta)
        assert isinstance(buffer, bytes)
        parser = EncodingParser(buffer)
        self.rawStream.seek(0)
        encoding = parser.getEncoding()

        if encoding in ("utf-16", "utf-16-be", "utf-16-le"):
            encoding = "utf-8"

        return encoding


class EncodingBytes(bytes):
    """String-like object with an associated position and various extra methods
    If the position is ever greater than the string length then an exception is
    raised"""
    def __new__(self, value):
        assert isinstance(value, bytes)
        return bytes.__new__(self, value.lower())

    def __init__(self, value):
        self._position = -1

    def __iter__(self):
        return self

    def __next__(self):
        p = self._position = self._position + 1
        if p >= len(self):
            raise StopIteration
        elif p < 0:
            raise TypeError
        return self[p:p + 1]

    def next(self):
        
        return self.__next__()

    def previous(self):
        p = self._position
        if p >= len(self):
            raise StopIteration
        elif p < 0:
            raise TypeError
        self._position = p = p - 1
        return self[p:p + 1]

    def setPosition(self, position):
        if self._position >= len(self):
            raise StopIteration
        self._position = position

    def getPosition(self):
        if self._position >= len(self):
            raise StopIteration
        if self._position >= 0:
            return self._position
        else:
            return None

    position = property(getPosition, setPosition)

    def getCurrentByte(self):
        return self[self.position:self.position + 1]

    currentByte = property(getCurrentByte)

    def skip(self, chars=spaceCharactersBytes):
        """Skip past a list of characters"""
        p = self.position               
        while p < len(self):
            c = self[p:p + 1]
            if c not in chars:
                self._position = p
                return c
            p += 1
        self._position = p
        return None

    def skipUntil(self, chars):
        p = self.position
        while p < len(self):
            c = self[p:p + 1]
            if c in chars:
                self._position = p
                return c
            p += 1
        self._position = p
        return None

    def matchBytes(self, bytes):
        """Look for a sequence of bytes at the start of a string. If the bytes
        are found return True and advance the position to the byte after the
        match. Otherwise return False and leave the position alone"""
        p = self.position
        data = self[p:p + len(bytes)]
        rv = data.startswith(bytes)
        if rv:
            self.position += len(bytes)
        return rv

    def jumpTo(self, bytes):
        """Look for the next sequence of bytes matching a given sequence. If
        a match is found advance the position to the last byte of the match"""
        newPosition = self[self.position:].find(bytes)
        if newPosition > -1:
            
            if self._position == -1:
                self._position = 0
            self._position += (newPosition + len(bytes) - 1)
            return True
        else:
            raise StopIteration


class EncodingParser(object):
    """Mini parser for detecting character encoding from meta elements"""

    def __init__(self, data):
        """string - the data to work on for encoding detection"""
        self.data = EncodingBytes(data)
        self.encoding = None

    def getEncoding(self):
        methodDispatch = (
            (b"<!--", self.handleComment),
            (b"<meta", self.handleMeta),
            (b"</", self.handlePossibleEndTag),
            (b"<!", self.handleOther),
            (b"<?", self.handleOther),
            (b"<", self.handlePossibleStartTag))
        for byte in self.data:
            keepParsing = True
            for key, method in methodDispatch:
                if self.data.matchBytes(key):
                    try:
                        keepParsing = method()
                        break
                    except StopIteration:
                        keepParsing = False
                        break
            if not keepParsing:
                break

        return self.encoding

    def handleComment(self):
        """Skip over comments"""
        return self.data.jumpTo(b"-->")

    def handleMeta(self):
        if self.data.currentByte not in spaceCharactersBytes:
            
            return True
        
        hasPragma = False
        pendingEncoding = None
        while True:
            
            attr = self.getAttribute()
            if attr is None:
                return True
            else:
                if attr[0] == b"http-equiv":
                    hasPragma = attr[1] == b"content-type"
                    if hasPragma and pendingEncoding is not None:
                        self.encoding = pendingEncoding
                        return False
                elif attr[0] == b"charset":
                    tentativeEncoding = attr[1]
                    codec = codecName(tentativeEncoding)
                    if codec is not None:
                        self.encoding = codec
                        return False
                elif attr[0] == b"content":
                    contentParser = ContentAttrParser(EncodingBytes(attr[1]))
                    tentativeEncoding = contentParser.parse()
                    if tentativeEncoding is not None:
                        codec = codecName(tentativeEncoding)
                        if codec is not None:
                            if hasPragma:
                                self.encoding = codec
                                return False
                            else:
                                pendingEncoding = codec

    def handlePossibleStartTag(self):
        return self.handlePossibleTag(False)

    def handlePossibleEndTag(self):
        next(self.data)
        return self.handlePossibleTag(True)

    def handlePossibleTag(self, endTag):
        data = self.data
        if data.currentByte not in asciiLettersBytes:
            
            
            
            if endTag:
                data.previous()
                self.handleOther()
            return True

        c = data.skipUntil(spacesAngleBrackets)
        if c == b"<":
            
            
            data.previous()
        else:
            
            attr = self.getAttribute()
            while attr is not None:
                attr = self.getAttribute()
        return True

    def handleOther(self):
        return self.data.jumpTo(b">")

    def getAttribute(self):
        """Return a name,value pair for the next attribute in the stream,
        if one is found, or None"""
        data = self.data
        
        c = data.skip(spaceCharactersBytes | frozenset([b"/"]))
        assert c is None or len(c) == 1
        
        if c in (b">", None):
            return None
        
        attrName = []
        attrValue = []
        
        while True:
            if c == b"=" and attrName:
                break
            elif c in spaceCharactersBytes:
                
                c = data.skip()
                break
            elif c in (b"/", b">"):
                return b"".join(attrName), b""
            elif c in asciiUppercaseBytes:
                attrName.append(c.lower())
            elif c is None:
                return None
            else:
                attrName.append(c)
            
            c = next(data)
        
        if c != b"=":
            data.previous()
            return b"".join(attrName), b""
        
        next(data)
        
        c = data.skip()
        
        if c in (b"'", b'"'):
            
            quoteChar = c
            while True:
                
                c = next(data)
                
                if c == quoteChar:
                    next(data)
                    return b"".join(attrName), b"".join(attrValue)
                
                elif c in asciiUppercaseBytes:
                    attrValue.append(c.lower())
                
                else:
                    attrValue.append(c)
        elif c == b">":
            return b"".join(attrName), b""
        elif c in asciiUppercaseBytes:
            attrValue.append(c.lower())
        elif c is None:
            return None
        else:
            attrValue.append(c)
        
        while True:
            c = next(data)
            if c in spacesAngleBrackets:
                return b"".join(attrName), b"".join(attrValue)
            elif c in asciiUppercaseBytes:
                attrValue.append(c.lower())
            elif c is None:
                return None
            else:
                attrValue.append(c)


class ContentAttrParser(object):
    def __init__(self, data):
        assert isinstance(data, bytes)
        self.data = data

    def parse(self):
        try:
            
            
            self.data.jumpTo(b"charset")
            self.data.position += 1
            self.data.skip()
            if not self.data.currentByte == b"=":
                
                return None
            self.data.position += 1
            self.data.skip()
            
            if self.data.currentByte in (b'"', b"'"):
                quoteMark = self.data.currentByte
                self.data.position += 1
                oldPosition = self.data.position
                if self.data.jumpTo(quoteMark):
                    return self.data[oldPosition:self.data.position]
                else:
                    return None
            else:
                
                oldPosition = self.data.position
                try:
                    self.data.skipUntil(spaceCharactersBytes)
                    return self.data[oldPosition:self.data.position]
                except StopIteration:
                    
                    return self.data[oldPosition:]
        except StopIteration:
            return None


def codecName(encoding):
    """Return the python codec name corresponding to an encoding or None if the
    string doesn't correspond to a valid encoding."""
    if isinstance(encoding, bytes):
        try:
            encoding = encoding.decode("ascii")
        except UnicodeDecodeError:
            return None
    if encoding:
        canonicalName = ascii_punctuation_re.sub("", encoding).lower()
        return encodings.get(canonicalName, None)
    else:
        return None
