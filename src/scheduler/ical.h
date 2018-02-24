/*
 * ical.h
 *
 * Created: 14/05/2015 10:09:57 AM
 *  Author: Kyle
 */ 


#ifndef ICAL_H_
#define ICAL_H_

#include <stdint.h>
#include <time.h>
#include <stdbool.h>

typedef enum{
    LIMITS,
    SECONDLY,
    MINUTELY,
    HOURLY,
}FREQ;

typedef enum{
    SU = 0x40,
    MO = 0x20,
    TU = 0x10,
    WE = 0x08,
    TH = 0x04,
    FR = 0x02,
    SA = 0x01,
    EVERYDAY = SU|MO|TU|WE|TH|FR|SA,
    WEEKDAYS = MO|TU|WE|TH|FR,
    WEEKENDS = SU|SA,
}BYDAY;

typedef enum{
    ICALEVENT_NONE,
    ICALEVENT_START,
    ICALEVENT_RECUR,
    ICALEVENT_END,
    
    ICALERROR_INVALID_FREQ,
    ICALERROR_INVALID_BYDAY,
    ICALERROR_INVALID_INTERVAL,
    ICALERROR_INVALID_RECURRENCE,
    ICALERROR_START_GREATER_THAN_END,
}ICALEVENT;

/** 
 * This struct is loosely based on Internet Calendaring and Scheduling 
 * Core Object Specification (https://tools.ietf.org/html/rfc5545) 
*/
typedef struct {
    /* Start and End Times of Schedule */
    struct tm t_start;
    struct tm t_end;
    /* Recurrence Rules */
    /* Frequency type */
    FREQ freq;
    /* Interval of frequency */
    uint8_t interval;
    /* Weekday mask */
    BYDAY byday;
    /* Number of occurances */
    uint8_t count;
    /* Enabled/Disabled */
    bool enabled;
}ICAL;

ICALEVENT ical_find_next_event(ICAL *const ical, struct tm *t_current_time, struct tm *t_next_event);
void ical_get_defaults(ICAL *const ical);
bool ical_is_enabled(ICAL *const ical);
void ical_set_time_struct(struct tm *t, uint16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

#endif /* ICAL_H_ */