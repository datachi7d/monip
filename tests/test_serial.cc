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

			std::vector<const char *> commandVector { "/usr/bin/socat", "-d", "-d", "-d", "-d", "-D", "-lu", pty1, pty2 };
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
		uint8_t testReadData[sizeof(writeData)] = {0};

		SerialStream.write((char *)writeData, sizeof(writeData));
		SerialStream.flush();

		ASSERT_EQ(ReadMessage(serial, AUTOREPORT_HEADER, testReadData), 28);
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

	class Message : public testing::Test
	{
	
	};

	TEST_F(Message, test_Message)
	{
		
		//uint8_t buffer[256] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x01, 0x00, 0x00 };
		//uint8_t buffer[256] = { 0xc7, 0x44, 0x00, 0xb0, 0xac, 0xff, 0xdb, 0xa8, 0x03, 0x14, 0x11, 0x00, 0x12, 0x00, 0x00, 0xee, 0xff, 0xff, 0x0b0, 0x00, 0x08, 0x5c, 0x10, 0x00, 0x00, 0x00, 0x0d };
		uint8_t buffer[256] = { 0xdc, 0x4c, 0x00 , 0xc1, 0xac, 0xff , 0xdd, 0xa8, 0x03 , 0x51, 0x11, 0x00 , 0xe9, 0xff, 0xff , 0xec, 0xff, 0xff , 0xf2, 0xff, 0xff , 0x85, 0xc1, 0x00 , 0x00, 0x00, 0x00 };
		printf("%s\n", ConvertAutoReportToJSON((AutoReportMessage *)&buffer[0]));
	}
	// TEST_F(PFC_Serial, test_Serial_ReadMessage)
	// {
	// 	Serial * serial = Serial_New(SerialPath.c_str());

	// 	EXPECT_TRUE(serial != NULL);

	// 	char writeData[] = {0xf6, 0x03, 0x00, 0x06};
	// 	uint8_t testReadData[255] = {0};

	// 	SerialStream.write(writeData, sizeof(writeData));
	// 	SerialStream.flush();

	// 	PFC_ID ID = 0;
	// 	pfc_size size = 0;

	// 	ASSERT_EQ(Serial_ReadPFCMessage(serial, &ID, testReadData, &size), PFC_ERROR_NONE);

	// 	ASSERT_EQ(size, 1);
	// 	ASSERT_EQ(ID, 0xf6);
	// 	ASSERT_EQ(testReadData[0], 0);

	// 	Serial_Free(serial);
	// }

	// TEST_F(PFC_Serial, test_Serial_WriteMessage)
	// {
	// 	Serial * serial = Serial_New(SerialPath.c_str());

	// 	EXPECT_TRUE(serial != NULL);

	// 	char expectedData[] = {0xf6, 0x03, 0x00, 0x06};
	// 	char testReadData[255] = {0};

	// 	PFC_ID ID = 0xf6;
	// 	pfc_size size = 1;
	// 	uint8_t writeData = 0;

	// 	ASSERT_EQ(Serial_WritePFCMessage(serial, ID, &writeData, size), PFC_ERROR_NONE);

	// 	ASSERT_USECS(SerialStream.read(testReadData, sizeof(expectedData)), 100000);

	// 	ASSERT_TRUE(memcmp(expectedData, testReadData, sizeof(expectedData)) == 0);

	// 	Serial_Free(serial);
	// }

}
