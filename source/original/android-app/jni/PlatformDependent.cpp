/*
 * PlatformDependent.cpp
 *
 *  Created on: Aug 13, 2014
 *      Author: elad
 */

#include "PlatformDependent.h"
#include <iostream>

using std::stringstream;
void showString(stringstream& sstream) {
	__android_log_write(ANDROID_LOG_INFO, "JNIDATA", sstream.str().c_str());
	sstream.str("");
}
