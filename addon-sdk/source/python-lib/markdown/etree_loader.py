
from markdown import message, CRITICAL
import sys


def importETree():
    """Import the best implementation of ElementTree, return a module object."""
    etree_in_c = None
    try: 
        import xml.etree.cElementTree as etree_in_c
    except ImportError:
        try: 
            import xml.etree.ElementTree as etree
        except ImportError:
            try: 
                import cElementTree as etree_in_c
            except ImportError:
                try: 
                    import elementtree.ElementTree as etree
                except ImportError:
                    message(CRITICAL, "Failed to import ElementTree")
                    sys.exit(1)
    if etree_in_c and etree_in_c.VERSION < "1.0":
        message(CRITICAL, "For cElementTree version 1.0 or higher is required.")
        sys.exit(1)
    elif etree_in_c :
        return etree_in_c
    elif etree.VERSION < "1.1":
        message(CRITICAL, "For ElementTree version 1.1 or higher is required")
        sys.exit(1)
    else :
        return etree

