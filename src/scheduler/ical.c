/*
 * ical.c
 *
 * Created: 14/05/2015 10:09:24 AM
 *  Author: Kyle
 *
 *  This module handles the iCal events for Blink. The
 *  variables to describe the iCal events are loosely based on
 *  the iCalendar spec (https://tools.ietf.org/html/rfc5545).
 *  Not all value types are implemented due to the complexity
 *  and unsuitable nature of the types. The schedule variables
 *  are defined as follows:
 *
 *    t_start -   The start date as well as the starting time
 *                for each schedule.
 *
 *    t_end -     The end date as well as the ending time
 *                for each schedule.
 *
 *    freq -      Sets the recurrence rate for each schedule. ie. 
 *                SECONDLY, MINUTELY, HOURLY. A freq of LIMITS will
 *                return a START and END flag only
 *
 *    interval -  Sets the recurrence interval for each schedule. ie. 
 *                how often the event will be repeated within the 
 *                given start and end times based on the freq
 *
 *    byday -     Mask that prevents schedules from occuring on a given
 *                day. ie. If schedule is only desired on Mondays, Wednesdays
 *                and Fridays, then byday = MO|WE|FR
 *
 *    enabled -   Enables the given schedule for computation
 *
 *    count -     Determines how many times an event can be triggered for 
 *                the schedule. A value of 0 means there is no count limit
 *
 *
 *  eg 1. Every 15 minutes, on Monday and Thursday from 8pm till 8am the next day
 *    Start Date = 2016/10/24
 *    Start Time = 20:00:00
 *    End Date = 2016/12/24
 *    End Time = 08:00:00
 *    FREQ = MINUTELY
 *    INTERVAL = 15
 *    BYDAY = MO|TH
 */ 

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "ical.h"

#define ONE_MIN  60
#define ONE_HOUR 60*ONE_MIN
#define ONE_DAY  24*ONE_HOUR

// Static Functions
static ICALEVENT _ical_find_next_recur_event(ICAL *const ical, struct tm *dt_current_time, struct tm *dt_next_event);
static void _ical_set_new_start_and_end_times(struct tm *dt_date, struct tm *dt_start, struct tm *dt_end);
static bool _is_day_of_week(uint8_t wday, BYDAY cal_day);

/**
 * \brief Get default values for schedule struct
 *   
 */
void ical_get_defaults(ICAL *const ical)
{
    // For ease of use, dt_start is comprised of start date and start time
    ical->t_start.tm_year = 116; /* year   */
    ical->t_start.tm_mon = 0;   /* month, range 0 to 11             */
    ical->t_start.tm_mday = 1;  /* day of the month, range 1 to 31  */
    ical->t_start.tm_hour = 8;  /* hours, range 0 to 23             */
    ical->t_start.tm_min = 0;   /* minutes, range 0 to 59           */
    ical->t_start.tm_sec = 0;   /* seconds,  range 0 to 59          */
    ical->t_start.tm_isdst = -1;

    // For ease of use, dt_end is comprised of end date and end time
    ical->t_end.tm_year = 120;  /* year   */
    ical->t_end.tm_mon = 11;    /* month, range 0 to 11             */
    ical->t_end.tm_mday = 31;   /* day of the month, range 1 to 31  */
    ical->t_end.tm_hour = 17;   /* hours, range 0 to 23             */
    ical->t_end.tm_min = 0;     /* minutes, range 0 to 59           */
    ical->t_end.tm_sec = 0;     /* seconds,  range 0 to 59          */
    ical->t_end.tm_isdst = -1;

    ical->freq = MINUTELY;
    ical->interval = 5;
    ical->byday = WEEKDAYS;
    ical->enabled = false;
    ical->count = 0;
}

/**
 * \brief Helper function for editing time struct
 *   
 */
void ical_set_time_struct(struct tm *t, uint16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)

{
    t->tm_sec = sec;        /* seconds,  range 0 to 59          */
    t->tm_min = min;        /* minutes, range 0 to 59           */
    t->tm_hour = hour;      /* hours, range 0 to 23             */
    t->tm_mday = day;       /* day of the month, range 1 to 31  */
    t->tm_mon = mon-1;      /* month, range 0 to 11             */
    t->tm_year = year-1900; /* year   */
    t->tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
}

/**
 * \brief Find next event in an ical struct
 *
 *  This function first checks if any of the ical inputs are invalid
 *  Then it checks if the current time is within the start and end date. If
 *  these are all valid, _ical_find_next_recur_event is called.
 *   
 */
ICALEVENT ical_find_next_event(ICAL *const ical, struct tm *t_current_time, struct tm *t_next_event)
{
    ICALEVENT event = ICALEVENT_NONE;

    // Convert all times to seconds since epoch
    time_t e_current = mktime(t_current_time);
    time_t e_start = mktime(&ical->t_start);
    time_t e_end = mktime(&ical->t_end);

    // Error checking
    if(e_start > e_end){
        event = ICALERROR_START_GREATER_THAN_END;
        return(event);
    }else if(!ical->interval){
        event = ICALERROR_INVALID_INTERVAL;
        return(event);
    }else if(ical->interval>24&&ical->freq==HOURLY){
        event = ICALERROR_INVALID_INTERVAL;
        return(event);
    }else if(ical->byday==0 || ical->byday>0x7F){
        event = ICALERROR_INVALID_BYDAY;
        return(event);
    }else if(ical->freq>3){
        event = ICALERROR_INVALID_FREQ;
        return(event);
    }else if(!(_is_day_of_week(ical->t_start.tm_wday, ical->byday)) &&
           !(ical->interval%168)&&(ical->freq==HOURLY)){
        event = ICALERROR_INVALID_RECURRENCE;
        return(event);
    }

    // Continue if ical active
    if(ical_is_enabled(ical)){
        if (e_current < e_start){ // Upcoming ical event
            event = ICALEVENT_START;
            *t_next_event = ical->t_start;
        }else if ((e_current >= e_start) && (e_current < e_end)){ // Active ical events
            // Get next recurring event
            event = _ical_find_next_recur_event(ical, t_current_time, t_next_event);
        }else{ // Past ical event
            event = ICALEVENT_NONE;
        }
    }

    return(event);
}

