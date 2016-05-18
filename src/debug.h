#pragma once

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

// Returns true if the current process is being debugged (either 
// running under the debugger or has a debugger attached post facto).
inline bool InDebugger()
{
    int                 mib[4];
    struct kinfo_proc   info;

    // Initialize the flags so that, if sysctl fails for some bizarre 
    // reason, we get a predictable result.
    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.
    auto size = sizeof(info);
#ifndef NDEBUG
    auto junk =
#endif
        sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.
    return ((info.kp_proc.p_flag & P_TRACED) != 0);
}

#if defined(__i386__) || defined(__x86_64__)
#define DEBUG_BREAK() __asm__ __volatile__("int $0x03")
#elif defined(__arm__) || defined(__arm64__)

// References:
// https://www.theiphonewiki.com/wiki/Kernel_Syscalls
// http://iphone.m20.nl/wp/2010/10/xcode-iphone-debugger-halt-assertions/

#ifdef __arm64__
#define DEBUG_BREAK() do {                                             \
    if (InDebugger())                                                  \
    __asm__ __volatile__("mov x0, %0\n\t"                              \
                         "mov x1, %1\n\t"                              \
                         "mov x16, #37\n\t"                            \
                         "svc 128\n\t"                                 \
                         /* outputs */ :                               \
                         /* inputs */ :                                \
                         "r"((int64_t)getpid()), "r"((int64_t)SIGSTOP) \
                         /* clobbered registers */ :                   \
                         "x0", "x1", "x16", "cc");                     \
} while (0)
#else
#define DEBUG_BREAK() do {                                             \
    if (InDebugger())                                                  \
    __asm__ __volatile__("mov r0, %0\n\t"                              \
                         "mov r1, %1\n\t"                              \
                         "mov r12, #37\n\t"                            \
                         "svc 128\n\t"                                 \
                         /* outputs */ :                               \
                         /* inputs */ :                                \
                         "r"(getpid()), "r"(SIGSTOP)                   \
                         /* clobbered registers */ :                   \
                         "r0", "r1", "r12", "cc");                     \
} while (0)
#endif

#else

#define DEBUG_BREAK() do { if (InDebugger()) __builtin_trap(); } while (0)

#endif
