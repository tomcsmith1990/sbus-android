#include "sbusandroid.h"

extern "C" {

// Declaration
long addEndpoint(JNIEnv* env, jobject thiz, jlong component, jstring endName, EndpointType type, jstring messageHash, jstring responseHash);


jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_scomponent( JNIEnv* env,
                                                  jobject thiz, 
                                                  jstring compName, 
                                                  jstring instanName )
{
	const char *cpt_name = env->GetStringUTFChars(compName, NULL);
	const char *instance_name = (instanName == NULL) ? cpt_name : env->GetStringUTFChars(instanName, NULL);
	
	scomponent *component;
	component = new scomponent(cpt_name, instance_name);

	env->ReleaseStringUTFChars(compName, cpt_name);
	if (instanName != NULL) env->ReleaseStringUTFChars(instanName, instance_name);
	
	return (long)component;
}

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpointSource( JNIEnv* env,
                                                  jobject thiz, 
                                                  jlong component,
                                                  jstring endName, 
                                                  jstring messageHash,
                                                  jstring responseHash )
{
	return addEndpoint(env, thiz, component, endName, EndpointSource, messageHash, responseHash);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpointSink( JNIEnv* env,
                                                  jobject thiz, 
                                                  jlong component,
                                                  jstring endName, 
                                                  jstring messageHash,
                                                  jstring responseHash )
{
	return addEndpoint(env, thiz, component, endName, EndpointSink, messageHash, responseHash);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpointClient( JNIEnv* env,
                                                  jobject thiz, 
                                                  jlong component,
                                                  jstring endName, 
                                                  jstring messageHash,
                                                  jstring responseHash )
{
	return addEndpoint(env, thiz, component, endName, EndpointClient, messageHash, responseHash);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_addEndpointServer( JNIEnv* env,
                                                  jobject thiz, 
                                                  jlong component,
                                                  jstring endName, 
                                                  jstring messageHash,
                                                  jstring responseHash )
{
	return addEndpoint(env, thiz, component, endName, EndpointServer, messageHash, responseHash);
}

long addEndpoint( JNIEnv* env,
                  jobject thiz, 
                  jlong component,
                  jstring endName,
                  EndpointType type,
                  jstring messageHash,
                  jstring responseHash )
{
	const char *endpoint_name = env->GetStringUTFChars(endName, NULL);
	const char *message_hash = env->GetStringUTFChars(messageHash, NULL);
	const char *response_hash = (responseHash == NULL) ? NULL : env->GetStringUTFChars(responseHash, NULL) ;
	
	sendpoint *endpoint;
	
	endpoint = ((scomponent *)component)->add_endpoint(endpoint_name, type, message_hash, response_hash);

	env->ReleaseStringUTFChars(endName, endpoint_name);
	env->ReleaseStringUTFChars(messageHash, message_hash);
	if (responseHash != NULL) env->ReleaseStringUTFChars(responseHash, response_hash);
	
	return (long)endpoint;
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_addRDC( JNIEnv* env,
                                             jobject thiz,
                                             jlong component, 
                                             jstring addr )
{
	const char *rdc = env->GetStringUTFChars(addr, NULL);
	
	((scomponent *)component)->add_rdc(rdc);

	env->ReleaseStringUTFChars(addr, rdc);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_removeRDC( JNIEnv* env,
		                                         jobject thiz,
		                                         jlong component, 
		                                         jstring addr )
{
	const char *rdc = env->GetStringUTFChars(addr, NULL);
	
	((scomponent *)component)->remove_rdc(rdc);

	env->ReleaseStringUTFChars(addr, rdc);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_setRDCUpdateNotify( JNIEnv* env,
						                                    jobject thiz, 
						                                    jlong component,
						                                    jboolean notify )
{
	((scomponent *)component)->set_rdc_update_notify(notify ? JNI_TRUE : JNI_FALSE);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_setRDCUpdateAutoconnect( JNIEnv* env,
								                                jobject thiz, 
								                                jlong component,
								                                jboolean autoconnect )
{
	((scomponent *)component)->set_rdc_update_autoconnect(autoconnect ? JNI_TRUE : JNI_FALSE);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_start( JNIEnv* env,
                                            jobject thiz, 
                                            jlong component,
                                            jstring cpt_file,
                                            jint port,
                                            jboolean use_rdc )
{
	const char *cpt_filename = env->GetStringUTFChars(cpt_file, NULL);
	
	((scomponent *)component)->start(cpt_filename, port, use_rdc ? JNI_TRUE : JNI_FALSE);

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
	const char *component_name = env->GetStringUTFChars(componentName, NULL);
	const char *instance_name = env->GetStringUTFChars(instanceName, NULL);
	
	((scomponent *)component)->set_permission(component_name, instance_name, allow ? JNI_TRUE : JNI_FALSE);

	env->ReleaseStringUTFChars(componentName, component_name);
	env->ReleaseStringUTFChars(instanceName, instance_name);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SComponent_RDCUpdateNotificationsEndpoint( 	JNIEnv* env,
												                        jobject thiz, 
												                        jlong component )
{
	return (long)((scomponent *)component)->rdc_update_notifications_endpoint();
}

jstring
Java_uk_ac_cam_tcs40_sbus_SComponent_declareSchema( JNIEnv* env,
		                                            jobject thiz, 
		                                            jlong component,
		                                            jstring javaSchema )
{
	const char *schema = env->GetStringUTFChars(javaSchema, NULL);
	
	const char *hash = ((scomponent *)component)->declare_schema(schema)->tostring();

	env->ReleaseStringUTFChars(javaSchema, schema);
		
	return env->NewStringUTF(hash);
}

void
Java_uk_ac_cam_tcs40_sbus_SComponent_delete( JNIEnv* env,
                                             jobject thiz,
                                             jlong component )
{
	// Also deletes endpoints.
	delete ((scomponent *)component);
}

}
