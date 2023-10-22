#include <jni.h>

#include "JNIUtil.h"

namespace jv8 {

// Fields
jfieldID JNIUtil::f_V8Runner_handle = NULL;
jfieldID JNIUtil::f_Function_handle = NULL;
jfieldID JNIUtil::f_Function_runnerHandle = NULL;

// Static fields
jfieldID JNIUtil::sf_Undefined_instance = NULL;

// jv8 Methods
jmethodID JNIUtil::m_Function_init = NULL;
jmethodID JNIUtil::m_V8MappableMethod_runMethod = NULL;
jmethodID JNIUtil::m_V8Exception_init_str_str_int_str = NULL;

// jv8 Classes
jclass JNIUtil::V8Runner_class = NULL;
jclass JNIUtil::Function_class = NULL;
jclass JNIUtil::Undefined_class = NULL;
jclass JNIUtil::V8Exception_class = NULL;
jclass JNIUtil::V8MappableMethod_class = NULL;

// Native methods
jmethodID JNIUtil::m_String_init_str = NULL;
jmethodID JNIUtil::m_Boolean_init_bool = NULL;
jmethodID JNIUtil::m_Double_init_double = NULL;
jmethodID JNIUtil::m_Number_doubleValue = NULL;
jmethodID JNIUtil::m_Boolean_booleanValue = NULL;

// Native classes
jclass JNIUtil::Object_class = NULL;
jclass JNIUtil::String_class = NULL;
jclass JNIUtil::Boolean_class = NULL;
jclass JNIUtil::Number_class = NULL;
jclass JNIUtil::Double_class = NULL;

// Misc
bool JNIUtil::needsToCacheClassData = true;
JavaVM* JNIUtil::javaVM = NULL;

}