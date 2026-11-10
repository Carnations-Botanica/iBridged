//
//  kern_ioreg.cpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#include "kern_ioreg.hpp"

static IOR::_IORegistryEntry_getProperty_t original_IORegistryEntry_getProperty_os_symbol = nullptr;

const IBGD::DetectedProcess IOR::filteredProcs[] = {
    {"ramrod", 0},
    {"softwareupdated", 0},
    {"com.apple.Mobile", 0},
    {"osinstallersetup", 0},
    {"SoftwareUpdateNo", 0},
    {"launchd", 0}
    
    /* launchd accesses apple-coprocessor-version for whatever reason. i suspect it has something to do with Cryptexes + libignition. */
};

/*
 * Other processes known to try and access apple-coprocessor-version:
 * - systemsoundserverd (probably for the iBridge Audio communication, see BridgeAudioCommunication.kext + BridgeAudioController.kext)
 * - AppleIDSettings
 * - akd
 *
 * In-Kernel clients:
 * - AppleKeyStore
 * - AppleCredentialManager
 * - AppleImage4 (this specifically checks for the T2 to read the set Security Policy)
 */

/*
 * How did we get here?
 *
 * The OTA process in macOS has sort of buggered itself for the weird configuration of Hackintosh systems using ASB.
 *
 * The Image4 library, be it `com.apple.security.AppleImage4` (/System/Library/Extensions/AppleImage4.kext) or the userspace
 * library (`libimg4.dylib` or `libimage4.dylib`) has a set of Image4 'chips'
 *
 * These 'chip' structures provide runtime data on the security policy of the Mac, alongside parameters for Image4 decoding.
 *
 * Validation of Image4 files require the parameters to match the device; which is where things get weird on Hackintoshes.
 *
 * As of some unknown version of macOS, Apple has four x86 'chip' types when evaluating non-cryptex Image4 files:
 * - img4_chip_x86_ap (Equivalent to the Full Security setting)
 * - img4_chip_x86_ap_medium (Equivalent to the Medium Security setting)
 * - img4_chip_x86_ap_void (Equivalent to the No Security setting)
 * - img4_chip_x86 (x86legacyap)
 *
 * AppleImage4/libimg4/libimage4 uses the `apple-coprocessor-version` variable to determine which Image4 'chip' to use, defaulting to
 * `img4_chip_x86` when no 
 *
 * When sealing the system volume, patchd uses a root hash encased in an Image4 file, which is ran through the function `img4_firmware_execute`
 * On systems using Apple Secure Boot, this would break the entire OTA process if RestrictEvents' SBVMM was not used.
 *
 * The specific log is as follows:
 * patchd: patchd_store_failure_info(879): setting ota-failure-reason=MSU 1130 (Failed to verify against canonical metadata);NSPOSIXErrorDomain 13 (img4_firmware_execute failed);
 *
 * Thankfully, XNU provides headers for the Image4 library in the EXTERNAL_HEADERS folder.
 *
 * Code 13 is actually a POSIX errno_t code, EACCES, which the header documents this code meaning that 
 * 'The given chip does not specify the constraints of the attached manifest', indicating a mismatch in the Image4 'chip'
 *
 * And thus is why this kext was created.
 *
 * --- ADDENDUM (13/02/2026) ---
 * Somewhere along the line, macOS also uses the iBridge TargetType (jXXXap) to identify the Mac on an update level.
 * Tahoe, Sequoia and maybe earlier will fail on T2 SMBIOSes because it will report the <Model>X,Y value instead of the iBridge model.
 * As a result of directly interfereing with softwareupdated, it also restores updating on T2 SMBIOSes without needing SBVMM too.
 * The outcome is that OTAs can be completed without hassle on systems that don't do executable patching (eg: BlueToolFixup).
 * Since softwareupdated is usually loaded early on, and is the primary source for updating, the chance of a patched dyld page being re-checked for a valid
 * signature is unlikely, so it's a one-size fits all solution, I suppose. So I doubt we'll face crashes from the code signature enforcer crashing anything that uses
 * BridgeOSInstall.framework in OS versions > 14.x, so no AppleSystemInfo.framework type crashes. I'm open to being proven wrong.
 * I... didn't expect this? I guess. It's a win, I suppose.
 */

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

    // Route Request for getProperty(const OSSymbol*) only.
    // In XNU, getProperty(const char*) delegates directly to getProperty(const OSSymbol*).
    // Hooking only the OSSymbol variant avoids conflicting with WhateverGreen's getProperty(const char*) hook on AMD GPUs.
    KernelPatcher::RouteRequest request = {
        "__ZNK15IORegistryEntry11getPropertyEPK8OSSymbol",
        ibgd_IORegistryEntry_getProperty_os_symbol,
        original_IORegistryEntry_getProperty_os_symbol
    };

    if (!Patcher.routeMultipleLong(KernelPatcher::KernelID, &request, 1)) {
        DBGLOG(MODULE_ERROR, "Failed to apply IORegistry patches.");
        return;
    }
    
    DBGLOG(MODULE_IOR, "IOR::init(Patcher) finished successfully.");
}

