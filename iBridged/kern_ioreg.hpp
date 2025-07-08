//
//  kern_ioreg.hpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#ifndef kern_ioreg_hpp
#define kern_ioreg_hpp

#include "kern_start.hpp"
#include <IOKit/IOPlatformExpert.h>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_nvram.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_efi.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_mach.hpp>
#include <mach/i386/vm_types.h>
#include <libkern/libkern.h>
#include <IOKit/IOLib.h>
#include <sys/sysctl.h>
#include <i386/cpuid.h>

#define MODULE_IOR "IOR"

class IOR {
public:
    static void init(KernelPatcher &Patcher);
    
    static const IBGD::DetectedProcess filteredProcs[];
    
    using _IORegistryEntry_getProperty_t = OSObject * (*)(const IORegistryEntry *that, const OSSymbol *aKey);
    using _IORegistryEntry_getProperty_cstring_t = OSObject * (*)(const IORegistryEntry *that, const char *aKey);
};

#endif /* kern_ioreg_hpp */
