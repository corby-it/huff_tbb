#ifndef BITREADER_H
#define BITREADER_H

#include <iostream>
#include <cstdint>

using namespace std;

struct bitreader {
	istream& _f;
	uint8_t _buf;
	uint8_t _count;
	
	bitreader (istream& f) : _f(f),_count(0) {}
	uint32_t read_bit() {
		if (_count==0) {
			_f.read(reinterpret_cast<char*>(&_buf),1);
			_count = 8;
		}
		return (_buf>>--_count)&1;
	}
	uint32_t read (uint8_t n) {
		uint32_t u = 0;
		while (n-->0)
			u = (u<<1) | read_bit();
		return u;
	}
};

struct bitwriter {
	ostream& _f;
	uint8_t _buf;
	uint8_t _count;
	
	bitwriter (ostream& f) : _f(f),_count(0) {}
	void write_bit (uint32_t u) {
		_buf = (_buf<<1) + (u&1);
		_count++;
		if (_count==8) {
			_f.write(reinterpret_cast<const char*>(&_buf),1);
			_count = 0;
		}
	}
	void write (uint32_t u, uint8_t n) {
		while (n-->0)
			write_bit(u>>n);
	}
	void flush() {
		while (_count>0)
			write_bit(0);
	}
};

#endif /* BITREADER_H */