//
//  kern_start.hpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#ifndef kern_start_h
#define kern_start_h

#include <Headers/plugin_start.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <mach/i386/vm_types.h>
#include <libkern/libkern.h>
#include <IOKit/IOLib.h>

#define MODULE_INIT "INIT"
#define MODULE_SHORT "IBGD"
#define MODULE_LONG "iBridged"
#define MODULE_ERROR "ERR"
#define MODULE_WARN "WARN"
#define MODULE_INFO "INFO"
#define MODULE_CUTE "\u2665"

class IBGD {
public:

    /**
     * Maximum number of processes we can track, Maximum length of a process name
     */
    #define MAX_PROCESSES 256
    #define MAX_PROC_NAME_LEN 256

    /**
     * Standard Init and deInit functions
     */
    void init();
    void deinit();
    
    /**
    * Publicly accessible internal build flag
    */
    static const bool IS_INTERNAL;

    /**
     * @brief Globally accessible Darwin kernel version numbers.
     * Populated by IBGD::init().
     */
    static int darwinMajor;
    static int darwinMinor;
    
    /**
    * Struct to hold both process name and potential PID
    */
    struct DetectedProcess {
        const char *name;
        pid_t pid;
    };
    
    
    static void onPatcherLoad(void *user __unused, KernelPatcher &Patcher);
    
private:

    /**
     *  Private self instance for callbacks
     */
    static IBGD *callbackIBGD;

};

#endif /* kern_start_h */
