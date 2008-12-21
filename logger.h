#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <iostream>

#define LOG(__lvl, __x) \
    do { \
        std::cerr << __FILE__ << ":" << __LINE__ << " " << #__lvl << ": " << __x << std::endl; \
    } while(0)

#define LOG_ERR(__x) LOG(ERR, __x)
#define LOG_WARN(__x) LOG(WARN, __x)
#define LOG_INFO(__x) LOG(INFO, __x)

#endif /* __LOGGER_H__ */
