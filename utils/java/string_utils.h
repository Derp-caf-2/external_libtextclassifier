/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_UTILS_JAVA_STRING_UTILS_H_
#define LIBTEXTCLASSIFIER_UTILS_JAVA_STRING_UTILS_H_

#include <jni.h>
#include <string>

namespace libtextclassifier3 {

bool JByteArrayToString(JNIEnv* env, const jbyteArray& array,
                        std::string* result);
bool JStringToUtf8String(JNIEnv* env, const jstring& jstr, std::string* result);

}  // namespace libtextclassifier3

#endif  // LIBTEXTCLASSIFIER_UTILS_JAVA_STRING_UTILS_H_
