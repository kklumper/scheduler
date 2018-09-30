#include "unity.h"
#include "ical.h"
#include "scheduler.h"

static struct tm current_time;
EVENT *next_event;
  
void assert_test_time(const struct tm *time, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
    TEST_ASSERT_EQUAL_UINT16(sec, time->tm_sec);
    TEST_ASSERT_EQUAL_UINT16(min, time->tm_min);
    TEST_ASSERT_EQUAL_UINT16(hour, time->tm_hour);
    TEST_ASSERT_EQUAL_UINT16(day, time->tm_mday);
    TEST_ASSERT_EQUAL_UINT16(month, time->tm_mon+1);
    TEST_ASSERT_EQUAL_UINT16(year, time->tm_year+1900);
}

void setUp(void)
{
    // Clear all schedules
    scheduler_init();

    current_time.tm_year = 118; /* year   */
    current_time.tm_mon = 1;   /* month, range 0 to 11             */
    current_time.tm_mday = 23;  /* day of the month, range 1 to 31  */
    current_time.tm_hour = 11;  /* hours, range 0 to 23             */
    current_time.tm_min = 20;   /* minutes, range 0 to 59           */
    current_time.tm_sec = 0;   /* seconds,  range 0 to 59          */
}

void tearDown(void)
{
    scheduler_clear();
}

void test_scheduler_get_schedule_returns_null_if_id_greater_than_num_schedules(void)
{
    // Add schedule
    ICAL ical_temp;
    ical_get_defaults(&ical_temp);
    scheduler_add(0, &ical_temp); // id: 0

    SCHEDULE *sched = scheduler_get_schedule_by_id(1);

    TEST_ASSERT_NULL(sched);
}

void test_scheduler_get_schedule(void)
{
    // Add schedule
    ICAL ical_temp;
    ical_get_defaults(&ical_temp);
    ical_temp.interval = 20;
    scheduler_add(0, &ical_temp); // id: 0

    SCHEDULE *schedule = scheduler_get_schedule_by_id(0);
    TEST_ASSERT_NOT_NULL(schedule);
    TEST_ASSERT_EQUAL_UINT8(schedule->ical.interval, 20);
}

void test_scheduler_get_next_events_with_multiple_events_disabled(void)
{
    ICAL ical_temp;

    ical_get_defaults(&ical_temp);
    ical_temp.interval = 20;
    scheduler_add(0, &ical_temp); // id: 0
    ical_temp.interval = 10;
    scheduler_add(1, &ical_temp); // id: 1
    ical_temp.interval = 5;
    scheduler_add(1, &ical_temp); // id: 2

    scheduler_update_events(&current_time);
    EVENT* event = scheduler_get_event_by_group(0);
    // No events are enabled so event list should be NULL
    TEST_ASSERT_NULL(event);
}

void test_scheduler_get_next_events_with_multiple_events_enabled(void)
{
    ICAL ical_temp;

    ical_get_defaults(&ical_temp);
    ical_temp.enabled = true;
    ical_temp.interval = 20;
    scheduler_add(0, &ical_temp); // id: 0
    ical_temp.interval = 5;
    scheduler_add(5, &ical_temp); // id: 1
    ical_temp.interval = 15;
    scheduler_add(5, &ical_temp); // id: 2

    scheduler_update_events(&current_time);
    EVENT* event = scheduler_get_event_by_group(5);

    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_UINT8(event->group, 5);
}

void test_scheduler_get_next_events_with_overlapping_groups(void)
{
    ICAL ical_temp;

    ical_get_defaults(&ical_temp);
    ical_temp.enabled = true;
    ical_temp.interval = 20;
    scheduler_add(0, &ical_temp); // id: 0
    ical_temp.interval = 10;
    scheduler_add(2, &ical_temp); // id: 1
    ical_temp.interval = 5;
    scheduler_add(2, &ical_temp); // id: 2
    ical_temp.interval = 3;
    scheduler_add(2, &ical_temp); // id: 3

    scheduler_update_events(&current_time);
    EVENT* event = scheduler_get_event_by_group(2);

    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_UINT8(event->group, 2);
    TEST_ASSERT_EQUAL_UINT8(event->id, 3);
}


