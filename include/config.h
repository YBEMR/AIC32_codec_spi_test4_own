// config.h
#if defined(_WIN32) || defined(_WIN64)  // Windows 平台（包括 CCS）
    #define TARGET_PLATFORM_WINDOWS
#elif defined(__linux__)               // Linux 平台
    #define TARGET_PLATFORM_LINUX
#endif
