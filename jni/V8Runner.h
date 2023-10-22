#ifndef V8RUNNER_H_
#define V8RUNNER_H_
#include <vector>

#include <jni.h>

#include <v8.h>
#include <v8-debug.h>

#include "jv8.h"

using namespace std;
using namespace v8;

struct MappableMethodData;

namespace jv8 {

  class V8Runner {
    Isolate* isolate;
    Persistent<Context> context;
    vector<MappableMethodData*> methodDatas;

    static Isolate* dbg_isolate;
    static Handle<Context> dbg_context;
    static bool debuggingInitialized;

  public:
    V8Runner();

    Handle<Value> runJS(const char* str);
    void mapMethod (JNIEnv* env,  jobject v8MappableMethod, const char* name);
    void destroy(JNIEnv* env);
    Handle<Value> callFunction(Handle<Function> function, std::vector<Handle<Value> > args);
    Isolate* getIsolate();
    Handle<Context>& getContext();

    jobject runJS (JNIEnv *env, jstring jname, jstring jjs);
    jobject callFunction (JNIEnv *env, jobject jfunction, jobjectArray jargs);
    void disposeFunction(JNIEnv* env, jobject jfunction);

    Handle<Value> v8ValueFromJObject (JNIEnv* env, jobject jobj);
    jobject jObjectFromV8Value (JNIEnv* env, Handle<Value> value);

    void printStackTrace();

    static Handle<Value> onMappableMethodCalled (const Arguments& args);

    // Debugger
    static void setDebuggingRunner (V8Runner* runner, int port, bool waitForConnection);
    static void disableDebugging ();

  protected:
    static void initRemoteDebugging (int port, bool waitForConnection);
    static void dispatchDebugMessages ();
  };

} // namespace jv8

#endif // V8RUNNER_H_