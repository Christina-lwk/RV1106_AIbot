#pragma once

typedef struct app {
    const char *name;

    void (*init)(void);
    void (*enter)(void);
    void (*exit)(void);
    void (*loop)(void); //optional
} app_t;

