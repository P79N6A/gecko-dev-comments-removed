



































import java.io.File;
import java.io.FileNotFoundException;
import java.util.Properties;

import org.mozilla.xpcom.GREVersionRange;
import org.mozilla.xpcom.Mozilla;
import org.mozilla.interfaces.nsIFile;
import org.mozilla.interfaces.nsILocalFile;
import org.mozilla.interfaces.nsISimpleEnumerator;
import org.mozilla.interfaces.nsISupports;














public class TestJavaProxy {
	public static void main(String [] args) throws Exception {
    GREVersionRange[] range = new GREVersionRange[1];
    range[0] = new GREVersionRange("1.8", true, "1.9+", true);
    Properties props = null;
      
    File grePath = null;
    try {
      grePath = Mozilla.getGREPathWithProperties(range, props);
    } catch (FileNotFoundException e) { }
      
    if (grePath == null) {
      System.out.println("found no GRE PATH");
      return;
    }
    System.out.println("GRE PATH = " + grePath.getPath());
    
    Mozilla Moz = Mozilla.getInstance();
    try {
      Moz.initXPCOM(grePath, null);
    } catch (IllegalArgumentException e) {
      System.out.println("no javaxpcom.jar found in given path");
      return;
    } catch (Throwable t) {
      System.out.println("initXPCOM failed");
      t.printStackTrace();
      return;
    }
    System.out.println("\n--> initialized\n");

		nsILocalFile directory = Moz.newLocalFile("/usr", false);
		nsISimpleEnumerator entries = (nsISimpleEnumerator)
        directory.getDirectoryEntries();
		while (entries.hasMoreElements()) {
			nsISupports supp = entries.getNext();
			nsIFile file = (nsIFile) supp.queryInterface(nsIFile.NS_IFILE_IID);
			System.out.println(file.getPath());
		}

    
    directory = null;
    entries = null;

		Moz.shutdownXPCOM(null);
	}
}
