package com.jovianware.jv8;

/**
 * An instance of V8Function has a v8::Function equivalent on the native side.
 * The link between those structures is stored in a map inside v8.cpp.
 */
public class Function extends V8Object {

  private long handle = 0;
  private long runnerHandle = 0;

  protected Function(){

  }

  /**
   * Constructs a function with a native JS Function pointer.
   * @param handle A pointer to Persistent<Function>
   */
  protected Function( long runnerHandle, long handle ){
    this.runnerHandle = runnerHandle;
    this.handle = handle;
  }

  /**
   * @return Whether the V8Function is bound to a v8::Function yet
   * It can be false if we instantiate V8Function with a String
   * and that jv8 hasn't created the v8::Function yet.
   */
  public boolean isLinked(){
    return handle != 0;
  }

  /**
   * Notifies C++ that it can release the handler.
   */
  native protected void dispose();

  protected void finalize() throws Throwable {
    dispose();
  }
}
