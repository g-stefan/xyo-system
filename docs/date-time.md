# Date and time

## DateTime

```cpp
DateTime now;                              // the current local date and time
printf("%04d-%02d-%02d %02d:%02d:%02d.%03d\n",
       now.getYear(), now.getMonth(), now.getDay(),
       now.getHour(), now.getMinute(), now.getSecond(), now.getMilliseconds());
```

A `DateTime` is a local calendar date and time with milliseconds. The
constructor sets it to **now**; the setters change single fields.

| Method | Range |
|--------|-------|
| `getYear()` / `setYear(v)` | e.g. `2026` |
| `getMonth()` / `setMonth(v)` | `1` - `12` |
| `getDay()` / `setDay(v)` | `1` - `31` |
| `getDayOfWeek()` | `0` = Sunday - `6`, computed from the date (`setDayOfWeek` is kept for compatibility; the getter ignores it) |
| `getHour()` / `setHour(v)` | `0` - `23` |
| `getMinute()` / `setMinute(v)`, `getSecond()` / `setSecond(v)` | `0` - `59` |
| `getMilliseconds()` / `setMilliseconds(v)` | `0` - `999` |

Arithmetic and comparison:

| Method | Notes |
|--------|-------|
| `copy(DateTime &in)` | `*this = in` (the class is not copyable) |
| `compare(DateTime &in)` | `-1`, `0`, `1` |
| `minus(DateTime &in)` | `*this -= in`: the result is a **duration**, stored as a date after 1601-01-01 |
| `plus(DateTime &in)` | `*this += in`, where `in` is such a duration |
| `toUnixTime()` / `fromUnixTime(t)` | seconds since 1970-01-01 (see *Local time* below) |
| `static timestampInMilliseconds()` | the current time, same clock as `toUnixTime`, in milliseconds |
| `setLocalTime()` | sets the **system clock** to this value (needs administrator / root; otherwise nothing changes) |

Durations: the origin is 1601-01-01 00:00:00.000 (the Windows `FILETIME`
origin), the same on both platforms. `10:30:15.250 minus 08:00:00.000` is
`1601-01-01 02:30:15.250`; `plus` adds it back.

```cpp
DateTime start;
// ... work
DateTime end;
end.minus(start);                          // end is now the elapsed time
printf("took %d min %d.%03d s\n", end.getMinute(), end.getSecond(), end.getMilliseconds());
```

For measuring elapsed time, `timestampInMilliseconds()` is simpler:

```cpp
uint64_t begin = DateTime::timestampInMilliseconds();
// ... work
printf("took %llu ms\n", (unsigned long long)(DateTime::timestampInMilliseconds() - begin));
```

(It follows the wall clock: a clock change during the measurement shows up
in the result.)

### Local time, not UTC — by design

`toUnixTime`, `fromUnixTime` and `timestampInMilliseconds` count the
**local wall clock as if it were UTC**: no time zone conversion is done.
`fromUnixTime(toUnixTime())` gives back the same local date and time, and
the values are the same on Windows and Linux, independent of time zone and
daylight saving rules. They are meant to store and restore a local date and
time, and to compute differences.

They are therefore **not** the standard Unix timestamp (which is UTC):
do not exchange them with other systems that expect real Unix time, and do
not convert them with `localtime` / `gmtime`.

## FileTime

```cpp
FileTime source;
FileTime target;
if (source.getLastWriteTime("main.cpp") && target.getLastWriteTime("main.o")) {
	if (target.compare(source) < 0) {
		// main.o is older than main.cpp
	};
};
```

| Method | Notes |
|--------|-------|
| `getLastWriteTime(file)` | modification time; `false` if the file can not be read (the object then holds no time) |
| `getLastAccessTime(file)` | last access (often not updated by the OS) |
| `getCreationTime(file)` | Windows: creation time. **Linux: the inode change time** (`st_ctime`), Linux has no portable creation time |
| `compare(x)` | `<0`, `0`, `>0` (only the sign matters). An object without a time is older than one with a time |
| `isChanged(TDynamicArray<FileTime> &sources)` | no time, or older than any of `sources` |
| `toUnixTime()` / `fromUnixTime(t)` | local wall clock as if UTC, same as `DateTime`: `DateTime::fromUnixTime(fileTime.toUnixTime())` is the local date and time of the file |
| `activeDestructor()` | back to "no time" |

The time itself is stored in UTC (nanoseconds on Linux, 100 ns on
Windows), so `compare` is exact across time zone and daylight saving
changes. Links are followed: the time is the time of what the link points
to.

For the common "is the target older than its sources" question,
`Shell::isChanged` and `Shell::compareLastWriteTime` (see
[Files](files.md#out-of-date-checks-build-helpers)) do it in one call.
