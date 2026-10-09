#pragma once
#include <ctime>
#include <cstdint>
typedef struct { uint8_t Second, Minute, Hour, Wday, Day, Month, Year; } tmElements_t;
inline void breakTime(time_t t, tmElements_t &tm) { struct tm g; gmtime_r(&t, &g);
  tm.Second=g.tm_sec; tm.Minute=g.tm_min; tm.Hour=g.tm_hour; tm.Day=g.tm_mday; tm.Month=g.tm_mon+1; tm.Year=g.tm_year-70; tm.Wday=g.tm_wday+1; }
inline time_t makeTime(const tmElements_t &tm) { struct tm g{}; g.tm_sec=tm.Second; g.tm_min=tm.Minute; g.tm_hour=tm.Hour; g.tm_mday=tm.Day; g.tm_mon=tm.Month-1; g.tm_year=tm.Year+70; return timegm(&g); }
inline int hour(time_t t) { return (t % 86400) / 3600; }
inline int minute(time_t t) { return (t % 3600) / 60; }
