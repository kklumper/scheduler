#include "ical.h"
#include "unity.h"
#include <time.h>
#include <stdlib.h>

#define ONE_MIN  60
#define ONE_HOUR 60*ONE_MIN
#define ONE_DAY  24*ONE_HOUR

static struct tm t_start, t_end, t_now, t_next;
static ICAL ical;

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
    // Set TZ variable to no time zone for testing purposes
    // This allows ical to behave as it would on an embedded
    // system
    setenv("TZ", "UTC0", 1);

    ical_get_defaults(&ical);
    ical_set_time_struct(&t_start, 2016, 10, 24, 8, 0, 0);
    ical_set_time_struct(&t_end, 2018, 10, 24, 16, 0, 0);
    ical_set_time_struct(&t_now, 2016, 10, 24, 16, 57, 0);

    ical.t_start = t_start;
    ical.t_end = t_end;
    ical.enabled = true;
    ical.freq = MINUTELY;
    ical.interval = 5;
    ical.byday = SU|MO|TU|WE|TH|FR|SA;
}

void tearDown(void)
{

}

void test_daylight_savings_isnt_changing_time(void)
{
    struct tm * t_test;
    ical_set_time_struct(&t_now, 2016, 11, 5, 8, 0, 0);
    time_t e_now = mktime(&t_now);
    e_now += ONE_DAY;
    t_test = localtime(&e_now);

    assert_test_time(t_test, 2016, 11, 6, 8, 0, 0);
}


void test_ical_returns_no_event_when_disabled(void)
{
    ical.enabled = false;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_NONE, event);
}

void test_ical_returns_start_event_when_before_start_datetime(void)
{
    ical_set_time_struct(&t_now, 2016, 9, 24, 9, 57, 0);

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 24, 8, 0, 0);
}

void test_ical_returns_no_event_when_after_end_datetime(void)
{
    ical_set_time_struct(&t_now, 2018, 10, 25, 9, 57, 0);

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_NONE, event);
}

void test_ical_returns_start_event_when_freq_is_limits(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 7, 57, 0);
    ical.freq = LIMITS;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 24, 8, 0, 0);
}

void test_ical_returns_end_event_when_freq_is_limits(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 8, 57, 0);
    ical.freq = LIMITS;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_END, event);
    assert_test_time(&t_next, 2016, 10, 24, 16, 0, 0);
}

void test_ical_returns_start_event_when_start_time_equal_to_end_time(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 8, 57, 0);
    ical_set_time_struct(&t_start, 2016, 10, 24, 8, 0, 0);
    ical_set_time_struct(&t_end, 2018, 10, 24, 8, 0, 0);
    ical.t_start = t_start;
    ical.t_end = t_end;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 25, 8, 0, 0);
}

void test_ical_returns_start_event_when_freq_is_minutely(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 7, 55, 0);
    ical.freq = MINUTELY;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 24, 8, 0, 0);
}

void test_ical_returns_recur_event_when_freq_is_minutely(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 8, 55, 0);
    ical.freq = MINUTELY;
    ical.interval = 3;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 8, 57, 0);
}

void test_ical_returns_recur_event_for_overnight_schedule_on_night_of(void)
{
    ical_set_time_struct(&t_start, 2016, 10, 24, 23, 0, 0);
    ical_set_time_struct(&t_end, 2018, 10, 24, 4, 0, 0);
    ical_set_time_struct(&t_now, 2016, 10, 24, 23, 59, 59);
    ical.freq = MINUTELY;
    ical.interval = 7;
    ical.t_start = t_start;
    ical.t_end = t_end;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 25, 0, 3, 0);
}

void test_ical_returns_recur_event_for_overnight_schedule_on_morning_of(void)
{
    ical_set_time_struct(&t_start, 2016, 10, 24, 23, 0, 0);
    ical_set_time_struct(&t_end, 2018, 10, 24, 4, 0, 0);
    ical_set_time_struct(&t_now, 2016, 10, 25, 1, 0, 0);
    ical.freq = MINUTELY;
    ical.interval = 7;
    ical.t_start = t_start;
    ical.t_end = t_end;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 25, 1, 6, 0);
}

void test_ical_returns_next_days_event_when_interval_surpasses_end_time(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 15, 59, 0);
    ical.freq = MINUTELY;
    ical.interval = 7;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 25, 8, 0, 0);
}

void test_ical_returns_no_event_when_interval_surpasses_end_time(void)
{
    ical_set_time_struct(&t_now, 2018, 10, 24, 15, 59, 0);
    ical.freq = MINUTELY;
    ical.interval = 7;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_NONE, event);
}

void test_ical_skips_day_with_byday_mask(void)
{
    ical_set_time_struct(&t_now, 2016, 11, 10, 4, 25, 0);
    ical.byday = MO|WE;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 11, 14, 8, 0, 0);
}

void test_ical_returns_error_when_start_datetime_greater_than_end_datetime(void)
{
    ical.t_start = t_end;
    ical.t_end = t_start;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALERROR_START_GREATER_THAN_END, event);
}

