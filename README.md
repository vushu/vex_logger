# vex_logger

- simple
- tiny
- nothing fancy
- thread safe
- header only logger

copy the vex_logger.h in your project and use, as default all logs are written to /tmp/vex_log.txt;

## How to use

```cpp
// logs functions:
log_info("Hello agent %s", "007");
log_warn("Hello agent %s", "007");
log_debug("Hello agent %s", "007");
log_error("Hello agent %s", "007");

// to append instead of overwritting the log file
log_append(true);
// to set your own output path
log_output("/home/myuser/my_log.txt");

// filter logs
log_filter("info", "error", "warn");

// remove filter if added
log_remove_filter("info", "warn");

```

## Showcase
![](log_example.png)

## TODO:
Filter function

