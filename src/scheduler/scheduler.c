/*
 * scheduler.c
 *
 * Created: 23/02/2018 3:28:00 PM
 *  Author: Kyle
 *
 *  This module is a wrapper for ical and is used to manage
 *  scheduling tasks for multiple ical instances. All data
 *  is dynamically allocated using linked lists. The premise
 *  for adding groups to the schedules is so that events of
 *  separate groups are calculated, however only the next 
 *  closest event of schedules within the same group are 
 *  calculated. This allows us to ignore similar schedules
 *  in the event that the event it triggers could cause 
 *  conflict.
 *
 *  
 *      // 1.  Initialize
 *      scheduler_init();
 *      
 *      ICAL ical_temp;
 *      ical_get_defaults(&ical_temp);
 *      ical_temp.enabled = true;
 *      ical_temp.interval = 20;
 *      
 *      // 2. Add schedules by passing a group number and 
 *      // an ICAL struct
 *      scheduler_add(0, &ical_temp);
 *      ical_temp.interval = 10;
 *      scheduler_add(1, &ical_temp);
 *      
 *      // 3. Loop through the schedule list
 *      uint8_t count = 0;
 *      SCHEDULE* schedule = scheduler_get_schedule_by_id(count++);
 *      while(schedule){
 *          printf("Schedule: %d\r\n", schedule->id);
 *          schedule = scheduler_get_schedule_by_id(count++);
 *      }
 *      
 *      struct tm current_time;
 *      current_time.tm_year = 118;
 *      current_time.tm_mon = 1;
 *      current_time.tm_mday = 23;
 *      current_time.tm_hour = 11;
 *      current_time.tm_min = 20;
 *      current_time.tm_sec = 0;
 *
 *      // 4. Call scheduler_update_events to populate the event list
 *      scheduler_update_events(&current_time);
 *      count = 0;
 *      EVENT* event = scheduler_get_event_by_group(count++);
 *      
 *      // 5. Loop through the event list. This will be empty if
 *      // there are no enabled schedules
 *      while(event){
 *          printf("Group: %d, Schedule id: %d, Event: %d\r\n", 
 *              event->group, event->id, event->ical_event);
 *          event = scheduler_get_event_by_group(count++);
 *      }
 *      
 *
 *
 */ 

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "scheduler.h"
#include "ical.h"

static void _event_list_clear(void);
static void _event_list_update(struct schedule_entry* s, 
                               struct tm *current_time,
                               ICALEVENT event);
static bool _event_add(ICALEVENT event, time_t epoch, 
                       uint8_t index, uint8_t group);

// Create heads of queues
typedef TAILQ_HEAD(schedule_head_s, schedule_entry) schedule_head_t;
typedef TAILQ_HEAD(event_head_s, event_entry) event_head_t;
static schedule_head_t schedule_head;
static event_head_t event_head;

static uint8_t schedule_count = 0;
static uint8_t event_count = 0;

/**
 *  Initialize scheduler
 */
void scheduler_init(void)
{
    TAILQ_INIT(&schedule_head);
    TAILQ_INIT(&event_head);
}

/**
 * \brief Add an entry into the queue
 *   
 *  This command should return false if it cannot allocate 
 *  any more memory or if the schedule limit is hit.
 */
bool scheduler_add(uint8_t group, ICAL* ical)
{
    if (schedule_count >= MAX_SCHEDULES){
        return false;
    }

    struct schedule_entry * s = malloc(sizeof(struct schedule_entry));

    if (s == NULL){
        return false;
    }

    s->schedule.id = schedule_count;
    s->schedule.group = group;
    s->schedule.ical.t_start = ical->t_start;
    s->schedule.ical.t_end = ical->t_end;
    s->schedule.ical.freq = ical->freq;
    s->schedule.ical.interval = ical->interval;
    s->schedule.ical.byday = ical->byday;
    s->schedule.ical.enabled = ical->enabled;

    TAILQ_INSERT_TAIL(&schedule_head, s, schedule_entries);
    // Increment counter
    schedule_count++;

    return true;
}

/**
 *  Add an event to the list
 *
 *  This command should return false if it cannot allocate 
 *  any more memory or if the schedule limit is hit.
 */
