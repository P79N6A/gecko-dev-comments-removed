




































import org.mozilla.xpcom.VersionComparator;

public class TestVersionComparator {

  public static void main(String[] args) {
    String[] comparisons = {
        "0.9",
        "0.9.1",
        "1.0pre1",
        "1.0pre2",
        "1.0",
        "1.1pre",
        "1.1pre1a",
        "1.1pre1",
        "1.1pre10a",
        "1.1pre10",
        "1.1",
        "1.1.0.1",
        "1.1.1",
        "1.1.*",
        "1.*",
        "2.0",
        "2.1",
        "3.0.-1",
        "3.0"
    };

    String[] equality = {
        "1.1pre",
        "1.1pre0",
        "1.0+"
    };

    VersionComparator comp = new VersionComparator();

    
    for (int i = 0; i < comparisons.length - 1; i++) {
      int res = comp.compare(comparisons[i], comparisons[i + 1]);
      _assert(res < 0);

      res = comp.compare(comparisons[i + 1], comparisons[i]);
      _assert(res > 0);
    }

    
    for (int i = 0; i < equality.length - 1; i++) {
      int res = comp.compare(equality[i], equality[i + 1]);
      _assert(res == 0);

      res = comp.compare(equality[i + 1], equality[i]);
      _assert(res == 0);
    }

    System.out.println("-- passed");
  }

  private static void _assert(boolean expression) {
    if (!expression) {
      Throwable t = new Throwable();
      t.printStackTrace();
      System.exit(1);
    }
  }
}

