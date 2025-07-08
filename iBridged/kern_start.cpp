//
//  kern_start.cpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#include "kern_start.hpp"
#include "kern_dyld.hpp"
#include "kern_ioreg.hpp"

static IBGD ibgdInstance;
IBGD *IBGD::callbackIBGD;

int IBGD::darwinMajor = 0;
int IBGD::darwinMinor = 0;

void IBGD::onPatcherLoad(void *user __unused, KernelPatcher &Patcher) {
    // Initialise modules here.
    DBGLOG(MODULE_INIT, "Initializing IOR module.");
    IOR::init(Patcher);
    
    DBGLOG(MODULE_INIT, "Initializing DYLD module.");
    DYLD::init(Patcher);
}

void IBGD::init() {
    callbackIBGD = this;
    IBGD::darwinMajor = getKernelVersion();
    IBGD::darwinMinor = getKernelMinorVersion();
    const char* ibgdVersionNumber = IBGD_VERSION;
    DBGLOG(MODULE_INIT, "Hello World from iBridged!");
    DBGLOG(MODULE_INFO, "Current Build Version running: %s", ibgdVersionNumber);
    DBGLOG(MODULE_INFO, "Copyright © 2024, 2025 Carnations Botanica. All rights reserved.");
    if (IBGD::darwinMajor > 0) {
        DBGLOG(MODULE_INFO, "Current Darwin Kernel version: %d.%d", IBGD::darwinMajor, IBGD::darwinMinor);
    } else {
        DBGLOG(MODULE_ERROR, "WARNING: Failed to retrieve Darwin Kernel version.");
    }

    // Begin based on kernel version detected.
    // This is messy because internally, we're debugging each version independently
    if (IBGD::darwinMajor >= KernelVersion::Tahoe) {
        DBGLOG(MODULE_INIT, "Detected macOS Tahoe (26.x) or newer.");
        DBGLOG(MODULE_WARN, "This version has not been verified to work with iBridged!");
        DBGLOG(MODULE_INIT, "Registering PHTM::solveSysCtlChildrenAddr with onPatcherLoadForce anyway.");
        lilu.onPatcherLoadForce(&IBGD::onPatcherLoad);
    } else if (IBGD::darwinMajor >= KernelVersion::Sequoia) {
        DBGLOG(MODULE_INIT, "Detected macOS Sequoia (15.x).");
        DBGLOG(MODULE_INIT, "Registering PHTM::solveSysCtlChildrenAddr with onPatcherLoadForce.");
        lilu.onPatcherLoadForce(&IBGD::onPatcherLoad);
    } else if (IBGD::darwinMajor >= KernelVersion::Sonoma) {
        DBGLOG(MODULE_INIT, "Detected macOS Sonoma (14.x).");
        DBGLOG(MODULE_INIT, "Registering PHTM::solveSysCtlChildrenAddr with onPatcherLoadForce.");
        lilu.onPatcherLoadForce(&IBGD::onPatcherLoad);
    } else {
        DBGLOG(MODULE_ERROR, "Detected an unsupported version of macOS (older than Sonoma).");
        panic(MODULE_LONG, "Detected an unsupported version of macOS (older than Sonoma).");
    }
}

void IBGD::deinit() {
    DBGLOG(MODULE_ERROR, "This kernel extension cannot be disabled this way!");
    SYSLOG(MODULE_ERROR, "This kernel extension cannot be disabled this way!");
}

const char *bootargOff[] {
    "-ibgdoff"
};

const char *bootargDebug[] {
    "-ibgddbg"
};

const char *bootargBeta[] {
    "-ibgdbeta"
};

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal |
    LiluAPI::AllowSafeMode |
    LiluAPI::AllowInstallerRecovery,
    bootargOff,
    arrsize(bootargOff),
    bootargDebug,
    arrsize(bootargDebug),
    bootargBeta,
    arrsize(bootargBeta),
    KernelVersion::Sonoma,
    KernelVersion::Tahoe,
    []() {
        ibgdInstance.init();
    }
};

