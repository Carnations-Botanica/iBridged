<h1 align="center">iBridged</h1>

<h5 align="center">Bridging the gap between you and your OTAs.</h5>
</br>

A [Lilu](https://github.com/acidanthera/Lilu) plug-in that patches Apple's `BridgeOSInstall` framework and injects the `apple-coprocessor-version` variable.

</br>
<h1 align="center">Usage / Features</h1>
</br>

### Usage

**To use iBridged, you must be using [the latest version of Lilu](https://github.com/acidanthera/Lilu/releases) (atleast 1.7.0+ required) to properly load the plug-in.**

1. Download the latest RELEASE or DEBUG from the Releases tab.
3. Drag/Drop into your OpenCore's Kexts folder.
4. Use ProperTree to OC Snapshot and add the kext to your config.plist
5. Boot macOS, verify the kernel extension is loaded with ``kextstat``.

<h1 align="center">Contributing to the Project</h1>

<h4 align="center">If you have any changes or improvements you'd like to contribute for review and merge, to update conventional mistakes or for QoL, as well as maybe even adding whole new features, you can follow the general outline below to get a local copy of the source building.</h4>

</br>

1. Install/Update ``Xcode``
    - Visit https://xcodereleases.com/ for your appropriate latest version.

2. Prepare source code
    - ``git clone --recursive https://github.com/Carnations-Botanica/iBridged.git``
    - Get the latest ``DEBUG`` Lilu.kext from [Releases](https://github.com/acidanthera/Lilu/releases) and update your EFI with it. Example Repository contents below.
        - iBridged/iBridged.xcodeproj <- Xcode Project file.
        - iBridged/iBridged/ <- Project Contents.
        - iBridged/MacKernelSDK <- Gotten by using ``--recursive``.
        - iBridged/Lilu <- Gotten by using ``--recursive``.
        - iBridged/README.md <- How you can tell you're in the root.

3. Launch ``.xcodeproj`` with Xcode to begin!
    - ``kern_start.cpp`` - Contains main Orchestrator for initializing various modules of reroutes.
    - ``kern_start.hpp`` - Header for Main, sets up various macros and globals and the IBGD class.
    - ``kern_ioreg.cpp`` - Injects the `apple-coprocessor-version` property for various update related services
    - ``kern_ioreg.hpp`` - Header for IOR module.
    - ``kern_dyld.cpp`` - Patches the full installer's frameworks and the system copies.
    - ``kern_dyld.hpp`` - Header for the DYLD module.
    

<br>
<h1 align="center">Special Thanks!</h1>
<br>

[<b>Zormeister</b>](https://github.com/Zormeister) - Lead Developer.

[<b>RoyalGraphX</b>](https://github.com/RoyalGraphX) - Developer of Phantom, which was used for early experiments alongside being a foundation for iBridged.

[<b>Lilu</b>](https://github.com/acidanthera/Lilu) - The patching engine that makes this kernel extension possible.

[<b>MacKernelSDK</b>](https://github.com/acidanthera/MacKernelSDK) - An amazing SDK used to standardize the usage of various kernel APIs across multiple versions of OS X / macOS. Makes this kernel extension possible.

<h6 align="center">A big thanks to all contributors and future contributors! ꩓</h6>
