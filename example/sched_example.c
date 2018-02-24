#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../src/scheduler/scheduler.h"
#include "../src/scheduler/ical.h"

int main()
{

	printf("Starting..\r\n");
    // Initialize scheduler
    scheduler_init();
    // Add schedules
    ICAL ical_temp;

    ical_get_defaults(&ical_temp);
    ical_temp.enabled = true;
    ical_temp.interval = 20;
    scheduler_add(0, &ical_temp); // id: 0
    ical_temp.interval = 10;
    scheduler_add(1, &ical_temp); // id: 1
    ical_temp.interval = 5;
    scheduler_add(1, &ical_temp); // id: 2
    ical_temp.t_start.tm_hour = 12;
    scheduler_add(2, &ical_temp); // id: 3
    ical_temp.interval = 3;
    scheduler_add(1, &ical_temp); // id: 4
    // Remove the last schedule added
    scheduler_remove_last();
    
    // Attempt to get a specific schedule
    uint8_t count = 0;
    SCHEDULE* schedule = scheduler_get_schedule_by_id(count++);
    while(schedule){
        printf("Schedule: %d\r\n", schedule->id);
        schedule = scheduler_get_schedule_by_id(count++);
    }
    
    struct tm current_time;
    current_time.tm_year = 118; /* year   */
    current_time.tm_mon = 1;   /* month, range 0 to 11             */
    current_time.tm_mday = 23;  /* day of the month, range 1 to 31  */
    current_time.tm_hour = 11;  /* hours, range 0 to 23             */
    current_time.tm_min = 20;   /* minutes, range 0 to 59           */
    current_time.tm_sec = 0;   /* seconds,  range 0 to 59          */

    scheduler_update_events(&current_time);
    count = 0;
    EVENT* event = scheduler_get_event_by_group(count++);

    while(event){
        printf("Group: %d, Schedule id: %d, Event: %d\r\n", 
            event->group, event->id, event->ical_event);
        event = scheduler_get_event_by_group(count++);
    }


    printf("Finished\r\n\r\n");
	return 0;
}
