
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../library/error.h"
#include "../library/datatype.h"
#include "../library/hash.h"

#include "permission.h"


/*general permissions*/
spermission::spermission()
{
	principal_cpt= strdup("");
	principal_inst=strdup("");
}

spermission::spermission(const char *p_cpt, const char *p_inst)
{
	if (p_cpt==NULL) p_cpt= strdup(""); else 	principal_cpt = strdup(p_cpt);
	if (p_inst==NULL) p_inst = strdup(""); else	principal_inst = strdup(p_inst);
}


spermission::~spermission()
{
	delete[] principal_cpt;
	delete[] principal_inst;
}


int spermission::identical(spermission *p){
	//printf("in identical passedin %s:%s  stored %s:%s\n",p->principal_cpt,p->principal_inst,principal_cpt,principal_inst);
	return (!strcmp(p->principal_cpt, principal_cpt) && !strcmp(p->principal_inst, principal_inst));
}

int spermission::authorised(const char *p_cpt, const char *p_inst){

	if (!strcmp(principal_cpt,"") || !strcmp(principal_cpt, p_cpt)) //applies to the cpt?
			if (!strcmp(principal_inst,"") || !strcmp(principal_inst,p_inst)) //applies to the inst?
					return true; //if so - permissions granted
	return false;
}


/* RDC Permissions---- as they hold permissions about the target... */
rdcpermission::rdcpermission() : spermission::spermission()
{
	target_ept =strdup("");
}


rdcpermission::~rdcpermission()
{
	if(target_ept != NULL) delete[] target_ept;
}

int rdcpermission::identical(rdcpermission *p){
	return (spermission::identical(p) && !strcmp(p->target_ept, target_ept) );
}

rdcpermissionstore::rdcpermissionstore() {
	acpolicies = new spermissionvector();
}

rdcpermissionstore::~rdcpermissionstore(){
//	TODO: MEMORY LEAK -- delete everything in the vectors for each map...
}


int rdcpermissionstore::any_authorising(const char *principal_name, const char *principal_inst, svector *builtins){

	int auth = false;

	rdcpermission *p;
	for (int i = 0; i < acpolicies->count(); i++)
	{
		p = (rdcpermission *)acpolicies->item(i);

		if (builtins->find(p->target_ept) == -1)
		{
			auth = p->authorised(principal_name, principal_inst);
			if (auth)
				break;

		}
	}
	printf("\t%s:%s  --- %s Authorised. Tested %d policies.\n",principal_name, principal_inst,  auth?"":"NOT", acpolicies->count(), builtins->count());
	//print something here
	return auth;
}

int rdcpermissionstore::authorised(const char *endpt, const char *principal_name, const char *principal_inst){

	int auth = false;

	rdcpermission *p;
	for (int i = 0; i < acpolicies->count(); i++)
	{
		p = (rdcpermission *)acpolicies->item(i);
		if (!strcmp("", endpt) || //this is see whether the principal has the possibility to access 'any' endpoint for the image..
				!strcmp("", p->target_ept) || !strcmp(endpt,p->target_ept)) //otherwise, check for a specific endpoint
		{
			auth = p->authorised(principal_name, principal_inst);
			if (auth)
				break;
		}
	}
	printf("\t%s:%s on ednpt %s --- %s Authorised. Tested %d policies.\n",principal_name, principal_inst, endpt, auth?"":"NOT", acpolicies->count());
	//print something here
	return auth;
}


//adds a permission
void rdcpermissionstore::add_permission(rdcpermission *perm){

	//TODO: check for duplicates..
	acpolicies->add(perm);
	//printf("\tAdded permission: pcp=%s pci=%s tept=%s\n",perm->principal_cpt,perm->principal_inst,perm->target_ept);
}

void rdcpermissionstore::remove_permission(rdcpermission *perm){
	int deleted = false;
	for (int i = 0; i < acpolicies->count(); i++)
		if ( ((rdcpermission *) acpolicies->item(i))->identical(perm)  )
		{
			acpolicies->delete_permission(perm);
			//printf("\tRemoved Permission: pcp=%s pci=%s tept=%s\n",perm->principal_cpt,perm->principal_inst,perm->target_ept);
			deleted = true;
			//break; //what about duplicates?
		}
	if (!deleted)
		printf("\tCould not remove permission: pcp=%s pci=%s tept=%s ---- no matching rule found\n",perm->principal_cpt,perm->principal_inst,perm->target_ept);
	delete 	perm;
}

int spermissionvector::find(spermission *x)
{
	for(int i = 0; i < pvector::count(); i++)
		if (item(i)->identical(x))
			return i;
	return -1;
}

void spermissionvector::print_permissions(){
	for (int i = 0; i < pvector::count(); i++)
		printf("Permission: %s:%s\n",item(i)->principal_cpt,item(i)->principal_inst);
}

//Note only deletes an ACL rule - i.e. an exactly specified permissions. For instance, a delete *:* won't delete all rules, but only a rule that is specified as *:*
//likewise client:* will only delete a rule that applies to all clients, not rules like client:instance
//this is deliberate, as the idea is to delete specific rules...
void spermissionvector::delete_permission(spermission *perm){

	int i = 0;
	while (i < pvector::count())
	{
		if (item(i)->identical(perm))
				del(i);
		else
			i++;
	}

}

int spermissionvector::check_authorised(const char *p_cpt, const char *p_inst){
	int ret = false;
	//printf("trying to authorise %s:%s count %d\n",p_cpt, p_inst,pvector::count());
	//verbose - for prints...
	for (int i = 0; i < pvector::count(); i++)
	{
		//printf("tested %s:%s\n",item(i)->principal_cpt,item(i)->principal_inst);
		ret = item(i)->authorised(p_cpt, p_inst);
		if (ret)
				break;
	}
	log("--- %s Authorised for access\n",ret?"":"NOT");
	return ret;
}


privilegeparams::privilegeparams()
{
	target_endpt=strdup("");
	p_cpt=strdup("");
	p_inst=strdup("");
	addpriv = false;
}

privilegeparams::privilegeparams(const char *targ_ept, const char *principal_cpt, const char *principal_inst, int adding_permission)
{

	if (targ_ept==NULL) target_endpt=strdup(""); else target_endpt=strdup(targ_ept);
	if (principal_cpt==NULL) p_cpt = strdup(""); else p_cpt=strdup(principal_cpt);
	if (principal_inst==NULL) p_inst = strdup(""); else p_inst=strdup(principal_inst);
	addpriv = adding_permission;

}
privilegeparams::~privilegeparams()
{
	if(target_endpt != NULL) delete target_endpt;
	if(p_cpt != NULL) delete p_cpt;
	if(p_inst != NULL) delete p_inst;
}
