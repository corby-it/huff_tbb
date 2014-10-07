#ifndef TIMER_H
#define TIMER_H

#include <windows.h>

class timer {
	__int64 _i64Frequency;
	__int64 _i64Start,_i64Stop;
	bool _bRunning;
    double elapsedTime;
public:
	timer() : _bRunning(false) {
		QueryPerformanceFrequency ((LARGE_INTEGER*)&_i64Frequency);
	}
    
	void start() {
		_bRunning = true;
		QueryPerformanceCounter ((LARGE_INTEGER*)&_i64Start);
	}
    
	void stop() {
		QueryPerformanceCounter ((LARGE_INTEGER*)&_i64Stop);
		_bRunning = false;
	}
    
	double elapsed_ms() {
		if (_bRunning)
			stop();
		double dFrequency = double(_i64Frequency);
		double dStart = double(_i64Start);
		double dStop = double(_i64Stop);
		return (dStop-dStart)*1000.0/dFrequency;
	}
};

#endif /* TIMER_H */