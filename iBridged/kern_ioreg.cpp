//
//  kern_ioreg.cpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#include "kern_ioreg.hpp"

static IOR::_IORegistryEntry_getProperty_t original_IORegistryEntry_getProperty_os_symbol = nullptr;
static IOR::_IORegistryEntry_getProperty_cstring_t original_IORegistryEntry_getProperty_cstring = nullptr;

const IBGD::DetectedProcess IOR::filteredProcs[] = {
    {"ramrod", 0},
    {"softwareupdated", 0},
    {"com.apple.Mobile", 0},
    {"osinstallersetup", 0},
    {"SoftwareUpdateNo", 0},
    {"launchd", 0}
    
    /* launchd access apple-coprocessor-version for whatever reason. i suspect it has something to do with Cryptexes + libignition. */
};

static bool isProcFiltered(const char *procName) {
    if (!procName) {
        return false;
    }
    const size_t num_filtered = sizeof(IOR::filteredProcs) / sizeof(IOR::filteredProcs[0]);
    for (size_t i = 0; i < num_filtered; ++i) {
        if (strcmp(procName, IOR::filteredProcs[i].name) == 0) {
            return true;
        }
    }
    return false;
}

OSObject *ibgd_IORegistryEntry_getProperty_os_symbol(const IORegistryEntry *that, const OSSymbol *aKey);

// This is the new hook for the getProperty(const char*) variant.
OSObject *ibgd_IORegistryEntry_getProperty_cstring(const IORegistryEntry *that, const char *aKey) {
    
    const OSSymbol *symbolKey = OSSymbol::withCString(aKey);
    OSObject *result = ibgd_IORegistryEntry_getProperty_os_symbol(that, symbolKey);
    
    if (symbolKey) {
        const_cast<OSSymbol *>(symbolKey)->release();
    }
    return result;
    
}


OSObject *ibgd_IORegistryEntry_getProperty_os_symbol(const IORegistryEntry *that, const OSSymbol *aKey) {
    
    // Always call the original function first to get the real property.
    OSObject* original_property = nullptr;
    if (original_IORegistryEntry_getProperty_os_symbol) {
        original_property = original_IORegistryEntry_getProperty_os_symbol(that, aKey);
    }
    
    proc_t p = current_proc();
    pid_t pid = proc_pid(p);
    char procName[MAX_PROC_NAME_LEN];
    proc_name(pid, procName, sizeof(procName));

    const char *keyName = aKey->getCStringNoCopy();
    if (keyName && strcmp(keyName, "apple-coprocessor-version") == 0) {
        DBGLOG(MODULE_IOR, "'%s' (PID: %d) is attempting to access 'apple-coprocessor-version'.",
               procName, pid);
    }

    // Check if the process is one we want to target.
    if (isProcFiltered(procName))
    {
        const char *keyName = aKey->getCStringNoCopy();

        if (keyName && strcmp(keyName, "apple-coprocessor-version") == 0) {
            const char* entryClassName = that->getMetaClass()->getClassName();
            /* oh god oh fuck what am i even doing anymore */
            UInt32 coproc = kCoprocessorVersion2;
            DBGLOG(MODULE_IOR, "'%s' (PID: %d) on class '%s' is spoofing 'apple-coprocessor-version'.",
                   procName, pid, entryClassName);
            return OSData::withBytes(&coproc, sizeof(UInt32));
        }
    }

    // For all other cases, just return the original property.
    return original_property;
}

void IOR::init(KernelPatcher &Patcher) {
    
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) called. IORegistry module is starting.");

    // Route Requests for getProperty
    KernelPatcher::RouteRequest requests[] = {
        { "__ZNK15IORegistryEntry11getPropertyEPK8OSSymbol", ibgd_IORegistryEntry_getProperty_os_symbol, original_IORegistryEntry_getProperty_os_symbol },
        { "__ZNK15IORegistryEntry11getPropertyEPKc", ibgd_IORegistryEntry_getProperty_cstring, original_IORegistryEntry_getProperty_cstring },
    };

    // By default, we only patch the first, most critical function.
    // Perform Safety Checks Before Attempting to Patch Both.
    size_t request_count = 1;
    
    // Resolve the addresses of BOTH functions first.
    mach_vm_address_t addr_os_symbol = Patcher.solveSymbol(KernelPatcher::KernelID, requests[0].symbol);
    mach_vm_address_t addr_cstring = Patcher.solveSymbol(KernelPatcher::KernelID, requests[1].symbol);

    if (!addr_os_symbol || !addr_cstring) {
        DBGLOG(MODULE_ERROR, "Could not resolve one or both getProperty variants.");
        return;
    }

    // Calculate the distance between the two functions in memory.
    const int64_t min_safe_distance = 32;
    int64_t functions_distance = (addr_os_symbol > addr_cstring) ? (addr_os_symbol - addr_cstring) : (addr_cstring - addr_os_symbol);
    DBGLOG(MODULE_IOR, "Distance between getProperty variants is %lld bytes.", functions_distance);
    
    // --- Decide Whether to Patch One or Both Functions ---
    if (functions_distance < min_safe_distance) {
        DBGLOG(MODULE_WARN, "getProperty variants are too close. Patching only one to avoid multiroute panic.");
    } else {
        DBGLOG(MODULE_IOR, "Conditions are safe to patch both getProperty variants.");
        request_count = 2;
    }

    // Perform the reRouting
    if (!Patcher.routeMultipleLong(KernelPatcher::KernelID, requests, request_count)) {
        DBGLOG(MODULE_ERROR, "Failed to apply IORegistry patches.");
        return;
    }
    
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) finished successfully.");
}

