/*
 * Copyright (C) 2009 The Android Open Source Project
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
 *
 */

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sstream>
#include <sys/select.h>
#include <jni.h>
#include <sbus.h>

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 */
 
extern "C" {

scomponent *com; //the component
sendpoint *sender; //endpoints
snode *sn, *sn2, *sn3, *parent; //for building/extracting message attributes	

void
Java_uk_ac_cam_tcs40_sbus_SComponent_scomponent( JNIEnv* env,
                                                  jobject thiz, jstring compType, jstring compName )
{
	const char *componentType = env->GetStringUTFChars(compType, 0);
	const char *componentName = env->GetStringUTFChars(compName, 0);
	printf("%s\n", componentName);
	com = new scomponent(componentType, componentName);
	
	//sets the logging / output levels
	scomponent::set_log_level(LogErrors | LogWarnings | LogMessages, LogErrors | LogWarnings);

	env->ReleaseStringUTFChars(compType, componentType);
	env->ReleaseStringUTFChars(compName, componentName);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpoint( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring endName, 
                                                  jstring endHash )
{
	const char *endpointName = env->GetStringUTFChars(endName, 0);
	const char *endpointHash = env->GetStringUTFChars(endHash, 0);
	
	//add an endpoint
	// -- corresponds to the endpoints defined in the schema file
	// --  name, source/sink, endpoint hash (obtained by running analysecpt <somesensor.cpt>)
	sender = com->add_endpoint(endpointName, EndpointSource, endpointHash);

	env->ReleaseStringUTFChars(endName, endpointName);
	env->ReleaseStringUTFChars(endHash, endpointHash);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_addRDC( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring addr )
{
	const char *rdc = env->GetStringUTFChars(addr, 0);
	
	//if the RDC is NOT local - you need to tell SBUS where it is
	com->add_rdc(rdc);

	env->ReleaseStringUTFChars(addr, rdc);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_start( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring cpt_file,
                                                  jint port,
                                                  jboolean use_rdc )
{
	const char *cpt_filename = env->GetStringUTFChars(cpt_file, 0);
	
	//start the SBUS wrapper
	// parameters: a) the component scehmafile, b) the port for the component, c) whether to (attempt to) register with the RDC
	com->start(cpt_filename, port, use_rdc);

	env->ReleaseStringUTFChars(cpt_file, cpt_filename);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_setPermission( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring cptName,
                                                  jstring s,
                                                  jboolean allow )
{
	const char *cpt_name = env->GetStringUTFChars(cptName, 0);
	const char *something = env->GetStringUTFChars(s, 0);
	
	//authorise the client to connect (here based on their class name)
	com->set_permission(cpt_name, something, allow);

	env->ReleaseStringUTFChars(cptName, cpt_name);
	env->ReleaseStringUTFChars(s, something);
}


jstring
Java_uk_ac_cam_tcs40_sbus_SComponent_emit( JNIEnv* env,
                                                  jobject thiz,
                                                  jstring msg,
                                                  jint val1,
                                                  jint val2 )
{
	const char *message = env->GetStringUTFChars(msg, 0);
	
	//pack the string
	sn2 = pack(message);

	sn = pack(val1,"someval"); //can specify the attribute name, optionally...

	sn3 = pack(val2, "somevar");
	
	parent = pack(sn2, sn, sn3, "reading"); //build the msg (corresponding to the schema)

	sender->emit(parent);
	
	const char *xml = parent->toxml(1);
	
	delete parent;
	
	env->ReleaseStringUTFChars(msg, message);
	
	return env->NewStringUTF(xml);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_delete( JNIEnv* env,
                                                  jobject thiz
                                                  )
{
	sender->unmap(); //nicer disconnect (warning without this)
	delete com; //component cleanup happens implicitly
}

}

