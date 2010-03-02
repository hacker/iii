/* vim:set sw=8 nosi noai cin cino=:0,l1,g0: */
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <list>
#include <string>
#include <iterator>

#include "config.h"

#define PHEADER \
	PACKAGE " Version " VERSION "\n" \
	"Copyright (c) 2009-2010 Klever Group"

typedef uint32_t fourcc_type;
enum fourcc_value {
	fourcc_RIFF = 0x46464952, fourcc_AVI  = 0x20495641, fourcc_LIST = 0x5453494c,
	fourcc_hdrl = 0x6c726468, fourcc_strl = 0x6c727473,
	fourcc_ncdt = 0x7464636e, fourcc_ncvr = 0x7276636e, fourcc_nctg = 0x6774636e, fourcc_ncth = 0x6874636e
};

fourcc_type str2fourcc(const char *str) {
	fourcc_type rv = 0;
	return *(fourcc_type*)strncpy((char*)&rv,str,sizeof(rv));
}
const std::string fourcc2str(fourcc_type fcc) {
	char rv[sizeof(fcc)+1];
	*(fourcc_type*)rv = fcc;
	rv[sizeof(fcc)]=0;
	return rv;
}

#pragma pack(1)
struct riff_sized_head {
	fourcc_type fourcc;
	uint32_t size;
};
#pragma pack()

class exceptional_success : public std::exception { };

struct riff {
	struct chunk_type {
		riff_sized_head head;
		uint32_t left;

		chunk_type(riff& r) {
			r.read(&head);
			left = head.size;
		}
	};

	std::istream *in;
	typedef std::list<chunk_type> chunks_type;
	chunks_type chunks;

	riff(std::istream& i) : in(&i) { }

	protected:

	int begin_chunk() {
		chunks.push_back( chunk_type(*this) );
		return chunks.size();
	}
	void end_chunk(int level) {
		assert(in); assert(chunks.size()==level);
		std::streamsize o = chunks.back().left;
		chunks.pop_back();
		if(!o) return;
		in->seekg(o,std::ios::cur);
		for(chunks_type::iterator i=chunks.begin(),ie=chunks.end() ;i!=ie; (i++)->left -= o) ;
	}

	void read(void *p,size_t n) {
		assert(in);
		if( (!chunks.empty()) && chunks.back().left < n)
			throw std::runtime_error("attempt to read beyond the end of chunk");
		if(!in->read((char*)p,n))
			throw std::runtime_error("failed to read()");
		std::streamsize gc = in->gcount();
		for(chunks_type::iterator i=chunks.begin(),ie=chunks.end();i!=ie;++i) {
			i->left -= gc; assert(i->left >= 0);
		}
		if(gc!=n) throw std::runtime_error("incomplete read()");
	}

	template<typename T> void read(T* v) { read(v,sizeof(*v)); }

	friend class scoped_chunk;

};

struct scoped_chunk {
	riff& rs;
	int level;
	riff::chunks_type::reverse_iterator chunk_iterator;

	scoped_chunk(riff& rs_) : rs(rs_), level(rs.begin_chunk()), chunk_iterator(rs.chunks.rbegin()) { }
	~scoped_chunk() { rs.end_chunk(level); }

	riff::chunk_type& chunk() { return *chunk_iterator; }

	fourcc_type get_chunk_id() { return chunk().head.fourcc; }
	bool has(uint32_t bytes=1) { return chunk().left >= bytes; }

	template<typename T> T read() { T rv; rs.read(&rv); return rv; }
	template<typename T> void read(T& t) { rs.read(&t); }
	void read(void *p,size_t n) { rs.read(p,n); }

};

struct chunk_path_type : public std::list<std::string> {

	chunk_path_type() { }
	chunk_path_type(const char *str) {
	for(const char *p0=str,*p1=strchr(p0,'/');p0;p1=p1?strchr(p0=++p1,'/'):(p0=0))
		if(p0!=p1)
			push_back( p1? std::string(p0,p1-p0) : std::string(p0) );
	}

};

