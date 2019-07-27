
































#include "pkix/Input.h"

namespace mozilla { namespace pkix {

bool
IsValidDNSName(Input hostname)
{
  if (hostname.GetLength() > 253) {
    return false;
  }

  Reader input(hostname);
  size_t labelLength = 0;
  bool labelIsAllNumeric = false;
  bool endsWithHyphen = false;

  do {
    static const size_t MAX_LABEL_LENGTH = 63;

    uint8_t b;
    if (input.Read(b) != Success) {
      return false;
    }
    switch (b) {
      case '-':
        if (labelLength == 0) {
          return false; 
        }
        labelIsAllNumeric = false;
        endsWithHyphen = true;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      
      
      case '0': case '5':
      case '1': case '6':
      case '2': case '7':
      case '3': case '8':
      case '4': case '9':
        if (labelLength == 0) {
          labelIsAllNumeric = true;
        }
        endsWithHyphen = false;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      
      
      
      case 'a': case 'A': case 'n': case 'N':
      case 'b': case 'B': case 'o': case 'O':
      case 'c': case 'C': case 'p': case 'P':
      case 'd': case 'D': case 'q': case 'Q':
      case 'e': case 'E': case 'r': case 'R':
      case 'f': case 'F': case 's': case 'S':
      case 'g': case 'G': case 't': case 'T':
      case 'h': case 'H': case 'u': case 'U':
      case 'i': case 'I': case 'v': case 'V':
      case 'j': case 'J': case 'w': case 'W':
      case 'k': case 'K': case 'x': case 'X':
      case 'l': case 'L': case 'y': case 'Y':
      case 'm': case 'M': case 'z': case 'Z':
        labelIsAllNumeric = false;
        endsWithHyphen = false;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      case '.':
        if (labelLength == 0) {
          return false;
        }
        if (endsWithHyphen) {
          return false; 
        }
        labelLength = 0;
        break;

      default:
        return false; 
    }
  } while (!input.AtEnd());

  if (endsWithHyphen) {
    return false; 
  }

  if (labelIsAllNumeric) {
    return false; 
  }

  return true;
}

} } 
