/*
 * ======================================================================
 *         Author:  Lucas MAHIEU (), lucas.mahieu@icloud.com
 * ======================================================================
 */

#ifndef MBED_MLX75306_H
#define MBED_MLX75306_H


#include "mbed.h"
#include "spi.h"

// Number of bits per spi frame
#define MLX75306_SPI_FRAME_SIZE 24 
//*
// mode | POL PHA
// -----+--------
//   0  |  0   0
//   1  |  0   1
//   2  |  1   0
//   3  |  1   1
//*
#define MLX75306_SPI_POLARITY_PHASE_MODE 3

#define MLX75306_NOP 0b0000000
#define MLX75306_CR 0b11110000
#define MLX75306_RT 0b11011000
#define MLX75306_WT 0b11001100
#define MLX75306_SI 0b10111000
#define MLX75306_SIL 0b10110100
#define MLX75306_RO1 0b10011100
#define MLX75306_RO2 0b10010110
#define MLX75306_RO4 0b10010011
#define MLX75306_RO8 0b10011001
#define MLX75306_TZ1 0b11101000
#define MLX75306_TZ2 0b11100100
#define MLX75306_TZ12 0b11100010
#define MLX75306_TZ0 0b11100001
#define MLX75306_SM 0b11000110
#define MLX75306_WU 0b11000011

#define TIME_INT_MSB				0b01001110
#define TIME_INT_LSB				0b00100100

class MLX75306
{
	public:
		MLX75306(PinName mosi, PinName miso, PinName clk, PinName cs, 
				PinName frameReady);
		
		void init(int freqMHz);

		void start();
		void sleep();
		void reset();

		void acquire_8b(uint8_t *data);
		//void acquire_4b();
		//void acquire_1b();

		int set_thresholds(int low, int high);
		int get_thresholds();

	private:
		SPI _spi;
		DigitalOut _cs;
		DigitalIn _frameReady;
};



#endif
