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
 
extern "C" {

scomponent *com; //the component
sendpoint *sender; //endpoints
snode *parent; //for building/extracting message attributes
const char *msg_type;

void
Java_uk_ac_cam_tcs40_sbus_SComponent_scomponent( JNIEnv* env,
                                                  jobject thiz, jstring compType, jstring compName )
{
	const char *componentType = env->GetStringUTFChars(compType, 0);
	const char *componentName = env->GetStringUTFChars(compName, 0);

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

void
Java_uk_ac_cam_tcs40_sbus_SComponent_createMessage( JNIEnv* env,
		                                              jobject thiz,
		                                              jstring name)
{
	msg_type = env->GetStringUTFChars(name, 0);
	
	// create parent snode
	parent = mklist(msg_type);
	
	env->ReleaseStringUTFChars(name, 0);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_packInt( JNIEnv* env,
                                                  jobject thiz,
                                                  jobject java_snode)
{	
	snode *sn;
	
	jclass clazz = env->GetObjectClass(java_snode);
	
	jmethodID getNodeName = env->GetMethodID(clazz, "getNodeName", "()Ljava/lang/String;");
	jobject result = env->CallObjectMethod(java_snode, getNodeName);
	const char* name = env->GetStringUTFChars((jstring) result, 0);
	
	jmethodID getValue = env->GetMethodID(clazz, "getIntValue", "()I");
	jint i = env->CallIntMethod(java_snode, getValue);	
	sn = pack(i, name);

	env->ReleaseStringUTFChars(0, name);

	parent->append(sn);	
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_packString( JNIEnv* env,
                                                  jobject thiz,
                                                  jobject java_snode)
{
	snode *sn;
	
	jclass clazz = env->GetObjectClass(java_snode);
	
	jmethodID getNodeName = env->GetMethodID(clazz, "getNodeName", "()Ljava/lang/String;");
	jobject result = env->CallObjectMethod(java_snode, getNodeName);
	const char* name = env->GetStringUTFChars((jstring) result, 0);
	
	jmethodID getValue = env->GetMethodID(clazz, "getStringValue", "()Ljava/lang/String;");
	jobject str = env->CallObjectMethod(java_snode, getValue);	
	const char* string;
	string = env->GetStringUTFChars((jstring) str, 0);
	sn = pack(string, name);
			
	env->ReleaseStringUTFChars(0, string);
	env->ReleaseStringUTFChars(0, name);

	parent->append(sn);	
}


jstring
Java_uk_ac_cam_tcs40_sbus_SComponent_emit( JNIEnv* env,
                                                  jobject thiz)
{
	sender->emit(parent);
	
	const char *xml = parent->toxml(1);
	
	parent = mklist(msg_type);
		
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
