// error.h - DMI - 17-2-2007

void error(const char *format, ...);
void log(const char *format, ...);
void warning(const char *format, ...);
void debug(const char *format, ...);

void sassert(int t, const char *msg);
char *sformat(const char *format, ...);
void init_logfile(const char *cpt_name, const char *instance_name,
		int wrapper);
char *get_sbus_dir();

extern const int LogErrors, LogWarnings, LogMessages, LogDebugging;
extern const int LogNothing, LogAll, LogDefault, EchoDefault;

extern int log_level;
extern int echo_level;

extern FILE *fp_log;

class SBUSException {};

class SchemaException : SBUSException
{
	public:
	SchemaException(const char *s, int l);
	const char *msg;
	int line;
};

class ProtocolException : public SBUSException
{ public: ProtocolException(const char *s); const char *msg; };

class SubscriptionException : SBUSException
{ public: SubscriptionException(const char *s); const char *msg; };

class ValidityException : SBUSException
{ public: ValidityException(const char *s); const char *msg; };

class ImportException : SBUSException
{ public: ImportException(const char *s); const char *msg; };

class MatchException : SBUSException {};
class DisconnectException : SBUSException {};
