
class spermission
{
public:
		spermission();
		spermission(const char *p_cpt, const char *p_inst);
		~spermission();

		//permissions allocated to principals... in future for keys or whatever
		const char *principal_cpt;
		const char *principal_inst;
		virtual int identical(spermission *p);
		virtual int authorised(const char *p_cpt, const char *p_inst);



};


class rdcpermission : public spermission
{
public:
		rdcpermission();
		virtual ~rdcpermission();

		char *target_ept;
		int identical(rdcpermission *p);

};

class spermissionvector : public pvector // wrapper class
{
	public:

		void add(spermission *x) { pvector::add((void *)x); }
		void set(int n, spermission *x) { pvector::set(n, (void *)x); }
		spermission *item(int n) { return (spermission *)pvector::item(n); }
		spermission *top() { return (spermission *)pvector::top(); }
		spermission *pop() { return (spermission *)pvector::pop(); }
		int find(spermission *x);
		void remove(spermission *x) { pvector::remove((void *)x); }
		void print_permissions();
		int check_authorised(const char *p_cpt, const char *p_inst);


		//Note only deletes an ACL rule - i.e. an exactly specified permissions. For instance, a delete *:* won't delete all rules, but only the identical rule - i.e. the one that is specified as *:*
		//likewise client:* will only delete a rule that applies to all clients, not rules like client:instance
		//this is deliberate, as the idea is to delete specific rules...
		void delete_permission(spermission *perm);
};


class rdcpermissionstore{
public:
		rdcpermissionstore();
		~rdcpermissionstore();

	    //accessors to the ACL
	    void add_permission(rdcpermission *perm);
	    void remove_permission(rdcpermission *perm);

	    //the permissions check
	    int authorised(const char *endpt, const char *principal_name, const char *principal_inst);

	    //returns whther there is ANY rule whihc authorised the principal (i.e. any endpoint)
	    //NB it ignores privileges on builtin endpoints...e.g. almost every endpoint allows lookup_schema, just hte data returned varies..
	    int any_authorising(const char *principal_name, const char *principal_inst, svector *builtins);

	    spermissionvector *acpolicies;

};



//a class for the sbus wrapper to deal with privilege changes inside the wrapper (maybe this should be in wrapper.h.. but anyway)
class privilegeparams{
public:

privilegeparams();
privilegeparams(const char *targ_ept, const char *principal_cpt, const char *principal_inst, int adding_permission);
~privilegeparams();

//snode *query_sn;
const char *target_endpt;
const char *p_cpt;
const char *p_inst;
int addpriv;

};

class privilegeparamsvector : public pvector // wrapper class
{
	public:

		void add(privilegeparams *x) { pvector::add((void *)x); }
		void set(int n, privilegeparams *x) { pvector::set(n, (void *)x); }
		privilegeparams *item(int n) { return (privilegeparams *)pvector::item(n); }
		privilegeparams *top() { return (privilegeparams *)pvector::top(); }
		privilegeparams *pop() { return (privilegeparams *)pvector::pop(); }
		void remove(privilegeparams *x) { pvector::remove((void *)x); }
};
