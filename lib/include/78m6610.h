#ifndef _78M6610_H_
#define _78M6610_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    float Vrms;
    float Irms;
    float Watts;
    float Pavg;
    float PF;
    float Freq;
    float KwH;
} AutoReportValues;

typedef struct _AutoReportMessage AutoReportMessage;

void ConvertAutoReport(const AutoReportMessage * message, AutoReportValues * values);
char * ConvertAutoReportToJSON(const AutoReportMessage * message);
int ReadMessage(Serial * serial, uint8_t expectedHeader, uint8_t * buffer);

#define AUTOREPORT_HEADER       0xAE

#ifdef __cplusplus
}
#endif

#endif /* _78M6610_H_ */