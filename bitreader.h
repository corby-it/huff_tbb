#ifndef BITREADER_H
#define BITREADER_H

#include <iostream>
#include <cstdint>

using namespace std;

//!  A test class. 
/*!
  A more elaborate class description.
*/
struct bitreader {
	//! istream.
    /*! More detailed istream description. */
	istream& _f;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	uint8_t _buf;
	//! An uint8_t.
    /*! More detailed uint8_t description. */
	uint8_t _count;
	
	//! A constructor.
    /*!
      A more elaborate description of the constructor.
    */
	bitreader (istream& f) : _f(f),_count(0) {}

	//! A normal member taking two arguments and returning an integer value.
    /*!
      \param a an integer argument.
      \param s a constant character pointer.
      \return The test results
      \sa Test(), ~Test(), testMeToo() and publicVar()
    */
	uint32_t read_bit() {
		if (_count==0) {
			_f.read(reinterpret_cast<char*>(&_buf),1);
			_count = 8;
		}
		return (_buf>>--_count)&1;
	}

	//! A normal member taking two arguments and returning an integer value.
    /*!
      \param a an integer argument.
      \param s a constant character pointer.
      \return The test results
      \sa Test(), ~Test(), testMeToo() and publicVar()
    */
	uint32_t read (uint8_t n) {
		uint32_t u = 0;
		while (n-->0)
			u = (u<<1) | read_bit();
		return u;
	}
};

//!  A test class. 
/*!
  A more elaborate class description.
*/
struct bitwriter {
	//! ostream.
    /*! More detailed ostream description. */
	ostream& _f;
	//! uint8_t.
    /*! More detailed uint8_t description. */
	uint8_t _buf;
	//! uint8_t.
    /*! More detailed uint8_t description. */
	uint8_t _count;
	
	//! A constructor.
    /*!
      A more elaborate description of the constructor.
    */
	bitwriter (ostream& f) : _f(f),_count(0) {}

	//! A normal member taking two arguments and returning an integer value.
    /*!
      \param a an integer argument.
      \param s a constant character pointer.
      \return The test results
      \sa Test(), ~Test(), testMeToo() and publicVar()
    */
	void write_bit (uint32_t u) {
		_buf = (_buf<<1) + (u&1);
		_count++;
		if (_count==8) {
			_f.write(reinterpret_cast<const char*>(&_buf),1);
			_count = 0;
		}
	}

	//! A normal member taking two arguments and returning an integer value.
    /*!
      \param a an integer argument.
      \param s a constant character pointer.
      \return The test results
      \sa Test(), ~Test(), testMeToo() and publicVar()
    */
	void write (uint32_t u, uint8_t n) {
		while (n-->0)
			write_bit(u>>n);
	}

	//! A normal member taking two arguments and returning an integer value.
    /*!
      \param a an integer argument.
      \param s a constant character pointer.
      \return The test results
      \sa Test(), ~Test(), testMeToo() and publicVar()
    */
	void flush() {
		while (_count>0)
			write_bit(0);
	}
};

#endif /* BITREADER_H */