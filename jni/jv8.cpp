#include <jni.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <v8.h>

#include "jv8.h"
#include "V8Runner.h"
#include "JNIUtil.h"

using namespace v8;

// TODO: Integrate this in with build system:
//#define NDK_GDB_ENTRYPOINT_WAIT_HACK

namespace jv8 {

//////////////////////////////////////////////////////////////////////////////////////////////
// V8RUNNER CLASS
//////////////////////////////////////////////////////////////////////////////////////////////

static void V8Runner_setDebuggingRunner (
  JNIEnv *env,
  jclass V8runner_class,
  jobject jrunner,
  jint port,
  jboolean waitForConnection
) {
  V8Runner* runner = (V8Runner*) env->GetLongField(jrunner, JNIUtil::f_V8Runner_handle);
  if (runner != NULL)  {
    runner->setDebuggingRunner(runner, port, waitForConnection);
  } else {
    runner->disableDebugging();
  }
}

static jobject V8Runner_runJS (
  JNIEnv *env,
  jobject jrunner,
  jstring jname,
  jstring jjs
) {
  V8Runner* runner = (V8Runner*) env->GetLongField(jrunner, JNIUtil::f_V8Runner_handle);
  return runner->runJS(env, jname, jjs);
}

static jlong V8Runner_create (
  JNIEnv *env,
  jclass klass
) {
  return (jlong) new V8Runner();
}

static void V8Runner_dispose (
  JNIEnv *env,
  jclass obj
) {
  V8Runner* r = (V8Runner*) env->GetLongField(obj, JNIUtil::f_V8Runner_handle);
  if (r) {
    r->destroy(env);
    delete r;
  }
}

static void V8Runner_map (
  JNIEnv *env,
  jobject jrunner,
  jstring jname,
  jobject jmappableMethod
) {
  V8Runner* runner = (V8Runner*) env->GetLongField(jrunner, JNIUtil::f_V8Runner_handle);
  const char* name = env->GetStringUTFChars(jname, NULL);
  runner->mapMethod(env, jmappableMethod, name);
  env->ReleaseStringUTFChars(jname, name);
}

static jobject V8Runner_callFunction (
  JNIEnv *env,
  jobject jrunner,
  jobject jfunction,
  jobjectArray jargs
) {
  V8Runner* runner = (V8Runner*) env->GetLongField(jrunner, JNIUtil::f_V8Runner_handle);
  return runner->callFunction(env, jfunction, jargs);
}

static void V8Runner_disposeFunction(
  JNIEnv* env,
  jobject jfunction
) {

  if (JNIUtil::needsToCacheClassData) {
    cacheClassData(env);
  }

  V8Runner* runner = (V8Runner*) env->GetLongField(jfunction, JNIUtil::f_Function_runnerHandle);
  runner->disposeFunction(env, jfunction);
}

static void V8Runner_printStackTrace (
  JNIEnv *env,
  jobject jrunner
) {
  V8Runner* runner = (V8Runner*) env->GetLongField(jrunner, JNIUtil::f_V8Runner_handle);
  runner->printStackTrace();
}

} // namespace jv8

static JNINativeMethod V8Runner_Methods[] = {
  {(char*)"create", (char*)"()J", (void *) jv8::V8Runner_create},
  {(char*)"dispose", (char*)"()V", (void *) jv8::V8Runner_dispose},
  {(char*)"runJS", (char*)"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;", (void *) jv8::V8Runner_runJS},
  {(char*)"map", (char*)"(Ljava/lang/String;Lcom/jovianware/jv8/V8MappableMethod;)V", (void *) jv8::V8Runner_map},
  {(char*)"setDebuggingRunner", (char*)"(Lcom/jovianware/jv8/V8Runner;IZ)V", (void *) jv8::V8Runner_setDebuggingRunner},
  {(char*)"callFunction", (char*)"(Lcom/jovianware/jv8/Function;[Ljava/lang/Object;)Ljava/lang/Object;", (void *)jv8::V8Runner_callFunction},
  {(char*)"printStackTrace", (char*)"()V", (void *)jv8::V8Runner_printStackTrace}
};

static JNINativeMethod Function_Methods[] = {
  {(char*)"dispose", (char*)"()V", (void *) jv8::V8Runner_disposeFunction}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// GLUE CODE
//////////////////////////////////////////////////////////////////////////////////////////////
// Lookup class, and Handle:
#define ENV_INIT(vm)\
  JNIEnv* env;\
  if ((vm)->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {\
    return -1;\
  }\
  jclass klass;\

#define CLASS(className, globalRef)\
  klass = NULL;\
  klass = (env)->FindClass(className);\
  if (!klass) { return -1; }\
  globalRef = reinterpret_cast<jclass>(env->NewGlobalRef(klass));\

#define MTABLE(method_table)\
  env->RegisterNatives(\
    klass,\
    (method_table),\
    sizeof((method_table)) / sizeof((method_table)[0])\
  );\

#define FIELD(fieldName, signature, field)\
  (field) = env->GetFieldID(klass, (fieldName), (signature));\
  if ((field) == NULL) return -1;\

// Lookup everything else:
#define METHOD(method, signature, methodID)\
  (methodID) = env->GetMethodID(klass, (method), (signature));\

#define CLASS_END()\
  env->DeleteLocalRef(klass);\

// We need to tell the JNI environment to find our method names and properly
//  map them to our C++ methods.
extern "C" {
jint JNI_OnLoad (
  JavaVM* vm,
  void* reserved
) {
  jv8::JNIUtil::javaVM = vm;
  jv8::JNIUtil::needsToCacheClassData = true;

  // Sometimes stuff breaks before `ndk-gdb` has a chance to notice.
  // Vigorously spin our wheels until `ndk-gdb` catches us.
  #ifdef NDK_GDB_ENTRYPOINT_WAIT_HACK
  int i=0;
  while(!i);
  #endif
  ENV_INIT(vm)

  CLASS("com/jovianware/jv8/V8Runner", jv8::JNIUtil::V8Runner_class)
  MTABLE(V8Runner_Methods)
  FIELD("handle", "J", jv8::JNIUtil::f_V8Runner_handle)
  CLASS_END()

  CLASS("com/jovianware/jv8/Function", jv8::JNIUtil::Function_class)
  MTABLE(Function_Methods)
  CLASS_END()

  CLASS("com/jovianware/jv8/V8Exception", jv8::JNIUtil::V8Exception_class)
  CLASS_END()

  return JNI_VERSION_1_6;
}

} // extern "C"
