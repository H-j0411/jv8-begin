package com.jovianware.jv8;

public class V8Runner {

  static {
    try {
      System.loadLibrary("gnustl_shared");
    } catch (UnsatisfiedLinkError e) {
      e.printStackTrace();
    }
    
    System.loadLibrary("jv8");
  }

  /**
   * Creates a new V8 Runner. V8 Runners don't act on the same JS context
   * so they are completely independent. A V8 Runner corresponds to a single
   * instance of V8 (isolate).
   */
  public V8Runner() {
    handle = create();
  }

  /**
   * Enables the v8 debugger.
   *
   * @param v8Runner
   * @param port
   * @param waitForConnection
   */
  public static native void setDebuggingRunner(V8Runner v8Runner, int port, boolean waitForConnection);

  /**
   * Calls a function.
   *
   * @param function
   * @param args
   *
   * @return Result of the function call.
   *
   * @throws V8Exception If something wrong happened on the JS side.
   */
  public native Object callFunction(Function function, Object[] args) throws V8Exception;

  /**
   * Runs a JS script in the global context.
   *
   * @param name Script filename (used for stacktraces)
   * @param src  Script content
   *
   * @return Result of the JS call.
   *
   * @throws V8Exception
   */
  public native Object runJS(String name, String src) throws V8Exception;

  /**
   * Prints the current JS stack trace in Android's log system (tag=jv8, type=info)
   */
  public native void printStackTrace();

  /**
   * Attach a MappableMethod to the JS context. Everytime the `name` function is called,
   * methodToRun is called with the provided arguments.
   *
   * @param name  Name of the JS function
   * @param m     A subclass of V8MappableMethod
   */
  public native void map(String name, V8MappableMethod m);

  /**
   * @return The singleton instance of Undefined.
   */
  public Undefined Undefined() {
    return Undefined.instance;
  }

  /**
   * ====== PRIVATE =====
   */
  
  private long handle;
  private native long create();
  private native void dispose();
  private Undefined Undefined_;

  @Override
  protected void finalize() {
    dispose();
  }
}
