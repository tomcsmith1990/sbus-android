#include "sbusandroid.h"

extern "C" {

jint
Java_uk_ac_cam_tcs40_sbus_SNode_count( JNIEnv* env,
		                                 jobject thiz,
		                                 jlong node)
{	
	return ((snode *)node)->count();
}

jint
Java_uk_ac_cam_tcs40_sbus_SNode_exists( JNIEnv* env,
		                                 jobject thiz,
		                                 jlong node,
		                                 jstring n)
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	int exists = ((snode *)node)->exists(name);
	
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return exists;
}

jlong
Java_uk_ac_cam_tcs40_sbus_SNode_find( JNIEnv* env,
		                                 jobject thiz,
		                                 jlong node,
		                                 jstring n)
{	
	if (n == NULL) return 0;
	
	const char *name = env->GetStringUTFChars(n, NULL);
	
	snode *sn = ((snode *)node)->find(name);
	
	env->ReleaseStringUTFChars(n, name);
	
	if (sn == NULL) return 0;
	else return (long)sn;
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packBoolean( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jboolean b,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	sn = pack_bool(b ? JNI_TRUE : JNI_FALSE, name);
		
	((snode *)node)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packInt( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jint i,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	sn = pack(i, name);
	((snode *)node)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packDouble( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jdouble d,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	sn = pack(d, name);
	((snode *)node)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packString( JNIEnv* env,
                                                jobject thiz,
                                                jlong node,
                                                jstring s,
                                                jstring n )
{
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	const char *string = (s == NULL) ? NULL : env->GetStringUTFChars(s, NULL);

	sn = pack(string, name);
	((snode *)node)->append(sn);
	
	if (s != NULL) env->ReleaseStringUTFChars(s, string);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packClock( JNIEnv* env,
                                               jobject thiz,
                                               jlong node,
                                               jstring c,
                                               jstring n )
{
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	const char *clk = env->GetStringUTFChars(c, NULL);

	sdatetime datetime(clk);
	
	sn = pack(&datetime, name);
			
	((snode *)node)->append(sn);	
	
	env->ReleaseStringUTFChars(c, clk);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SNode_extractItem( JNIEnv* env,
			                                     jobject thiz,
			                                     jlong node,
			                                     jint item)
{	
	return (long)((snode *)node)->extract_item(item);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SNode_extractItemByName( JNIEnv* env,
			                                     jobject thiz,
			                                     jlong node,
			                                     jstring item)
{	
	snode *sn;
	
	const char *item_name = (item == NULL) ? NULL : env->GetStringUTFChars(item, NULL);
	
	sn =((snode *)node)->extract_item(item_name);
	
	if (item != NULL) env->ReleaseStringUTFChars(item, item_name);
	
	return (long)sn;
}

jint
Java_uk_ac_cam_tcs40_sbus_SNode_extractBoolean( JNIEnv* env,
			                                     jobject thiz,
			                                     jlong node,
			                                     jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	int i = ((snode *)node)->extract_flg(name);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return i;
}

jint
Java_uk_ac_cam_tcs40_sbus_SNode_extractInt( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	int i = ((snode *)node)->extract_int(name);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return i;
}

jdouble
Java_uk_ac_cam_tcs40_sbus_SNode_extractDouble( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	double d = ((snode *)node)->extract_dbl(name);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return d;
}

jstring
Java_uk_ac_cam_tcs40_sbus_SNode_extractString( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, NULL);
	
	const char *s = ((snode *)node)->extract_txt(name);
			
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return env->NewStringUTF(s);	
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_delete( JNIEnv* env,
                                             jobject thiz,
                                             jlong node )
{
	delete ((snode *)node);
}

}