/**
 * \brief Find next recurring event in an ical struct
 *   
 *  This function first checks if the current time is within the start/end times
 *  of the previous day's schedule. If not, then the day is incremented and the 
 *  next days schedule is checked. This continues until the algorithm reaches the
 *  end of the week (current day + 7).
 *
 */
ICALEVENT _ical_find_next_recur_event(ICAL *const ical, struct tm *t_current_time, struct tm *t_event)
{
    ICALEVENT event = ICALEVENT_NONE;
    uint8_t i = 0, count = 0;

    time_t e_next_event = mktime(t_current_time);
    struct tm t_temp_start = ical->t_start;
    struct tm t_temp_end = ical->t_end;
    struct tm * t_next_event;

    // Iterate through each day until an active schedule is found
    while(event==ICALEVENT_NONE && i<10){

        if(i==0){
            // Start by checking the schedule from the day before
            e_next_event -= ONE_DAY;
        }else{
            // Now increment the day
            e_next_event += ONE_DAY;
        }

        // Convert back to human readable
        t_next_event = localtime(&e_next_event);

        // Check if schedule is active on this day
        if(_is_day_of_week(t_next_event->tm_wday, ical->byday)){
          
            // Reset start and end times
            _ical_set_new_start_and_end_times(t_next_event, &t_temp_start, &t_temp_end);
            time_t e_current = mktime(t_current_time);
            time_t e_start_time = mktime(&t_temp_start);
            time_t e_end_time = mktime(&t_temp_end);

            if(e_current < e_start_time){
                // If current time is before schedule start, then that is next event
                e_next_event = e_start_time;
                event = ICALEVENT_START;
            }else if((e_current >= e_start_time) && (e_current < e_end_time)){
                // If current time is in between start and end, find next event
                e_next_event = e_start_time;
                // Restart count
                count = 0;
                // Count up in increments of 'interval' to find next event
                while (e_next_event <= e_current)
                {
                    switch (ical->freq)
                    {
                        case LIMITS:
                            e_next_event = e_end_time;
                            break;
                          
                        case SECONDLY:
                            e_next_event += ical->interval;
                            break;

                        case MINUTELY:
                            e_next_event += ical->interval*60;
                            break;

                        case HOURLY:
                            e_next_event += ical->interval*3600;
                            break;

                        default:
                            break;
                    }
                    count++;
                }

                // If the event is found within a schedule, then we're done, otherwise continue search
                if(e_next_event <= e_end_time){
                    // Check if an occurrence counter rule is applied
                    if(ical->count){
                        if(count < ical->count){
                            event = ICALEVENT_RECUR;
                        }else{
                            // Count is exceeded, nothing else to do
                            event = ICALEVENT_NONE;
                            break;
                        }
                    }else{
                        event = ICALEVENT_RECUR;
                    }
                    // Check for special case of LIMITS
                    if(ical->freq == LIMITS){
                        event = ICALEVENT_END;
                    }
                }
            }
        }
        // Only iterate through 7 days
        // i can go up to 10 in this loop because we first step back a day, then forward
        i++;
    }
    // Update tm struct
    *t_event = *localtime(&e_next_event);

    // Final check to determine if next event time surpasses end datetime
    if(e_next_event > mktime(&ical->t_end)){
        event = ICALEVENT_NONE;
    }

    return(event);
}

/**
 * \brief Output start and end time structs based on a date
 *   
 *  Albeit somewhat convoluted, this function simplifies the process
 *  of setting the correct date and time of the scheduled event. Currently,
 *  it only checks if the start time of the schedule is greater than or equal
 *  to the end time. If this is the case, then we can be certain that the
 *  schedule starts on one day and carries over to the next day.
 *  
 *  TODO: Add support for a 'duration' variable so that this function can 
 *  handle setting a schedule longer than 24 hours
 */
static void _ical_set_new_start_and_end_times(struct tm *t_current, struct tm *t_start, struct tm *t_end)
{
    // Set start and end structs to the current date
    t_start->tm_year = t_end->tm_year = t_current->tm_year;
    t_start->tm_mon = t_end->tm_mon = t_current->tm_mon;
    t_start->tm_mday = t_end->tm_mday = t_current->tm_mday;
    // Get epoch time for easier math
    time_t e_start = mktime(t_start);
    time_t e_end = mktime(t_end);
    // Increment end time by 1 day if start time is after end time
    if (e_start > e_end){
        e_end += ONE_DAY;
        *t_end = *localtime(&e_end);
    }
}

/**
 * \brief Helper function for checking if ical is enabled
 *   
 */
bool ical_is_enabled(ICAL *const ical)
{
    if (ical->enabled==true){
        return true;
    }
    return false;
}

/*************** Static Functions *********************/

static bool _is_day_of_week(uint8_t wday, BYDAY cal_day)
{
    for(int i=0; i<7; i++){
        if(i==wday && (cal_day>>(6-i))&1){
            return true;
        }
    }
    return false;
}
