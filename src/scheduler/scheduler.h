/*
 * scheduler.h
 *
 * Created: 14/05/2015 10:09:57 AM
 *  Author: Kyle
 */ 

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "ical.h"
#include "queue.h"

#define MAX_SCHEDULES 5

typedef struct {
    // Defines calendar event and recurrence (see ical.c)
    ICAL ical;

    // Schedule Group
    // Schedules are grouped together to determine whether
    // there can be an event at the same time
    uint8_t group;

    uint8_t id;
}SCHEDULE;

typedef struct {
    // Flag identifying the event to be triggered
    ICALEVENT ical_event;
    // Event time
    time_t epoch;
    // Schedule id
    uint8_t id;
    // Group
    uint8_t group;
}EVENT;

typedef struct schedule_entry
{
    SCHEDULE schedule;

    TAILQ_ENTRY(schedule_entry) schedule_entries;
}schedule_entry_t;

typedef struct event_entry
{
    EVENT event;

    TAILQ_ENTRY(event_entry) event_entries;
}event_entry_t;

void scheduler_clear(void);
void scheduler_init(void);
bool scheduler_add(uint8_t group, ICAL* ical);
bool scheduler_remove_last(void);
SCHEDULE* scheduler_get_schedule_by_id(uint8_t id);
EVENT* scheduler_get_event_by_group(uint8_t group);
void scheduler_update_events(struct tm *current_time);

#endif /* SCHEDULER_H_ */