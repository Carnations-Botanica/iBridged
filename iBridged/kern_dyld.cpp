//
//  kern_dyld.cpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#include "kern_dyld.hpp"
#include "kern_start.hpp"

static const UInt8 kBridgeOSInstallBridgeCheckFind15[] = {
    0x48, 0x8d, 0x5d, 0xc0,             // lea     rbx, [rbp-0x40]
    0x48, 0x89, 0xdf,                   // mov     rdi, rbx
    0xe8, 0x00, 0x00, 0x00, 0x00,       // call    _multiverse_platform_get_type
    0x8b, 0x0b,                         // mov     ecx, dword [rbx]
    0x81, 0xf1, 0x00, 0x00, 0x02, 0x00, // xor     ecx, 0x20000
    0x09, 0xc1,                         // or      ecx, eax
    0x41, 0x0f, 0x94, 0xc6              // sete    r14b
};
/* This is the weirdest compiler optimisation ever, but if it makes the signature more unique I'll take it for the sake of dyld patching. */

static const UInt8 kBridgeOSInstallBridgeCheckMask15[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const UInt8 kBridgeOSInstallBridgeCheckReplace15[] = {
    0x48, 0x8d, 0x5d, 0xc0,             // lea     rbx, [rbp-0x40]
    0x48, 0x89, 0xdf,                   // mov     rdi, rbx
    0xe8, 0x00, 0x00, 0x00, 0x00,       // call    _multiverse_platform_get_type
    0x8b, 0x0b,                         // mov     ecx, dword [rbx]
    0x81, 0xf1, 0x00, 0x00, 0x04, 0x00, // xor     ecx, 0x40000
    0x09, 0xc1,                         // or      ecx, eax
    0x41, 0x0f, 0x94, 0xc6              // sete    r14b
};

static const UInt8 kBridgeOSInstallBridgeCheckFind10_14[] = {
    0x48, 0x8d, 0x5d, 0xc0,       // lea     rbx, [rbp-0x40]
    0x48, 0x89, 0xdf,             // mov     rdi, rbx
    0xe8, 0x00, 0x00, 0x00, 0x00, // call    _multiverse_platform_get_type
    0xb9, 0x00, 0x00, 0x02, 0x00, // mov     ecx, 0x20000
    0x33, 0x0b,                   // xor     ecx, dword [rbx]
    0x09, 0xc1,                   // or      ecx, eax
    0x41, 0x0f, 0x94, 0xc6        // sete    r14b
};

static const UInt8 kBridgeOSInstallBridgeCheckMask10_14[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const UInt8 kBridgeOSInstallBridgeCheckReplace10_14[] = {
    0x48, 0x8d, 0x5d, 0xc0,       // lea     rbx, [rbp-0x40]
    0x48, 0x89, 0xdf,             // mov     rdi, rbx
    0xe8, 0x00, 0x00, 0x00, 0x00, // call    _multiverse_platform_get_type
    0xb9, 0x00, 0x00, 0x04, 0x00, // mov     ecx, 0x40000
    0x33, 0x0b,                   // xor     ecx, dword [rbx]
    0x09, 0xc1,                   // or      ecx, eax
    0x41, 0x0f, 0x94, 0xc6        // sete    r14b
};

static DYLD::_cs_validate_page_t _original_cs_validate_page = nullptr;
static DYLD::_cs_validate_range_t _original_cs_validate_range = nullptr;

static void perform_patch(const void *data, size_t size, const char *binPath) {
    /* I would say 'gate this patch' but the installers can be used on any version, I can have a 10.15 system and run the Tahoe installer. */
    if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), size,
                                                        kBridgeOSInstallBridgeCheckFind15, kBridgeOSInstallBridgeCheckMask15,
                                                        kBridgeOSInstallBridgeCheckReplace15, kBridgeOSInstallBridgeCheckMask15, 1, 0))) {
        DBGLOG(MODULE_DYLD, "Performed the bridgeOS Install patch (14.x+) on %s", binPath);
        return;
    }

    /* You can only use the installers for the current OS version and beyond. (eg: i can do Tahoe DP1 from Tahoe DP3, but not Sequoia from Tahoe) */
    if (IBGD::darwinMajor <= KernelVersion::Sonoma) {
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), size,
                                                            kBridgeOSInstallBridgeCheckFind10_14, kBridgeOSInstallBridgeCheckMask10_14,
                                                            kBridgeOSInstallBridgeCheckReplace10_14, kBridgeOSInstallBridgeCheckMask10_14, 1, 0))) {
            DBGLOG(MODULE_DYLD, "Performed the bridgeOS Install patch (10.14+) on %s", binPath);
            return;
        }
    }
}

void ibgd_cs_validate_page(vnode *vp, memory_object_t pager, memory_object_offset_t page_offset,
                           const void *data, int *validated_p, int *tainted_p, int *nx_p) {
    char path[PATH_MAX];
    int pathlen = PATH_MAX;

    _original_cs_validate_page(vp, pager, page_offset, data, validated_p, tainted_p, nx_p);
    
    if (vn_getpath(vp, path, &pathlen)) { return; }

    /*
     * This should hit:
     *  - The dyld cache
     *  - InstallAssistant's BridgeOSInstall variant
     *  - The system copy of BridgeOSInstall
     */
    if (strstr(path, "dyld_shared_cache_x86_64") || strstr(path, "BridgeOSInstall")) {
        /* I have zero idea if this matches the Sonoma binaries. */
        perform_patch(data, PAGE_SIZE, path);
    }
}

void ibgd_cs_validate_range(vnode *vp, memory_object_t pager, memory_object_offset_t offset,
                           const void *data, vm_size_t size, unsigned *result) {
    char path[PATH_MAX];
    int pathlen = PATH_MAX;

    _original_cs_validate_range(vp, pager, offset, data, size, result);
    
    if (vn_getpath(vp, path, &pathlen)) { return; }

    /*
     * This should hit:
     *  - The dyld cache
     *  - InstallAssistant's BridgeOSInstall variant
     *  - The system copy of BridgeOSInstall
     */
    if (strstr(path, "dyld_shared_cache_x86_64") || strstr(path, "BridgeOSInstall")) {
        perform_patch(data, size, path);
    }
}

void DYLD::init(KernelPatcher &Patcher) {
    DBGLOG(MODULE_DYLD, "DYLD::init(Patcher) called. Dyld patcher module is starting.");
    
    if (IBGD::darwinMajor >= KernelVersion::BigSur) {
        KernelPatcher::RouteRequest req = {"_cs_validate_page", ibgd_cs_validate_page, _original_cs_validate_page};
        
        if (!Patcher.routeMultipleLong(KernelPatcher::KernelID, &req, 1)) {
            DBGLOG(MODULE_DYLD, "Failed to route _cs_validate_page");
        }
    } else {
        /* Assume we are Catalina or Mojave. */
        KernelPatcher::RouteRequest req = {"_cs_validate_range", ibgd_cs_validate_range, _original_cs_validate_range};
        
        if (!Patcher.routeMultipleLong(KernelPatcher::KernelID, &req, 1)) {
            DBGLOG(MODULE_DYLD, "Failed to route _cs_validate_range");
        }
    }
}
