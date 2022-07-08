#include <unity.h>

// pio test

// #include <C:/Users/bvnet/.platformio/lib/SNTPtime/SNTPtime.h>
// SNTPtime ntp;
// StrDateTime now;

// #include <stdio.h>

// void setUp(void) {}
// void tearDown(void) {}

typedef unsigned char byte;

int HourMinToSecs(byte h, byte m)
{
    return ((h * 60) + m) * 60;
}

void TimeAddMin(int &h, int &m, int dm)
{
    // m += dm;

    // int mins = h * 60 + m;
    // mins += dm;
    // h = mins / 60;
    // m = mins % 60;

    int mins = h * 60 + m;
    mins += dm;
    const int DAY_MINUTES = 24 * 60;
    if (mins < 0)
        mins += DAY_MINUTES;
    if (mins > DAY_MINUTES)
        mins -= DAY_MINUTES;
    h = mins / 60;
    m = mins % 60;
}

int autoStartHour, autoStartMin;

void test_HourMinToSecs()
{
    TEST_ASSERT_EQUAL(15 * 60, HourMinToSecs(0, 15));
    TEST_ASSERT_EQUAL(60 * (2 + 1 * 60), HourMinToSecs(1, 02));
}

void TimeAddMin_Add1Min()
{
    autoStartHour = 12;
    autoStartMin = 15;
    TimeAddMin(autoStartHour, autoStartMin, 1);
    TEST_ASSERT_EQUAL(12, autoStartHour);
    TEST_ASSERT_EQUAL(16, autoStartMin);
}

void TimeAddMin_Sub1Min()
{
    autoStartHour = 12;
    autoStartMin = 15;
    TimeAddMin(autoStartHour, autoStartMin, -1);
    TEST_ASSERT_EQUAL(12, autoStartHour);
    TEST_ASSERT_EQUAL(14, autoStartMin);
}

void TimeAddMin_SubPrevHour()
{
    autoStartHour = 10;
    autoStartMin = 0;
    TimeAddMin(autoStartHour, autoStartMin, -2);
    TEST_ASSERT_EQUAL(9, autoStartHour);
    TEST_ASSERT_EQUAL(58, autoStartMin);
}

void TimeAddMin_SubPrevDay()
{
    autoStartHour = 0;
    autoStartMin = 0;
    TimeAddMin(autoStartHour, autoStartMin, -5);
    TEST_ASSERT_EQUAL(23, autoStartHour);
    TEST_ASSERT_EQUAL(55, autoStartMin);
}

void TimeAddMin_SubNextDay()
{
    autoStartHour = 23;
    autoStartMin = 55;
    TimeAddMin(autoStartHour, autoStartMin, 10);
    TEST_ASSERT_EQUAL(0, autoStartHour);
    TEST_ASSERT_EQUAL(5, autoStartMin);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(test_HourMinToSecs);
    RUN_TEST(TimeAddMin_Add1Min);
    RUN_TEST(TimeAddMin_Sub1Min);
    RUN_TEST(TimeAddMin_SubPrevHour);
    RUN_TEST(TimeAddMin_SubPrevDay);
    RUN_TEST(TimeAddMin_SubNextDay);

    UNITY_END();
    return 0;
}
