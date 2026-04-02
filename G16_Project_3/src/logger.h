#ifndef LOGGER_H
#define LOGGER_H

void logger_init(const char *data_dir);
void logger_log(const char *client,
                const char *command,
                const char *target,
                const char *status,
                const char *detail);
const char *logger_path(void);

#endif
