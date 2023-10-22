#ifndef JV8_H_
#define JV8_H_
#include <jni.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <v8.h>

using namespace v8;

#include "JNIUtil.h"

namespace jv8 {

class V8Runner;

#define REQUIRE_CLASS(className, globalRef)\
  if (!globalRef) {\
    jclass klass = NULL;\
    klass = (env)->FindClass(className);\
    globalRef = reinterpret_cast<jclass>(env->NewGlobalRef(klass));\
    env->DeleteLocalRef(klass);\
  }\

#define REQUIRE_METHOD(klass, method, signature, methodID)\
  if (!methodID) {\
    (methodID) = env->GetMethodID(klass, (method), (signature));\
  }\

#define REQUIRE_FIELD(klass, field, signature, fieldID)\
  if (!fieldID) {\
    (fieldID) = env->GetFieldID(klass, (field), (signature));\
  }\

#define REQUIRE_STATIC_FIELD(klass, field, signature, fieldID)\
  if (!fieldID) {\
    (fieldID) = env->GetStaticFieldID(klass, (field), (signature));\
  }\

#define THROW_V8EXCEPTION(env, tryCatch)\
  if (!tryCatch.HasCaught()) {\
    env->ThrowNew(JNIUtil::V8Exception_class, "Unexpected Error running JS");\
  }\
  else {\
    jstring msg = env->NewStringUTF( *String::Utf8Value(tryCatch.Exception()) );\
    jstring filename = env->NewStringUTF( *String::Utf8Value(tryCatch.Message()->GetScriptResourceName()) );\
    int lineNumber = tryCatch.Message()->GetLineNumber();\
    jstring stackTrace = env->NewStringUTF( *String::Utf8Value(tryCatch.StackTrace()) );\
    \
    jthrowable jException = (jthrowable)env->NewObject(\
      JNIUtil::V8Exception_class,\
      JNIUtil::m_V8Exception_init_str_str_int_str,\
      msg,\
      filename,\
      lineNumber,\
      stackTrace\
    );\
    \
    env->Throw(jException);\
  }\

static inline void
cacheClassData(JNIEnv* env) {

#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "jv8", "Caching JNI methods");
#endif

  // Undefined
  REQUIRE_CLASS("com/jovianware/jv8/Undefined", JNIUtil::Undefined_class)
  REQUIRE_STATIC_FIELD(JNIUtil::Undefined_class, "instance", "Lcom/jovianware/jv8/Undefined;", JNIUtil::sf_Undefined_instance)

  // Function
  REQUIRE_CLASS("com/jovianware/jv8/Function", JNIUtil::Function_class)
  REQUIRE_METHOD(JNIUtil::Function_class, "<init>", "(JJ)V", JNIUtil::m_Function_init)
  REQUIRE_FIELD(JNIUtil::Function_class, "handle", "J", JNIUtil::f_Function_handle)
  REQUIRE_FIELD(JNIUtil::Function_class, "runnerHandle", "J", JNIUtil::f_Function_runnerHandle)

  // V8MappableMethod
  REQUIRE_CLASS("com/jovianware/jv8/V8MappableMethod", JNIUtil::V8MappableMethod_class)
  REQUIRE_METHOD(JNIUtil::V8MappableMethod_class, "methodToRun", "([Ljava/lang/Object;)Ljava/lang/Object;", JNIUtil::m_V8MappableMethod_runMethod)

  // V8Exception
  REQUIRE_CLASS("com/jovianware/jv8/V8Exception", JNIUtil::V8Exception_class)
  REQUIRE_METHOD(JNIUtil::V8Exception_class, "<init>", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)V", JNIUtil::m_V8Exception_init_str_str_int_str)

  // Object
  REQUIRE_CLASS("java/lang/Object", JNIUtil::Object_class)

  // String
  REQUIRE_CLASS("java/lang/String", JNIUtil::String_class)
  REQUIRE_METHOD(JNIUtil::String_class, "<init>", "(Ljava/lang/String;)V", JNIUtil::m_String_init_str)

  // Boolean
  REQUIRE_CLASS("java/lang/Boolean", JNIUtil::Boolean_class)
  REQUIRE_METHOD(JNIUtil::Boolean_class, "<init>", "(Z)V", JNIUtil::m_Boolean_init_bool)
  REQUIRE_METHOD(JNIUtil::Boolean_class, "booleanValue", "()Z", JNIUtil::m_Boolean_booleanValue)

  // Number
  REQUIRE_CLASS("java/lang/Number", JNIUtil::Number_class)
  REQUIRE_METHOD(JNIUtil::Number_class, "doubleValue", "()D", JNIUtil::m_Number_doubleValue)

  // Double
  REQUIRE_CLASS("java/lang/Double", JNIUtil::Double_class)
  REQUIRE_METHOD(JNIUtil::Double_class, "<init>", "(D)V", JNIUtil::m_Double_init_double)

  JNIUtil::needsToCacheClassData = false;
}

} // namespace jv8

#endif // JV8_H_