void test_ical_returns_invalid_byday_when_byday_equals_zero(void)
{
    ical.byday = 0;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALERROR_INVALID_BYDAY, event);
}

void test_ical_returns_invalid_interval_when_interval_equals_zero(void)
{
    ical.interval = 0;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALERROR_INVALID_INTERVAL, event);
}

void test_ical_returns_invalid_interval_when_interval_greater_than_24_and_freq_equals_hourly(void)
{
    ical.freq = HOURLY;
    ical.interval = 25;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALERROR_INVALID_INTERVAL, event);
}

void test_ical_returns_invalid_freq_when_freq_greater_than_four(void)
{
    ical.freq = 4;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALERROR_INVALID_FREQ, event);
}

void test_ical_get_next_secondly_event(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 15, 57, 0);
    ical.freq = SECONDLY;
    ical.interval = 30;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 15, 57, 30);
}

void test_ical_get_next_secondly_event_interval_surpasses_end_time(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 15, 57, 0);
    ical.freq = SECONDLY;
    ical.interval = 255;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 25, 8, 0, 0);
}

void test_ical_get_next_minutely_event(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 9, 46, 0);
    ical.freq = MINUTELY;
    ical.interval = 12;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 9, 48, 0);
}

void test_ical_get_next_minutely_event_interval_surpasses_end_time(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 12, 57, 0);
    ical.freq = MINUTELY;
    ical.interval = 255;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 25, 8, 0, 0);
}

void test_ical_get_next_hourly_event(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 9, 46, 0);
    ical.freq = HOURLY;
    ical.interval = 3;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 11, 0, 0);
}

void test_ical_get_next_hourly_event_interval_surpasses_end_time(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 12, 57, 0);
    ical.freq = HOURLY;
    ical.interval = 10;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 25, 8, 0, 0);
}

void test_ical_get_next_secondly_event_with_byday_restriction(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 15, 57, 0);
    ical.freq = SECONDLY;
    ical.interval = 10;
    ical.byday = WE;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 26, 8, 0, 0);
}

void test_ical_multi_minute_rollover_by_secondly_freq(void)
{
    // This test checks if next event can handle rolling over
    // to multiple minutes
    ical_set_time_struct(&t_now, 2016, 10, 24, 9, 55, 0);
    ical.freq = SECONDLY;
    ical.interval = 241;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 9, 56, 29);
}

void test_ical_multi_hour_rollover_by_minutely_freq(void)
{
    // This test checks if next event can handle rolling over
    // to multiple hours
    ical_set_time_struct(&t_now, 2016, 10, 24, 9, 55, 0);
    ical.freq = MINUTELY;
    ical.interval = 255;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 12, 15, 0);
}

void test_ical_month_rollover_by_interval(void)
{
    // This test checks if next event can handle rolling over
    // to next month
    ical_set_time_struct(&t_now, 2016, 11, 30, 15, 55, 0);
    ical.freq = MINUTELY;
    ical.interval = 13;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 12, 1, 8, 0, 0);
}

void test_ical_year_rollover_by_interval(void)
{
    // This test checks if next event can handle rolling over
    // to next year
    ical_set_time_struct(&t_now, 2016, 12, 31, 15, 55, 0);
    ical.freq = HOURLY;
    ical.interval = 5;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2017, 1, 1, 8, 0, 0);
}

void test_ical_year_rollover_by_byday_mask(void)
{
    // This test checks if next event can handle rolling over
    // to next year
    ical_set_time_struct(&t_now, 2016, 12, 26, 15, 55, 0);
    ical.freq = HOURLY;
    ical.interval = 5;
    ical.byday = MO;
    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);

    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2017, 1, 2, 8, 0, 0);
}

void test_ical_returns_start_event_when_count_is_used(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 7, 57, 0);
    ical.freq = MINUTELY;
    ical.count = 1;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_START, event);
    assert_test_time(&t_next, 2016, 10, 24, 8, 0, 0);
}

void test_ical_returns_no_event_when_count_is_used(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 8, 0, 0);
    ical.count = 1;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_NONE, event);

    ical_set_time_struct(&t_now, 2016, 10, 24, 8, 49, 0);
    ical.freq = MINUTELY;
    ical.interval = 1;
    ical.count = 50;

    event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_NONE, event);
}

void test_ical_returns_recur_event_when_count_is_used(void)
{
    ical_set_time_struct(&t_now, 2016, 10, 24, 9, 55, 0);
    ical.freq = HOURLY;
    ical.interval = 1;
    ical.count = 3;

    ICALEVENT event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 10, 0, 0);

    ical_set_time_struct(&t_now, 2016, 10, 24, 8, 48, 0);
    ical.freq = MINUTELY;
    ical.interval = 1;
    ical.count = 50;

    event = ical_find_next_event(&ical, &t_now, &t_next);
    TEST_ASSERT_EQUAL_HEX16(ICALEVENT_RECUR, event);
    assert_test_time(&t_next, 2016, 10, 24, 8, 49, 0);
}