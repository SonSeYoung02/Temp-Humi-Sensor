/*
 * DHT22 for Raspberry Pi with WiringPi
 * Original Author: Hyun Wook Choi
 * Original Version: 0.1.0
 * Forked from: https://github.com/ccoong7/DHT22
 */

#include <stdio.h>
#include <wiringPi.h>

#define DHT22 18 // GPIO 18번(물리적 핀 12번)

// 코드 추가
typedef struct{ // 구조체 선언
	float humidity;
	float celsius;
	float fahrenheit;
	int success; // 1 = 정상, 0 = 실패
}DHT22Result;

short readData(void);
extern unsigned short data[5]; // 해당 데이터 전역변수로 설정
// 코드 추가 끝

static const unsigned short signal = DHT22; // GPIO 18번 읽기(물리적 12번핀)
unsigned short data[5] = {0, 0, 0, 0, 0};

short readData()
{
	unsigned short val = 0x00;
	unsigned short signal_length = 0;
	unsigned short val_counter = 0;
	unsigned short loop_counter = 0;

	while (1)
	{
		// Count only HIGH signal
		while (digitalRead(signal) == HIGH)
		{
			signal_length++;

			// When sending data ends, high signal occur infinite.
			// So we have to end this infinite loop.
			if (signal_length >= 200)
			{
				return -1;
			}

			delayMicroseconds(1);
		}

		// If signal is HIGH
		if (signal_length > 0)
		{
			loop_counter++;	// HIGH signal counting

			// The DHT22 sends a lot of unstable signals.
			// So extended the counting range.
			if (signal_length < 10)
			{
				// Unstable signal
				val <<= 1;		// 0 bit. Just shift left
			}

			else if (signal_length < 30)
			{
				// 26~28us means 0 bit
				val <<= 1;		// 0 bit. Just shift left
			}

			else if (signal_length < 85)
			{
				// 70us means 1 bit
				// Shift left and input 0x01 using OR operator
				val <<= 1;
				val |= 1;
			}

			else
			{
				// Unstable signal
				return -1;
			}

			signal_length = 0;	// Initialize signal length for next signal
			val_counter++;		// Count for 8 bit data
		}

		// The first and second signal is DHT22's start signal.
		// So ignore these data.
		if (loop_counter < 3)
		{
			val = 0x00;
			val_counter = 0;
		}

		// If 8 bit data input complete
		if (val_counter >= 8)
		{
			// 8 bit data input to the data array
			data[(loop_counter / 8) - 1] = val;

			val = 0x00;
			val_counter = 0;
		}
	}
}

DHT22Result readDHT22(void)
{
	DHT22Result result = {0.0f, 0.0f, 0.0f, 0};
	short checksum;

	for (unsigned char i = 0; i < 10; i++)
	{
		pinMode(signal, OUTPUT);

		// Send out start signal
		digitalWrite(signal, LOW);
		delay(20);					// Stay LOW for 5~30 milliseconds
		pinMode(signal, INPUT);		// 'INPUT' equals 'HIGH' level. And signal read mode

		readData();		// Read DHT22 signal

		// The sum is maybe over 8 bit like this: '0001 0101 1010'.
		// Remove the '9 bit' data using AND operator.
		checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;

		// If Check-sum data is correct (NOT 0x00), display humidity and temperature
		if (data[4] == checksum && checksum != 0x00)
		{
			// * 256 is the same thing '<< 8' (shift).
			result.humidity = ((data[0] * 256) + data[1]) / 10.0;
			
			// found that with the original code at temperatures > 25.4 degrees celsius
			// the temperature would print 0.0 and increase further from there.
			// Eventually when the actual temperature drops below 25.4 again
			// it would print the temperature as expected.
			// Some research and comparisin with other C implementation suggest a
			// different calculation of celsius.
			//celsius = data[3] / 10.0; //original
			result.celsius = (((data[2] & 0x7F)*256) + data[3]) / 10.0; //Juergen Wolf-Hofer

			// If 'data[2]' data like 1000 0000, It means minus temperature
			if (data[2] == 0x80)
			{
				result.celsius *= -1;
			}

			result.fahrenheit = ((result.celsius * 9) / 5) + 32;
			result.success = 1; // 성공
			
			return result;
		}

		else
		{
			printf("[Error] 올바른 데이터가 아님니다. 다시 시도해주세요.\n\n");
			result.success = 0; // 실패

			return result;
		}

		// Initialize data array for next loop
		for (unsigned char i = 0; i < 5; i++) // 배열 초기화
		{
			data[i] = 0;
		}

		delay(2000);	// DHT22 average sensing period is 2 seconds
	}

	return result;
}
