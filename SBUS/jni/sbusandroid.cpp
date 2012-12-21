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

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpoint( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring endName, 
                                                  jstring endHash )
{
	const char *endpoint_name = env->GetStringUTFChars(endName, 0);
	const char *endpoint_hash = env->GetStringUTFChars(endHash, 0);
	
	sendpoint *endpoint;
	endpoint = com->add_endpoint(endpoint_name, EndpointSource, endpoint_hash);

	env->ReleaseStringUTFChars(endName, endpoint_name);
	env->ReleaseStringUTFChars(endHash, endpoint_hash);
	
	return (long)endpoint;
}

jstring
Java_uk_ac_cam_tcs40_sbus_SEndpoint_endpointMap( JNIEnv* env,
				                                 jobject thiz, 
			                                     jlong endpoint,
			                                     jstring addr )
{
	const char *address = env->GetStringUTFChars(addr, 0);
	
	const char *s = ((sendpoint *)endpoint)->map(address, NULL);
	
	env->ReleaseStringUTFChars(addr, address);
	
	if (s == NULL) return env->NewStringUTF("error"); 
	else return env->NewStringUTF(s);
}

void
Java_uk_ac_cam_tcs40_sbus_SEndpoint_endpointUnmap( JNIEnv* env,
				                                   jobject thiz,
			                                       jlong endpoint )
{
	((sendpoint *)endpoint)->unmap();
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_addRDC( JNIEnv* env,
                                             jobject thiz, 
                                             jstring addr )
{
	const char *rdc = env->GetStringUTFChars(addr, 0);
	
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
	
	com->set_permission(component_name, instance_name, allow);

	env->ReleaseStringUTFChars(componentName, component_name);
	env->ReleaseStringUTFChars(instanceName, instance_name);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SEndpoint_createMessage( JNIEnv* env,
	                                               jobject thiz,
	                                               jlong endpoint,
	                                               jstring name )
{
	const char *msg_type = env->GetStringUTFChars(name, 0);
	
	snode *message;
	message = mklist(msg_type);
	
	env->ReleaseStringUTFChars(name, msg_type);
	
	return (long)message;
}

void
Java_uk_ac_cam_tcs40_sbus_SEndpoint_packInt( JNIEnv* env,
                                             jobject thiz,
                                             jlong message,
                                             jint i,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	sn = pack(i, name);
	((snode *)message)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SEndpoint_packString( JNIEnv* env,
                                                jobject thiz,
                                                jlong message,
                                                jstring s,
                                                jstring n )
{
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	const char *string = env->GetStringUTFChars(s, 0);

	sn = pack(string, name);
	((snode *)message)->append(sn);
	
	env->ReleaseStringUTFChars(s, string);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SEndpoint_packClock( JNIEnv* env,
                                               jobject thiz,
                                               jlong message,
                                               jstring c,
                                               jstring n )
{
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	const char *clk = env->GetStringUTFChars(c, 0);

	sn = pack(clk, name);
			
	((snode *)message)->append(sn);	
	
	env->ReleaseStringUTFChars(c, clk);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}


jstring
Java_uk_ac_cam_tcs40_sbus_SEndpoint_emit( JNIEnv* env,
                                          jobject thiz,
                                          jlong endpoint,
                                          jlong message )
{
	((sendpoint *)endpoint)->emit(((snode *)message));
	
	const char *xml = ((snode *)message)->toxml(1);
			
	return env->NewStringUTF(xml);
}


void
Java_uk_ac_cam_tcs40_sbus_SComponent_delete( JNIEnv* env,
                                             jobject thiz )
{
	delete com;
}

}
