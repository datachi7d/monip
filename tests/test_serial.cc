#include <gtest/gtest.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <signal.h>


#include <fstream>
#include <iostream>

#include "process.h"

#include "serial.h"

#include "78m6610.h"

namespace PFC
{

static jmp_buf jmp_env;

static void catch_alarm(int sig)
{
    longjmp(jmp_env, 1);
}

#define ASSERT_USECS(fn, usecs) { \
    const auto val = setjmp(jmp_env); \
    if (val == 0) { \
        signal(SIGALRM, catch_alarm); \
        ualarm((usecs), 0); \
        { fn; }; \
        ualarm(0, 0); \
    } else { \
        GTEST_FATAL_FAILURE_(#usecs " usecs timer tripped for " #fn); \
    } }


	class SerialTest : public testing::Test
	{
		pid_t _pid;
		std::string TestSerialPath;

protected:
        std::string SerialPath;
		std::fstream SerialStream;

		SerialTest(): _pid(-1), TestSerialPath("/tmp/TestSerial_monip"), SerialPath("/tmp/Serial_monip") {}

		void SetUp()
		{
			char pty1[256] = {0};
			sprintf(pty1, "pty,raw,echo=0,link=%s", TestSerialPath.c_str());

			char pty2[256] = {0};
			sprintf(pty2, "pty,raw,echo=0,link=%s", SerialPath.c_str());

			//"-d", "-d", "-d", "-d", "-D", "-lu",
			std::vector<const char *> commandVector { "/usr/bin/socat", "-D", pty1, pty2 };
			_pid = SpawnProcess(commandVector, false, false);

			int counter = 0;

			while ((!SerialStream.is_open()) && counter < 10)
			{
				SerialStream.open(TestSerialPath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
				usleep(5000);
				counter++;
			}

			usleep(5000);

			ASSERT_TRUE(SerialStream);
		}
		void TearDown()
		{
			TerminateProcess(_pid);
		}
	};


	TEST_F(SerialTest, test_Serial_NewFree_no_path)
	{
		Serial * serial = Serial_New("");

		ASSERT_TRUE(serial == NULL);

		Serial_Free(serial);
	}

	TEST_F(SerialTest, test_Serial_NewFree)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		ASSERT_TRUE(serial != NULL);

		Serial_Free(serial);
	}


	TEST_F(SerialTest, test_Serial_Write)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		EXPECT_TRUE(serial != NULL);

		uint8_t writeData[] = {0x08, 0x02, 0x04, 0x13, 0x11, 0xe0};
		char testReadData[sizeof(writeData)] = {0};

		ASSERT_EQ(Serial_Write(serial, writeData, sizeof(writeData)), sizeof(writeData));

		ASSERT_USECS(SerialStream.read(testReadData, sizeof(testReadData)), 100000);

		ASSERT_TRUE(memcmp(writeData, testReadData, sizeof(writeData)) == 0);

		Serial_Free(serial);
	}

	TEST_F(SerialTest, test_Serial_Read)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		EXPECT_TRUE(serial != NULL);

		unsigned char writeData[] = {0x01, 0x02, 0x03, 0x13, 0x11, 0xff};
		uint8_t testReadData[sizeof(writeData)] = {0};

		SerialStream.write((char *)writeData, sizeof(writeData));
		SerialStream.flush();

		ASSERT_EQ(Serial_Read(serial, testReadData, sizeof(testReadData)), sizeof(testReadData));

		ASSERT_TRUE(memcmp(writeData, testReadData, sizeof(writeData)) == 0);

		Serial_Free(serial);
	}

	TEST_F(SerialTest, test_Serial_Read_Timeout)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		EXPECT_TRUE(serial != NULL);

		unsigned char writeData[] = {0x05, 0x08, 0x02, 0x13, 0x11, 0xff};
		uint8_t testReadData[sizeof(writeData)] = {0};

		SerialStream.write((char *)writeData, sizeof(writeData) - 2);
		SerialStream.flush();

		ASSERT_EQ(Serial_Read(serial, testReadData, sizeof(testReadData)), 0);

		ASSERT_TRUE(memcmp(writeData, testReadData, sizeof(writeData)) != 0);

		Serial_Free(serial);
	}

	TEST_F(SerialTest, test_ReadMessage_Simple)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		EXPECT_TRUE(serial != NULL);

		unsigned char writeData[] = {0xae, 0x1e, 0xdc, 0x4c, 0x00 , 0xc1, 0xac, 0xff , 0xdd, 0xa8, 0x03 , 0x51, 0x11, 0x00 , 0xe9, 0xff, 0xff , 0xec, 0xff, 0xff , 0xf2, 0xff, 0xff , 0x85, 0xc1, 0x00 , 0x00, 0x00, 0x00, 0xaf};
		uint8_t testReadData[255] = {0};

		SerialStream.write((char *)writeData, sizeof(writeData));
		SerialStream.flush();

		ASSERT_EQ(ReadMessage(serial, AUTOREPORT_HEADER, testReadData), 28);

		char * output = ConvertAutoReportToJSON((AutoReportMessage *)&testReadData[0]);

		ASSERT_TRUE(output != NULL);
		//printf("%s\n", ConvertAutoReportToJSON((AutoReportMessage *)&testReadData[0]));
	}

	TEST_F(SerialTest, test_ReadMessage_Checksum_Fail)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		EXPECT_TRUE(serial != NULL);

		unsigned char writeData[] = {0xae, 0x1e, 0xdc, 0x4c, 0x00 , 0xc1, 0xab, 0xff , 0xdd, 0xa8, 0x03 , 0x51, 0x11, 0x00 , 0xe9, 0xff, 0xff , 0xec, 0xff, 0xff , 0xf2, 0xff, 0xff , 0x85, 0xc1, 0x00 , 0x00, 0x00, 0x00, 0xaf};
		uint8_t testReadData[sizeof(writeData)] = {0};

		SerialStream.write((char *)writeData, sizeof(writeData));
		SerialStream.flush();

		ASSERT_EQ(ReadMessage(serial, AUTOREPORT_HEADER, testReadData), -EIO);
	}


	TEST_F(SerialTest, test_ReadMessage_BadData)
	{
		Serial * serial = Serial_New(SerialPath.c_str());

		EXPECT_TRUE(serial != NULL);

		unsigned char writeData[] = {0xae, 0x1e, 0xdc, 0x4c, 0x00 , 0xc1, 0xac, 0xff , 0xdd, 0xa8, 0x03 , 0x51, 0x11, 0x00 , 0xe9, 0xff, 0xff , 0xec, 0xff, 0xff , 0xf2, 0xff, 0xff , 0x85, 0xc1, 0x00 , 0x00, 0x00, 0x00, 0xaf};
		uint8_t testReadData[255] = {0};

		unsigned int i = 0;

		int Result = 0;
		int Counter = 0;
		srand(time(NULL));

		do {
			printf("start...\n");
			if(Counter == 0)
			{
				for(i = 0; i < 255; i++)
				{
					unsigned char random = rand();
					SerialStream.write((char *)&random, 1);
				}
				SerialStream.flush();				
			}
			else
			{
				SerialStream.write((char *)writeData, sizeof(writeData));
				SerialStream.flush();				
			}
			
			Result = ReadMessage(serial, AUTOREPORT_HEADER, testReadData);
			printf("Counter:%d Result:%d\n", Counter, Result);
			Counter++;
		} while (Counter < 100 && Result != 28);

		ASSERT_EQ(Result, 28);
	}

}
