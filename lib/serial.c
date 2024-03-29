#include "serial.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct _Serial
{
	int serialfd;
};

typedef struct __attribute__((__packed__))
{
    uint8_t ID;
    uint8_t Length;
} Header;

void printHex(uint8_t * buffer, uint8_t len)
{
    uint8_t pos = 0;

    for(; pos < len; pos++)
        printf("%02x ", buffer[pos]);
}

uint8_t CheckSum(uint8_t * buffer, uint8_t length)
{
    uint8_t * ptr = buffer;
    uint8_t sum = 0xFF;

    for(; ptr < (buffer+length); ptr++)
        sum -= *ptr;

    return sum;
}

int SetInterfaceAttributes(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr [%d]: %s\n", fd, strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    //tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag |= PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    //tty.c_cflag |= CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */


    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;


    /* fetch bytes as they become available */
    //tty.c_cc[VMIN] = 0;
    //tty.c_cc[VTIME] = 2;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;


    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr [%d]: %s\n", fd, strerror(errno));
        return -1;
    }
    return 0;
}

int SetInterfaceVMIN(int fd, uint8_t vmin)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcsetattr [%d]: %s\n", fd, strerror(errno));
        return -1;
    }

    tty.c_cc[VMIN] = vmin;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr [%d]: %s\n", fd, strerror(errno));
        return -1;
    }
    return 0;
}

Serial * Serial_New(const char * path)
{
	Serial * serial = NULL;

	if(path != NULL)
	{
		serial = malloc(sizeof(*serial));

		if(serial != NULL)
		{
			serial->serialfd = open(path, O_RDWR | O_NOCTTY | O_SYNC);

			if (serial->serialfd >= 0)
			{
				SetInterfaceAttributes(serial->serialfd, B19200);
				lseek(serial->serialfd, 0, SEEK_END);
				printf("Serial [%p:%d]: %s\n", (void *)serial, serial->serialfd, path);
			}
			else
			{
				printf("Error opening %s: %s\n", path, strerror(errno));
				free(serial);
				serial = NULL;
			}
		}
	}

	return serial;
}

void Serial_Reset(Serial * serial)
{
	if(serial != NULL)
	{
		if (serial->serialfd >= 0)
		{
			SetInterfaceVMIN(serial->serialfd, 0);
		}
	}
}

uint8_t Serial_Read(Serial * serial, uint8_t * buffer, uint8_t size)
{
    size_t ret = 0;

    if(serial != NULL)
    {
        ret = SetInterfaceVMIN(serial->serialfd, size);
        if(ret >= 0)
        {
            ret = read(serial->serialfd, buffer, size);
        }
    }

    //TODO: debug mode
//    if(ret > 0)
//    {
//        printf("Read [%p,%d,%d]:", (void *)serial, size, (int)ret);
//        printHex(buffer, (uint8_t)ret);
//        printf("\n");
//    }

    return ((ret < 0) | (ret != size)) ? 0 : size;
}

uint8_t Serial_Write(Serial * serial, uint8_t * buffer, uint8_t size)
{
    size_t ret = 0;

    if(serial != NULL)
    {
    	ret = write(serial->serialfd, buffer, size);
    	fsync(serial->serialfd);
		tcdrain(serial->serialfd);
    }

    return ((ret < 0) | (ret != size)) ? 0 : size;
}

int Serial_GetFD(Serial * serial)
{
	int result = -1;

	if(serial != NULL)
	{
		result = serial->serialfd;
	}

	return result;
}

void Serial_Free(Serial * serial)
{

	if(serial != NULL)
	{
		if(serial->serialfd >= 0)
		{
			close(serial->serialfd);
		}

		free(serial);
	}
}

/*
pfc_error Serial_ReadPFCMessage(Serial * serial, PFC_ID * ID, uint8_t * data, pfc_size * size)
{
	pfc_error result = PFC_ERROR_UNSET;

	if(serial != NULL && ID != NULL && data != NULL && size != NULL)
	{
		uint8_t data_buffer[PFC_MAX_MESSAGE_LENGTH] = {0};
		PFC_Header * header = (PFC_Header *)&data_buffer[0];
		int readLen = 0;

		readLen = Serial_Read(serial, (uint8_t *)header, sizeof(*header));

		if(readLen == sizeof(*header))
		{
			if(header->Length < PFC_MAX_MESSAGE_LENGTH)
			{
				*size = header->Length - sizeof(*header);
				//uint8_t RecvChecksum = 0;

				if(header->Length >= sizeof(*header))
				{
					readLen = Serial_Read(serial, &data_buffer[sizeof(*header)], (*size) + 1);
				}
				else
				{
					readLen = 0;
				}

				if(readLen == (*size)+1)
				   //&& Serial_Read(serial, &, 1) == 1)
				{
					if(CheckSum(data_buffer, header->Length) == data_buffer[sizeof(*header) + (*size)])
					{
						*ID = header->ID;
						memcpy(data, &data_buffer[sizeof(*header)], *size);
// TODO: debug mode
//						printf("Recv Message [%p]: ", (void *)serial);
//						printHex(data_buffer,*size + header->Length);
//						printf("\n");
						result = PFC_ERROR_NONE;
					}
					else
					{
						*size = 0;
						result = PFC_ERROR_CHECKSUM;
					}
				}
				else
				{
					*size = 0;
					result = PFC_ERROR_TIMEOUT;
				}

			}
			else
			{
				result = PFC_ERROR_MESSAGE_LENGTH;
			}
		}
		else
		{
			result = PFC_ERROR_TIMEOUT;
		}
	}
	else
	{
		result = PFC_ERROR_NULL_PARAMETER;
	}

	return result;
}


pfc_error Serial_WritePFCMessage(Serial * serial, PFC_ID ID, uint8_t * data, pfc_size size)
{
	pfc_error result = PFC_ERROR_UNSET;

	if(serial != NULL)
	{
		uint8_t data_buffer[PFC_MAX_MESSAGE_LENGTH] = {0};
		PFC_Header * header = (PFC_Header *)data_buffer;

	    header->ID = ID;
	    header->Length = sizeof(*header);

	    if(data != NULL
	       && size > 0
		   && size < PFC_MAX_MESSAGE_LENGTH)
	    {
	        header->Length += size;
	        memcpy(&data_buffer[sizeof(*header)], data, size);
	        result = PFC_ERROR_NONE;
	    }
	    else
	    {
	    	if(size != 0)
	    	{
	    		result = PFC_ERROR_LENGTH;
	    	}
	    	else
	    	{
	    		result = PFC_ERROR_NONE;
	    	}
	    }

	    if	(result == PFC_ERROR_NONE)
	    {
	    	data_buffer[header->Length] = CheckSum(data_buffer, header->Length);
	    	Serial_Write(serial, data_buffer, header->Length + 1);
// TODO: debug
//			printf("Send Message [%p]: ", (void *)serial);
//			printHex(data_buffer,header->Length + 1);
//			printf("\n");

	    }
	}
	else
	{
		result = PFC_ERROR_NULL_PARAMETER;
	}

	return result;
}

pfc_error Serial_WritePFCAcknowledge(Serial * serial, PFC_ID ID)
{
	return Serial_WritePFCMessage(serial, PFC_ID_ACK, NULL, 0);
}
*/


void Serial_FlushInput(Serial * serial)
{
    tcflush(serial->serialfd, TCIFLUSH);
}


