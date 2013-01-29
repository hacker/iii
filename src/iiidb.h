#ifndef __IIIDB_H
#define __IIIDB_H

#include <autosprintf.h>
#include "seclude.h"
#include "eyekinfig.h"

struct iiidb_t : public seclude::db_t {
    iiidb_t(eyekinfig_t& k) : seclude::db_t(gnu::autosprintf("%s/.iii.db",k.get_targetdir().c_str())) {
	try {
	    exec("SELECT 1 FROM photo LIMIT 0");
	}catch(const seclude::sqlite3_error& e) {
	    exec( "CREATE TABLE photo ("
		  " id integer PRIMARY KEY AUTOINCREMENT,"
		  " ctime integer NOT NULL,"
		  " mac text NOT NULL,"
		  " fileid integer NOT NULL,"
		  " filename text NOT NULL,"
		  " filesize integer NOT NULL,"
		  " filesignature text NOT NULL UNIQUE,"
		  " encryption text NOT NULL,"
		  " flags integer NOT NULL"
		  ")" );
	}
    }
};

#endif /* __IIIDB_H */
