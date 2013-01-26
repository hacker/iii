#ifndef __SECLUDE_H
#define __SECLUDE_H

#include <sqlite3.h>
#include <stdexcept>
#include <tr1/memory>
#include <cstdarg>

namespace seclude {
    using std::tr1::shared_ptr;

    static struct sql_null_type { } sql_null;


    struct sqlite3_error : public std::runtime_error {
	int code, ecode;
	sqlite3_error(sqlite3 *s,const std::string& m)
	    : std::runtime_error(m+": "+(s?sqlite3_errmsg(s):"unknown error")),
	      code(s?sqlite3_errcode(s):-1), ecode(s?sqlite3_extended_errcode(s):-1) { }
	sqlite3_error(sqlite3_stmt *s,const std::string& m)
	    : std::runtime_error(m+": "+(s?sqlite3_errmsg(sqlite3_db_handle(s)):"unknown error")),
	      code(s?sqlite3_errcode(sqlite3_db_handle(s)):-1),
	      ecode(s?sqlite3_extended_errcode(sqlite3_db_handle(s)):-1) { }
    };


    static void sqlite3_stmt_deleter(sqlite3_stmt* s) { s && sqlite3_finalize(s); }
    struct stmt_t : public shared_ptr<sqlite3_stmt> {
	stmt_t(sqlite3_stmt *s) : shared_ptr<sqlite3_stmt>(s,sqlite3_stmt_deleter) { }

	bool step() {
	    switch(sqlite3_step(get())) {
		case SQLITE_DONE: return false;
		case SQLITE_ROW: return true;
	    }
	    panic("failed to sqlite3_step()");
	}
	stmt_t& reset() {
	    return worry(sqlite3_reset(get()),"failed to sqlite3_reset()");
	}
	stmt_t& clear_bindings() {
	    return worry(sqlite3_clear_bindings(get()),"failed to sqlite3_clear_bindings()");
	}
	int data_count() { return sqlite3_data_count(get()); }
	int column_count() { return sqlite3_column_count(get()); }
	template<typename T> T column(int i);
	std::string column_name(int i) { return sqlite3_column_name(get(),i); }
	int column_type(int i) { return sqlite3_column_type(get(),i); }
	bool is_column_null(int i) { return column_type(i)==SQLITE_NULL; }

	template<typename T> int bind_(int i,T v);
	template<typename T>
	stmt_t& bind(int i,T v) {
	    return worry(bind_(i,v),"failed to perform binding");
	}
	template<typename T>
	stmt_t& bind(const char *n,T v) { return bind(bind_parameter_index(n),v); }
	int bind_parameter_index(const char *n) {
	    int rv = sqlite3_bind_parameter_index(get(),n);
	    if(!rv) throw std::invalid_argument("failed to sqlite3_bind_parameter_index()");
	    return rv;
	}


	stmt_t& worry(int c,const char *m) { if(c!=SQLITE_OK) panic(m); return *this;}
	stmt_t& panic(const std::string& m) { throw sqlite3_error(get(),m); return *this;}
    };

    template<> inline int stmt_t::bind_<int>(int i,int v) {
	return sqlite3_bind_int(get(),i,v); }
    template<> inline int stmt_t::bind_<long>(int i,long v) {
	return sqlite3_bind_int64(get(),i,v); }
    template<> inline int stmt_t::bind_<sqlite3_int64>(int i,sqlite3_int64 v) {
	return sqlite3_bind_int64(get(),i,v); }
    template<> inline int stmt_t::bind_<double>(int i,double v) {
	return sqlite3_bind_double(get(),i,v); }
    template<> inline int stmt_t::bind_<const std::string&>(int i,const std::string& v) {
	return sqlite3_bind_text(get(),i,v.c_str(),-1,SQLITE_TRANSIENT); }
    template<> inline int stmt_t::bind_<std::string>(int i,std::string v) {
	return sqlite3_bind_text(get(),i,v.c_str(),-1,SQLITE_TRANSIENT); }
    template<> inline int stmt_t::bind_<const char *>(int i,const char *v) {
	return sqlite3_bind_text(get(),i,v,-1,SQLITE_TRANSIENT); }
    template<> inline int stmt_t::bind_<sql_null_type>(int i,sql_null_type) {
	return sqlite3_bind_null(get(),i); }

    template<> int stmt_t::column<int>(int i) { return sqlite3_column_int(get(),i); }
    template<> long stmt_t::column<long>(int i) { return sqlite3_column_int64(get(),i); }
    template<> sqlite3_int64 stmt_t::column<sqlite3_int64>(int i) { return sqlite3_column_int64(get(),i); }
    template<> double stmt_t::column<double>(int i) { return sqlite3_column_double(get(),i); }
    template<> std::string stmt_t::column<std::string>(int i) { return (const char*)sqlite3_column_text(get(),i); }


    static void sqlite3_deleter(sqlite3* s) { s && sqlite3_close(s); }
    struct db_t : public shared_ptr<sqlite3> {
	db_t(const char *f) {
	    sqlite3 *s=0;
	    int c = sqlite3_open(f,&s);
	    reset(s,sqlite3_deleter);
	    worry(c,"failed to sqlite3_open()");
	}

	void exec(const char *s) {
	    char *e = 0;
	    int c = sqlite3_exec(get(),s,NULL,NULL,&e);
	    if(e) sqlite3_free(e);
	    worry(c,"failed to sqlite3_exec()");
	}

	stmt_t prepare(const char *s) {
	    sqlite3_stmt *r=0;
	    int c = sqlite3_prepare(get(),s,-1,&r,0);
	    stmt_t rv(r);
	    worry(c,"failed to sqlite3_prepare()");
	    return rv;
	}

	int changes() { return sqlite3_changes(get()); }

	void worry(int c,const char *m) { if(c!=SQLITE_OK) throw sqlite3_error(get(),m); }
    };

}

#endif /* __SECLUDE_H */
