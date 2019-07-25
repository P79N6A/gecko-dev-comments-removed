



package org.mozilla.gecko.sync;

import java.util.List;

public abstract class CommandRunner {
  public final int argCount;

  public CommandRunner(int argCount) {
    this.argCount = argCount;
  }

  public abstract void executeCommand(GlobalSession session, List<String> args);

  public boolean argumentsAreValid(List<String> args) {
    return args != null &&
           args.size() == argCount;
  }
}
