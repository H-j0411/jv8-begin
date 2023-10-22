#ifndef JNIUTIL_H_
#define JNIUTIL_H_

#include <sys/types.h>
#include <jni.h>

namespace jv8 {

/**
 * Utility class that stores all the cached field/method/class
 * to share pointers accross files.
 */
class JNIUtil {
public:
  static jfieldID f_V8Runner_handle,
                  f_Function_handle,
                  f_Function_runnerHandle,
                  sf_Undefined_instance;

  static jmethodID m_Function_init,
                   m_V8MappableMethod_runMethod,
                   m_V8Exception_init_str_str_int_str;

  static jclass V8Runner_class,
                V8Function_class,
                Function_class,
                Undefined_class,
                V8Exception_class,
                V8MappableMethod_class;

  static jmethodID m_String_init_str,
                   m_Boolean_init_bool,
                   m_Double_init_double,
                   m_Boolean_booleanValue,
                   m_Number_doubleValue;

  static jclass Object_class,
                String_class,
                Number_class,
                Double_class,
                Boolean_class;

  static bool needsToCacheClassData;
  static JavaVM* javaVM;
  
};

}

#endif