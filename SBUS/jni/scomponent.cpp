#include "sbusandroid.h"
 
extern "C" {

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_scomponent( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring compName, 
                                                  jstring instanName )
{
	const char *cpt_name = env->GetStringUTFChars(compName, 0);
	const char *instance_name = env->GetStringUTFChars(instanName, 0);
	
	scomponent *component;
	component = new scomponent(cpt_name, instance_name);

	env->ReleaseStringUTFChars(compName, cpt_name);
	env->ReleaseStringUTFChars(instanName, instance_name);
	
	return (long)component;
}

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpoint( JNIEnv* env,
                                                  jobject thiz, 
                                                  jlong component,
                                                  jstring endName, 
                                                  jstring endHash )
{
	const char *endpoint_name = env->GetStringUTFChars(endName, 0);
	const char *endpoint_hash = env->GetStringUTFChars(endHash, 0);
	
	sendpoint *endpoint;
	endpoint = ((scomponent *)component)->add_endpoint(endpoint_name, EndpointSource, endpoint_hash);

	env->ReleaseStringUTFChars(endName, endpoint_name);
	env->ReleaseStringUTFChars(endHash, endpoint_hash);
	
	return (long)endpoint;
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_addRDC( JNIEnv* env,
                                             jobject thiz,
                                             jlong component, 
                                             jstring addr )
{
	const char *rdc = env->GetStringUTFChars(addr, 0);
	
	((scomponent *)component)->add_rdc(rdc);

	env->ReleaseStringUTFChars(addr, rdc);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_start( JNIEnv* env,
                                            jobject thiz, 
                                            jlong component,
                                            jstring cpt_file,
                                            jint port,
                                            jboolean use_rdc )
{
	const char *cpt_filename = env->GetStringUTFChars(cpt_file, 0);
	
	((scomponent *)component)->start(cpt_filename, port, use_rdc);

	env->ReleaseStringUTFChars(cpt_file, cpt_filename);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_setPermission( JNIEnv* env,
		                                            jobject thiz, 
		                                            jlong component,
		                                            jstring componentName,
		                                            jstring instanceName,
		                                            jboolean allow )
{
	const char *component_name = env->GetStringUTFChars(componentName, 0);
	const char *instance_name = env->GetStringUTFChars(instanceName, 0);
	
	((scomponent *)component)->set_permission(component_name, instance_name, allow);

	env->ReleaseStringUTFChars(componentName, component_name);
	env->ReleaseStringUTFChars(instanceName, instance_name);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_delete( JNIEnv* env,
                                             jobject thiz,
                                             jlong component )
{
	delete ((scomponent *)component);
}

}
