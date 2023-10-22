#include <sys/types.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <vector>
#include <string>
#include <sstream>

#include <v8.h>

#include "V8Runner.h"
#include "jv8.h"
#include "JNIUtil.h"

using namespace std;
using namespace v8;

struct MappableMethodData {
  jobject methodObject;
  jv8::V8Runner* runner;
  JavaVM* jvm;
};

namespace jv8 {  

  Isolate* V8Runner::dbg_isolate;
  Handle<Context> V8Runner::dbg_context;
  bool V8Runner::debuggingInitialized = 0;

  V8Runner::V8Runner () {
    isolate = Isolate::New();
    Locker l(isolate);
    Isolate::Scope isolateScope(isolate);

    HandleScope handle_scope;

    context = Persistent<Context>(Context::New());
  }

  void V8Runner::mapMethod (JNIEnv* env,  jobject v8MappableMethod, const char* name) {

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "jv8", "Mapping method %s", name);
#endif

    Locker l(isolate);
    Isolate::Scope isolateScope(isolate);

    HandleScope handle_scope;

    Context::Scope context_scope(context);
    
    MappableMethodData* data = new MappableMethodData();
    data->runner = this;
    data->methodObject = env->NewGlobalRef(v8MappableMethod);
    env->GetJavaVM(&data->jvm);
    methodDatas.push_back(data);