struct chunk_walker {
	riff& r;
	chunk_path_type path;

	chunk_walker(riff& r_) : r(r_) { }

	void shoot(scoped_chunk& chunk) {
		riff::chunk_type& c = chunk.chunk();
		while(c.left) {
			static char tmp[512];
			int r = (c.left<sizeof(tmp)) ? c.left : sizeof(tmp) ;
			chunk.read(tmp,r); std::cout.write(tmp,r);
		}
		if(first) throw exceptional_success();
	}

	void walk_chunks() {
		scoped_chunk chunk(r);
		std::string pc = fourcc2str(chunk.get_chunk_id());
		bool list = false;
		switch(chunk.get_chunk_id()) {
		case fourcc_RIFF: case fourcc_LIST:
			list = true;
			pc += '.'; pc += fourcc2str(chunk.read<fourcc_type>());
			break;
		}
		path.push_back(pc);
		std::pair<chunk_path_type::const_iterator,chunk_path_type::const_iterator>
			mm = std::mismatch(path.begin(),path.end(),target.begin());
		bool dive = list;
		if(mm.first==path.end()) {
			if(mm.second==target.end()) {
				shoot(chunk);
				dive = false;
			}
		}else{
			assert(mm.second!=target.end());
			dive = false;
		}

		if(dive) while(chunk.has()) walk_chunks();
		path.pop_back();
	}


	chunk_path_type target;
	chunk_walker& set_target(chunk_path_type t) { target = t; return *this; }
	bool first;
	chunk_walker& set_oneshot(bool os) { first = true; return *this; }
};

void usage(char **argv) {
	std::cerr << PHEADER << std::endl << std::endl
		<< ' ' << argv[0] << " [options] <avi-file> <chunk-path>" << std::endl
		<< std::endl <<
		" -h, --help,\n"
		" --usage        display this text\n"
		" -V, --version  display version information\n"
		" -L, --license  show license\n"
		"\n"
		"Example: " << argv[0] << " DSC_0001.AVI '/RIFF.AVI /LIST.ncdt/nctg' \\\n"
		"\t\t| dd bs=1 skip=82 count=19 2>/dev/null\n"
		"Output:  YYYY:MM:DD HH:MM:SS (for Nikon-recorded AVI)"
		<< std::endl << std::endl;
	exit(0);
}

int main(int argc,char **argv) try {

	bool first = false;

	for(;;) {
		static struct option opts[] = {
			{ "first", no_argument, 0, '1' },
			{ "help", no_argument, 0, 'h' },
			{ "usage", no_argument, 0, 'h' },
			{ "version", no_argument, 0, 'V' },
			{ "license", no_argument, 0, 'L' },
			{ NULL, 0, 0, 0 }
		};
		int c = getopt_long(argc,argv,"1hVL",opts,NULL);
		if(c==-1) break;
		switch(c) {
		case 'h':
			usage(argv); break;
		case 'V':
			std::cerr << VERSION << std::endl;
			exit(0); break;
		case 'L':
			extern const char *COPYING;
			std::cerr << COPYING << std::endl;
			exit(0); break;
		case '1':
			first = true; break;
		default:
			std::cerr << "Huh?" << std::endl;
			exit(1); break;
		}
	}

	if(optind!=(argc-2)) usage(argv);

	std::ifstream i(argv[optind],std::ios::in);
	if(!i) {
		std::cerr << "Failed to open file '" << argv[optind] << "'" << std::endl;
		exit(1);
	}
	riff rs(i);
	chunk_walker(rs)
		.set_target(chunk_path_type(argv[++optind]))
		.set_oneshot(first)
		.walk_chunks();
	return 0;
}catch(exceptional_success&) {
	return 0;
}catch(std::exception &e) {
	std::cerr << "oops: " << e.what() << std::endl;
	return 1;
}
