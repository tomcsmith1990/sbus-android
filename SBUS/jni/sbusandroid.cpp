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

void
Java_uk_ac_cam_tcs40_sbus_SComponent_scomponent( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring compName, 
                                                  jstring instanName )
{
	const char *cpt_name = env->GetStringUTFChars(compName, 0);
	const char *instance_name = env->GetStringUTFChars(instanName, 0);
	
	com = new scomponent(cpt_name, instance_name);

	env->ReleaseStringUTFChars(compName, cpt_name);
	env->ReleaseStringUTFChars(instanName, instance_name);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpoint( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring endName, 
                                                  jstring endHash )
{
	const char *endpoint_name = env->GetStringUTFChars(endName, 0);
	const char *endpoint_hash = env->GetStringUTFChars(endHash, 0);
	
	//add an endpoint
	// -- corresponds to the endpoints defined in the schema file
	// --  name, source/sink, endpoint hash (obtained by running analysecpt <somesensor.cpt>)
	sender = com->add_endpoint(endpoint_name, EndpointSource, endpoint_hash);

	env->ReleaseStringUTFChars(endName, endpoint_name);
	env->ReleaseStringUTFChars(endHash, endpoint_hash);
}

jstring
Java_uk_ac_cam_tcs40_sbus_SComponent_endpointMap( JNIEnv* env,
				                                          jobject thiz, 
				                                          jstring addr)
{
	const char *address = env->GetStringUTFChars(addr, 0);
	
	const char *s = sender->map(address, NULL);
	
	env->ReleaseStringUTFChars(addr, address);
	
	if (s == NULL) return env->NewStringUTF("error"); 
	else return env->NewStringUTF(s);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_endpointUnmap( JNIEnv* env,
				                                          jobject thiz)
{
	sender->unmap();
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
	
	// start the SBUS wrapper
	// parameters: a) the component scehmafile, b) the port for the component, c) whether to (attempt to) register with the RDC
	com->start(cpt_filename, port, use_rdc);

	env->ReleaseStringUTFChars(cpt_file, cpt_filename);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_setPermission( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring componentName,
                                                  jstring instanceName,
                                                  jboolean allow )
{
	const char *component_name = env->GetStringUTFChars(componentName, 0);
	const char *instance_name = env->GetStringUTFChars(instanceName, 0);
	
	//authorise the client to connect (here based on their class name)
	com->set_permission(component_name, instance_name, allow);

	env->ReleaseStringUTFChars(componentName, component_name);
	env->ReleaseStringUTFChars(instanceName, instance_name);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_createMessage( JNIEnv* env,
		                                              jobject thiz,
		                                              jstring name)
{
	const char *msg_type = env->GetStringUTFChars(name, 0);
	
	// create parent snode
	parent = mklist(msg_type);
	
	env->ReleaseStringUTFChars(name, msg_type);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_packInt( JNIEnv* env,
                                                  jobject thiz,
                                                  jint i,
                                                  jstring n)
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	sn = pack(i, name);

	if (n != NULL) env->ReleaseStringUTFChars(n, name);

	parent->append(sn);	
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_packString( JNIEnv* env,
                                                  jobject thiz,
                                                  jstring s,
                                                  jstring n)
{
	snode *sn;
	
	const char* name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	const char* string = env->GetStringUTFChars(s, 0);

	sn = pack(string, name);
			
	env->ReleaseStringUTFChars(s, string);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);

	parent->append(sn);	
}


jstring
Java_uk_ac_cam_tcs40_sbus_SComponent_emit( JNIEnv* env,
                                                  jobject thiz)
{
	sender->emit(parent);
	
	const char *xml = parent->toxml(1);
	
	//parent = mklist(msg_type);
		
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
