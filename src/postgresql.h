#include <pgsql/libpq-fe.h>

typedef struct dbi_Postgresql_Data {
	PGconn *conn;
	PGresult *res;
} dbi_Postgresql_Data;
