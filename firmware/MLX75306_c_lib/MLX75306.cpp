/*
 * ======================================================================
 *         Author:  Lucas MAHIEU (), lucas.mahieu@icloud.com
 * ======================================================================
 */

#include "MLX75306.h"

MLX75306::MLX75306(PinName miso, PinName mosi, PinName clk, PinName cs, 
	PinName frameReady) : _spi(miso, mosi, clk), _cs(cs), 
						_frameReady(frameReady)
{
}


void MLX75306::init(int freqMHz)
{
	_spi.frequency(freqMHz*1000000);
	_spi.format(MLX75306_SPI_FRAME_SIZE, MLX75306_SPI_POLARITY_PHASE_MODE);
	_spi.set_default_write_value(0x00);
	_cs = 0;
	_spi.write(MLX75306_CR);
	_spi.write(0x00);
	_spi.write(0x00);
	_cs = 1;
}

void MLX75306::start()
{
	_cs = 0;
	_spi.write(MLX75306_WU);
	_spi.write(0x00);
	_spi.write(0x00);
	_cs = 1;
}

void MLX75306::sleep()
{
	_cs = 0;
	_spi.write(MLX75306_SM);
	_spi.write(0x00);
	_spi.write(0x00);
	_cs = 1;
}

/** Set thresholds
 * This function set the low and high thresholds
 * @param low is the 4 lower bits for the low threshold, 
 * high is the 4 higher bits for the high thresholds
 * @return 0 if the operation succeed, 1 if not.
 */
int MLX75306::set_thresholds(int low, int high)
{
	_cs = 0;
	int thresholds = ((high << 4) & 0xF0) | (low & 0x0F);
	_spi.write(MLX75306_WT);
	_spi.write(thresholds);
	_spi.write(0x0);
	_cs = 1;
	
	if (thresholds == get_thresholds()) {
		return 0;
	} else {
		return 1;
	}
}

int MLX75306::get_thresholds() 
{
	_cs = 0;
	_spi.write(MLX75306_WT);
	int thresholds = _spi.write(0x00);
	_spi.write(0x0);
	_cs = 1;

	return thresholds;
}

void MLX75306::acquire_8b(uint8_t *data)
{
	_cs = 0;
	_spi.write(MLX75306_SI);
	_spi.write(TIME_INT_MSB);
	_spi.write(TIME_INT_LSB);
	_cs = 1;
	
	DigitalOut led(LED1);
	while(!_frameReady) {
		led = !led;
	}

	_cs = 0;
	_spi.write(MLX75306_RO8);
	_spi.write(0x02);
	_spi.write(0x8F);
	
	int tx_length = 143 + 1 + 17 -3;
	const char tx_buffer[tx_length] = {0x00};
	int rx_length = 143 + 1 + 17 - 3;
	char rx_buffer[rx_length] = {0x00};
	_spi.write(tx_buffer, tx_length, rx_buffer, rx_length);

	// 10 junk data at the begining
	// and 4 at the end
	data = (uint8_t*)strncpy((char*)data, rx_buffer+10, 142);

	_cs = 1;
}

