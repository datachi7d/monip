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

#ifdef __cplusplus
}
#endif

#endif /* _78M6610_H_ */