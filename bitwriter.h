#ifndef BITWRITER_H
#define BITWRITER_H

#include <iostream>
#include <vector>
#include <cstdint>

//!  BitWriter class is used to write bit-by-bit 
/*!
A more elaborate class description.
*/
class BitWriter {
	//! ostream.
	/*! More detailed ostream description. */
	std::vector<std::uint8_t>& _f;
	//! uint8_t.
	/*! More detailed uint8_t description. */
	std::uint8_t _buf;
	//! uint8_t.
	/*! More detailed uint8_t description. */
	std::uint8_t _count;
	//! uint8_t.
	/*! More detailed uint8_t description. */
	std::uint32_t _index;

	//! write_bit function description
	/*!
	\param u an unsigned 32 bit integer
	\return void
	*/
	void write_bit (std::uint32_t u) {
		_buf = (_buf<<1) + (u&1);
		_count++;
		if (_count==8) {
			//! Write one byte (_buf content) into the stream
			_f.push_back(_buf);
			_index++;
			//_f.write(reinterpret_cast<const char*>(&_buf),1);
			_count = 0;
		}
	}

public:
	//! A constructor.
	/*!
	A more elaborate description of the constructor.
	*/
	BitWriter (std::vector<std::uint8_t>& f) : _f(f), _count(0), _index(0) {}

	//! write function description
	/*!
	\param u an unsigned 32 bit integer.
	\param n an unsigned 8 bit integer.
	\return void
	\sa BitWriter::flush()
	*/
	void write (std::uint32_t u, std::uint8_t n) {
		while (n-->0)
			write_bit(u>>n);
	}

	//! flush function description.
	/*!
	\param void
	\return void
	\sa BitWriter::write (uint32_t u, uint8_t n)
	*/
	void flush() {
		while (_count>0)
			write_bit(0);
	}

	std::uint32_t tell_index(){
		return _index;
	}

	void seek_index(std::uint32_t idx){
		_index = idx;
	}

	void reset_index(){
		_index = 0;
	}
};

#endif /*BITWRITER_H*/