    Handle<Object> global = context->Global();
    global->Set(String::New(name), FunctionTemplate::New(&onMappableMethodCalled, External::New(data))->GetFunction());
  }

  Handle<Value> V8Runner::callFunction(Handle<Function> function, std::vector<Handle<Value> > args) {
    return function->Call(context->Global(), args.size(), &args[0]);
  }

  void V8Runner::destroy (JNIEnv* env) {
    for (uint i=0; i<methodDatas.size(); ++i) {
      env->DeleteGlobalRef(methodDatas[i]->methodObject);
      delete methodDatas[i];
    }
  }

  Isolate* V8Runner::getIsolate() {
    return isolate;
  }

  Handle<Context>& V8Runner::getContext() {
    return context;
  }

  jobject V8Runner::runJS (JNIEnv *env, jstring jname, jstring jjs) {
    if (JNIUtil::needsToCacheClassData) {
      cacheClassData(env);
    }

    Locker l(isolate);
    Isolate::Scope isolateScope(isolate);

    HandleScope handle_scope;

    Context::Scope context_scope(context);

    const char* js = env->GetStringUTFChars(jjs, NULL);
    const char* name = env->GetStringUTFChars(jname, NULL);

    // Create a string containing the JavaScript source code.
    Handle<String> source = String::New(js);

    env->ReleaseStringUTFChars(jjs, js);

    TryCatch tryCatch;

    // Compile the source code.
    Handle<Script> script = Script::Compile(source, String::New(name));
    env->ReleaseStringUTFChars(jname, name);
    
    Handle<Value> result;

    if (script.IsEmpty()) {
      THROW_V8EXCEPTION(env, tryCatch)
      return NULL;
    }

    // Run the script to get the result.
    result = script->Run();

    if (result.IsEmpty()) {
      THROW_V8EXCEPTION(env, tryCatch)
      return NULL;
    }

    return jObjectFromV8Value(env, result);
  }

  jobject V8Runner::callFunction (
    JNIEnv *env,
    jobject jfunction,
    jobjectArray jargs
  ) {

    if( jfunction == NULL ){

#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_INFO, "jv8", "callFunction called with `jfunction == NULL`. Returning NULL.");
#endif

      return NULL;
    }

    Locker l(isolate);
    Isolate::Scope isolateScope(isolate);

    HandleScope handle_scope(isolate);
    Context::Scope context_scope(context);

    if (JNIUtil::needsToCacheClassData) {
      cacheClassData(env);
    }

    jlong functionPointer = env->GetLongField(jfunction, JNIUtil::f_Function_handle);
    Persistent<Function> function = Persistent<Function>((Function*) functionPointer);

    if( function.IsEmpty() || *function == NULL ){

#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_INFO, "jv8", "callFunction called with `function == NULL`. Returning NULL.");
#endif

      return NULL;
    }

    std::vector<Handle<Value> > args;
    int length = env->GetArrayLength(jargs);
    for (int i=0; i<length; i++) {
      jobject obj = env->GetObjectArrayElement(jargs, i);
      Handle<Value> handle = v8ValueFromJObject(env, obj);
      args.push_back(handle);
    }

    TryCatch tryCatch;

    Handle<Value> returnedJSValue = callFunction(function, args);

    if( returnedJSValue.IsEmpty() ){
      THROW_V8EXCEPTION(env, tryCatch)
      return NULL;
    }

    return jObjectFromV8Value(env, returnedJSValue);
  }

  /**
   * Releases the v8 function handler.
   */
  void V8Runner::disposeFunction(
    JNIEnv* env,
    jobject jfunction
  ) {
    Locker l(isolate);
    Isolate::Scope isolateScope(isolate);

    HandleScope handle_scope(isolate);
    Context::Scope context_scope(context);

    jlong functionPointer = env->GetLongField(jfunction, JNIUtil::f_Function_handle);
    Persistent<Function> function = Persistent<Function>((Function*) functionPointer);

    // Release the JS function so it can be collected.
    function.Dispose(isolate);
    function.Clear();
  }

  void V8Runner::printStackTrace(){
    Local<StackTrace> ex = StackTrace::CurrentStackTrace(1000);
    int frameCount = ex->GetFrameCount();
    std::stringstream stackTraceString("");
    for (int i=0; i<frameCount; i++) {
      Local<StackFrame> frame = ex->GetFrame(i);
      stackTraceString << *String::Utf8Value(frame->GetFunctionName())
                      << " ("
                      << *String::Utf8Value(frame->GetScriptName())
                      << ":"
                      << frame->GetLineNumber()
                      << ")"
                      << "\n";
    }

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "jv8", "Current stack trace:\n%s", stackTraceString.str().c_str());
#endif

  }

  Handle<Value>
  V8Runner::onMappableMethodCalled (const Arguments& args) {
    Isolate* isolate = args.GetIsolate();

    Locker l(isolate);
    Isolate::Scope isolateScope(isolate);

    HandleScope handle_scope(isolate);

    MappableMethodData* data = (MappableMethodData*) External::Cast(*args.Data())->Value();
    JNIEnv* env;
    JNIUtil::javaVM->AttachCurrentThread(&env, NULL);
    
    if (JNIUtil::needsToCacheClassData) {
      cacheClassData(env);
    }

    jobject methodObject = data->methodObject;
    jobjectArray jargs = (jobjectArray) env->NewObjectArray(args.Length(), JNIUtil::Object_class, NULL);
    for (int i=0; i<args.Length(); ++i) {
      jobject wrappedArg = data->runner->jObjectFromV8Value(env, args[i]);

      env->SetObjectArrayElement(jargs, i, wrappedArg);
      env->DeleteLocalRef(wrappedArg);
    }

    Handle<Value> returnVal;

    jobject jresult = env->CallObjectMethod(methodObject, JNIUtil::m_V8MappableMethod_runMethod, jargs);

    env->DeleteLocalRef(jargs);

    returnVal = data->runner->v8ValueFromJObject(env, jresult);

    env->DeleteLocalRef(jresult);

    return returnVal; // TODO
  }

  /**
   * ============= OBJECT CONVERSION (Java <=> JS) ===========
   */

  Handle<Value>
  V8Runner::v8ValueFromJObject (
    JNIEnv* env,
    jobject jobj
  ) {

    if (JNIUtil::needsToCacheClassData) {
      cacheClassData(env);
    }

    if (jobj == NULL) {
      return Null();
    }

    Handle<Value> returnVal;

    // Number
    if (env->IsInstanceOf(jobj, JNIUtil::Number_class)) {
      jdouble num = env->CallDoubleMethod(jobj, JNIUtil::m_Number_doubleValue);    
      returnVal = Number::New(num);    
    }

    // Boolean
    else if (env->IsInstanceOf(jobj, JNIUtil::Boolean_class)) {
      jboolean b = env->CallBooleanMethod(jobj, JNIUtil::m_Boolean_booleanValue);    
      returnVal = Boolean::New(b);
    }

    // String
    else if (env->IsInstanceOf(jobj, JNIUtil::String_class)) {
      jstring jstr = (jstring)jobj;
      const char* str = env->GetStringUTFChars(jstr, 0);
      returnVal = String::New(str);
      env->ReleaseStringUTFChars(jstr, str);
    }

    // Function
    else if (env->IsInstanceOf(jobj, JNIUtil::Function_class)) {
      jlong functionPointer = env->GetLongField(jobj, JNIUtil::f_Function_handle);
      v8::Handle<v8::Function>(reinterpret_cast<v8::Function*>(functionPointer));
    }

    // Default to Undefined
    else {
      returnVal = Undefined();
    }

    return returnVal;
  }

  jobject
  V8Runner::jObjectFromV8Value (
    JNIEnv* env,
    Handle<Value> value
  ) {
    if (JNIUtil::needsToCacheClassData) {
      cacheClassData(env);
    }

    jobject wrappedReturnValue;

    // Check whether we have an empty handle.
    if( value.IsEmpty() ){

#ifdef __ANDROID__
      __android_log_write(ANDROID_LOG_WARN, "jv8", "JS=>Java: Empty value provided. Defaulting to null.");
#endif

      printStackTrace();
      return NULL;
    }

    // Number
    if (value->IsNumber()) {
      wrappedReturnValue = env->NewObject(JNIUtil::Double_class,
        JNIUtil::m_Double_init_double,
        value->NumberValue()
      );
    }

    // String
    else if (value->IsString()) {
      wrappedReturnValue = env->NewStringUTF( *String::Utf8Value(value->ToString()) );
    }

    // Boolean
    else if (value->IsBoolean()) {
      wrappedReturnValue = env->NewObject(JNIUtil::Boolean_class,
        JNIUtil::m_Boolean_init_bool,
        value->BooleanValue()
      );

    }

    // Function
    else if( value->IsFunction()) {
      Persistent<Function> functionPersistent = Persistent<Function>::New(Handle<Function>::Cast(value));
      functionPersistent.MarkIndependent();
      wrappedReturnValue = env->NewObject(JNIUtil::Function_class,
        JNIUtil::m_Function_init,
        (jlong)this,
        (jlong)*functionPersistent
      );
    }

    // Undefined
    else if( value->IsUndefined()) {
      wrappedReturnValue = env->GetStaticObjectField(JNIUtil::Undefined_class, JNIUtil::sf_Undefined_instance);
    }

    // null
    else if( value->IsNull() ){
      wrappedReturnValue = NULL;
    }

    // Default to null
    else {
      std::string jsType(std::string(*String::Utf8Value(value->ToString())));

#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_WARN, "jv8", "JS=>Java: Unsupported JS type detected: %s", jsType.c_str());
#endif

      printStackTrace();
      wrappedReturnValue = NULL;
    }

    return wrappedReturnValue;
  }

  /**
   * =========== DEBUGGER (all methods are static) ==========
   */

  void V8Runner::dispatchDebugMessages () {
    Locker l(dbg_isolate);
    Isolate::Scope isolateScope(dbg_isolate);

    HandleScope handle_scope;

    Context::Scope context_scope(dbg_context);

    Debug::ProcessDebugMessages();
  }

  void V8Runner::initRemoteDebugging (int port, bool waitForConnection) {
    if (!debuggingInitialized) {

#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_INFO, "jv8", "Initting debugger on port %d", port);
#endif
      
      Debug::SetDebugMessageDispatchHandler(dispatchDebugMessages, true);
      Debug::EnableAgent("jv8", port, waitForConnection);
      debuggingInitialized = true;
    }
  }

  void V8Runner::setDebuggingRunner (V8Runner* runner, int port, bool waitForConnection) {
    dbg_isolate = runner->getIsolate();

    Locker l(dbg_isolate);
    Isolate::Scope isolateScope(dbg_isolate);

    HandleScope handle_scope;

    Context::Scope context_scope(runner->getContext());

    dbg_context = Persistent<Context>::New(dbg_isolate, runner->getContext());
    initRemoteDebugging(port, waitForConnection);
  }

  void V8Runner::disableDebugging () {
    Debug::DisableAgent();
  }

} // namespace jv8