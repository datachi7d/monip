#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "serial.h"
#include "78m6610.h"



size_t msprintf(char ** string, const char * format, ...)
{
    int len = 0;
    if (string != NULL)
    {
        *string = NULL;

        {
            va_list args;
            va_start(args, format);
            len = vsnprintf(NULL, 0, format, args);
            va_end(args);
        }

        if (len > 0)
        {
            *string = calloc(len + 1, 0);
            if (*string != NULL)
            {
                va_list args;
                va_start(args, format);
                len = vsnprintf(*string, len + 1, format, args);
                va_end(args);
            }
        }
    }
    return (len > 0) ? len : 0;
}



struct __attribute__((__packed__)) _AutoReportMessage
{
    uint32_t Unkown1 : 24;
    uint32_t Unkown2 : 24;
    uint32_t Vrms : 24;
    uint32_t Irms : 24;
    uint32_t Watts : 24;
    uint32_t Pavg : 24;
    uint32_t PF : 24;
    uint32_t Freq : 24;
    uint32_t KwH : 24;
};

/*
vrms = np.mean(result[:,2])*1e-3
irms = np.mean(result[:,3])*7.77e-6
watts = np.mean(result[:,4])*0.005
pavg = np.mean(result[:,5])*0.005
pf = np.mean(result[:,6])*1e-3
freq = np.mean(result[:,7])*1e-3
kwh = np.mean(result[:,8])*1e-3 
*/

/*
vrms = [float(x)/1000.0 for x in data['vrms']]
irms = [float(x)*7.77e-6 for x in data['irms']]
watts = [float(x)/200.0 for x in data['watts']]
pavg = [float(x)/200.0 for x in data['pavg']]
pf = [float(x)/1000 for x in data['pf']]
freq = [float(x)/1000 for x in data['freq']]
kwh = [float(x)/1000 for x in data['kwh']]
*/


#define CONVERT_INT24(value) (int32_t)(value & 0x800000 ? value | 0xff000000 : value)


void ConvertAutoReport(const AutoReportMessage * message, AutoReportValues * values)
{
    values->Vrms  = CONVERT_INT24(message->Vrms)   / 1000.0f;
    values->Irms  = CONVERT_INT24(message->Irms)   / 128700.1287f;
    values->Watts = CONVERT_INT24(message->Watts)  / 200.0f;
    values->Pavg  = CONVERT_INT24(message->Pavg)   / 200.0f;
    values->PF    = CONVERT_INT24(message->PF)     / 1000.0f;
    values->Freq  = CONVERT_INT24(message->Freq)   / 1000.0f;
    values->KwH   = CONVERT_INT24(message->KwH)    / 1000.0f;   
}

char * ConvertAutoReportToJSON(const AutoReportMessage * message)
{
    AutoReportValues values = {0};
    char * Result = NULL;

    ConvertAutoReport(message, &values);

    msprintf(&Result, 
        "{"
        "\"Vrms\":%3.2f,"
        "\"Irms\":%3.2f,"
        "\"Watts\":%3.2f,"
        "\"Pavg\":%3.2f,"
        "\"PF\":%3.2f,"
        "\"Freq\":%3.2f,"
        "\"kWh\":%3.2f"
        "}"
        , 
        values.Vrms,
        values.Irms,
        values.Watts,
        values.Pavg,
        values.PF,
        values.Freq,
        values.KwH
    );

    return Result;

}





int ReadMessage(Serial * serial, uint8_t expectedHeader, uint8_t * buffer)
{
    uint8_t PacketHeader = 0;
    uint8_t PacketLength = 0;
    int Result = -ENODEV ; 

    if( Serial_Read(serial, &PacketHeader, 1) == 1 )
    {
        if(Serial_Read(serial, &PacketLength, 1) == 1)
        {
            Result = Serial_Read(serial, buffer, PacketLength-2);

            if(Result > 0)
            {
                uint8_t sum = 0;
                uint8_t pos = 0;

                sum += PacketHeader;
                sum += PacketLength;

                for(pos = 0; pos < Result; pos++)
                {
                    sum += buffer[pos];
                }

                sum = ((~sum)+1);

                if(sum != 0)
                {
                    Result = -EIO;
                }

                if(PacketHeader != expectedHeader && Result > 0)
                {
                    Result = -EFAULT;
                }
            }
            else
            {
                Result = -EAGAIN;
            }
        }
        else
        {
            Result = -EAGAIN;
        }
    }
    else
    {
        Result = -EAGAIN;
    }

    return Result;
}