static bool _event_add(ICALEVENT event, time_t epoch, uint8_t id, uint8_t group)
{

    if (event_count >= MAX_SCHEDULES){
        return false;
    }

    struct event_entry * e = malloc(sizeof(struct event_entry));

    if (e == NULL){
        return false;
    }

    e->event.ical_event = event;
    e->event.id = id;
    e->event.group = group;
    e->event.epoch = epoch;

    TAILQ_INSERT_TAIL(&event_head, e, event_entries);
    // Increment counter
    event_count++;

    return true;
}

/**
 *  Get schedule by id
 *   
 *  This command returns a pointer to the schedule associated
 *  with the input id. NULL is returned if it doesn't exist
 */
SCHEDULE* scheduler_get_schedule_by_id(uint8_t id)
{

    struct schedule_entry * s = NULL;
    // Iterate through queue for the index
    TAILQ_FOREACH(s, &schedule_head, schedule_entries) {
        if (id == s->schedule.id) {
            return(&s->schedule);
        }
    }

    return(NULL);
}

/**
 *  Get event by group number
 *   
 *  This command returns a pointer to the event associated
 *  with the input group. NULL is returned if it doesn't exist
 */
EVENT* scheduler_get_event_by_group(uint8_t group)
{

    struct event_entry * e = NULL;
    // Iterate through queue for the index
    TAILQ_FOREACH(e, &event_head, event_entries) {
        if (group == e->event.group) {
            return(&e->event);
        }
    }

    return(NULL);
}

/**
 *  Clear events list
 * 
 */
static void _event_list_clear(void)
{
    event_count = 0;

    struct event_entry * e = NULL;
    while ((e = TAILQ_FIRST(&event_head))) {
        TAILQ_REMOVE(&event_head, e, event_entries);
        free(e);
    }
}

/**
 *  Remove a schedule from the bottom of the list
 *   
 *  returns false if empty
 */
bool scheduler_remove_last(void)
{
    struct schedule_entry * s = NULL;
    // Get first entry in queue
    s = TAILQ_LAST(&schedule_head, schedule_head_s);
    if(s == NULL)
    {
        return(false);
    }

    // Delete first item from queue
    TAILQ_REMOVE(&schedule_head, s, schedule_entries);
    free(s);

    return true;
}

/**
 *  Clear schedule list
 * 
 */
void scheduler_clear(void)
{
    schedule_count = 0;
    _event_list_clear();

    struct schedule_entry * s = NULL;
    while ((s = TAILQ_FIRST(&schedule_head))) {
        TAILQ_REMOVE(&schedule_head, s, schedule_entries);
        free(s);
    }
}

/**
 *  Main scheduling function to determine next alarm event(s) 
 *  based on all schedules
 */
void scheduler_update_events(struct tm *current_time)
{
    struct tm t_temp;
    ICALEVENT ical_event = ICALEVENT_NONE;
    struct schedule_entry * s = NULL;

    // Clear old event list
    _event_list_clear();

    // Loop through each schedule
    TAILQ_FOREACH(s, &schedule_head, schedule_entries) {
        // Get next event for each enabled schedule
        if(s->schedule.ical.enabled){
            ical_event = ical_find_next_event(&s->schedule.ical,
                                          current_time,
                                          &t_temp);

            // Update the list with the new event
            _event_list_update(s, &t_temp, ical_event);

        }
    }
}

/**
 *  Update event list with new schedule event
 *
 *  This function checks if the schedule group is already
 *  in the list. If yes, it then checks if the new event time
 *  happens before the stored one. If yes, update the list.
 */
static void _event_list_update(struct schedule_entry* s, 
                               struct tm *new_event_time,
                               ICALEVENT event)
{

    struct event_entry * e = NULL;
    //struct tm t_temp;
    time_t new_epoch = mktime(new_event_time);
    // Loop through all events
    TAILQ_FOREACH(e, &event_head, event_entries) {

        // Check if group is in event list
        if(s->schedule.group == e->event.group){

            // Check if new event happens before old one
            if(new_epoch < e->event.epoch){
                // Replace event with new one
                e->event.ical_event = event;
                e->event.id = s->schedule.id;
                e->event.group = s->schedule.group;
                e->event.epoch = new_epoch;
            }
            // Either way return immediately
            return;
        }
    }

    // If we're here, this means there isn't an event for 
    // the group number so create one
    _event_add(event, new_epoch, s->schedule.id, s->schedule.group);
}



