#include "../include/antidebug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <unistd.h>
    #include <sys/ptrace.h>
    #ifndef PT_DENY_ATTACH
    #define PT_DENY_ATTACH 31
    #endif
#elif defined(__linux__)
    #include <unistd.h>
    #include <sys/ptrace.h>
    #include <sys/types.h>
#endif

void epy_protect_process() {
#if defined(_WIN32)

    if (IsDebuggerPresent()) {
        printf("[!] Anti-Debug: Debugger detected (IsDebuggerPresent).\n");
        exit(1);
    }

    BOOL isDebuggerPresent = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent);
    if (isDebuggerPresent) {
        printf("[!] Anti-Debug: Remote Debugger detected.\n");
        exit(1);
    }

    if (GetModuleHandleA("frida-agent.dll") != NULL) {
        printf("[!] Anti-Debug: Frida detected in memory.\n");
        exit(1);
    }

#elif defined(__APPLE__)

    if (getenv("DYLD_INSERT_LIBRARIES")) {
        printf("[!] Anti-Debug: DYLD_INSERT_LIBRARIES detected.\n");
        exit(1);
    }

    int mib[4];
    struct kinfo_proc info;
    size_t size = sizeof(info);
    
    info.kp_proc.p_flag = 0;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();
    
    if (sysctl(mib, 4, &info, &size, NULL, 0) == 0) {
        if ((info.kp_proc.p_flag & P_TRACED) != 0) {
            printf("[!] Anti-Debug: Debugger attached (sysctl).\n");
            exit(1);
        }
    }
    
    ptrace(PT_DENY_ATTACH, 0, 0, 0);

#elif defined(__linux__)

    if (getenv("LD_PRELOAD")) {
        printf("[!] Anti-Debug: LD_PRELOAD detected.\n");
        exit(1);
    }
    
    if (getenv("LD_AUDIT")) {
        printf("[!] Anti-Debug: LD_AUDIT detected.\n");
        exit(1);
    }

    FILE* fp = fopen("/proc/self/status", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "TracerPid:", 10) == 0) {
                if (atoi(&line[10]) != 0) {
                    printf("[!] Anti-Debug: TracerPid is not 0 (Debugger attached).\n");
                    fclose(fp);
                    exit(1);
                }
            }
        }
        fclose(fp);
    }

    if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) {
        printf("[!] Anti-Debug: ptrace failed (debugger attached or blocked by kernel).\n");
        exit(1);
    }

    fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (
                strstr(line, "frida") || 
                strstr(line, "xposed") || 
                strstr(line, "strace") ||
                strstr(line, "ltrace")
            ) {
                printf("[!] Anti-Debug: Suspicious library loaded in memory.\n");
                fclose(fp);
                exit(1);
            }
        }
        fclose(fp);
    }

#endif
}