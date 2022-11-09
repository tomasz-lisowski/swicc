# swICC Library
Software ICC (or swICC) is a framework providing an easy and flexible way to develop most types of smart cards. It also allows any swICC-based card to be connected to the PC through the [PC/SC](https://en.wikipedia.org/wiki/PC/SC) interface (using a [software PC/SC reader](https://github.com/tomasz-lisowski/swicc-drv-ifd)) which is the de facto standard for connecting smart cards to PCs.
In summary:
- A framework for developing smart cards with contacts in software, without any hardware dependencies.
- Allows any swICC-based card to be connected to the PC via PC/SC using a [software reader](https://github.com/tomasz-lisowski/swicc-drv-ifd).
- Allows for the smart card file system to be defined using JSON.
- Can save the file system of a card to a proprietary file which can later be loaded back into the card.
- Offers many debug utilities to help parse various data coming in and out of the card.
- Keeps all state in a single struct allowing for many instances to be spawned if needed.
- Offers a BER-TLV implementation which is clear and easy to use (this is useful for forming responses to some instructions).
- Implements a simple proprietary protocol which allows for transport-layer exchanges with the card (unlike PC/SC which operates at the  application-layer).

## Building
The build process requires that `make` and `gcc` are present. Also make sure that you clone the repository recursively so that all sub-modules get cloned as expected.
The make targets are as follows:
- **main**: This builds a static library without any debug information or debug utilities.
- **main-dbg**: This builds a static library with all debug information, debug utilities, and an address sanitizer.
- **test**: Build the testing binary and link it with the non-debug version of the swICC library.
- **test-dbg**: Build the testing binary with debug information and an address sanitizer. and link it with the non-debug version of the swICC library.
- **clean**: Performs a cleanup of the project and all sub-modules.

For adding additional C flags, just pass them inside an `ARG` variable when calling make: `make target ARG="-DDEBUG_CLR -DEXAMPLE_DEFINE"`.

All possible arguments:
- `DEBUG_CLR` to add color to the debug output.

## Instructions
A vanilla (minimal) smart card can be implemented by simply calling the `swicc_net_client` function. This will connect the card to a desired server and handle all requests as a barebones smart card.
To implement a custom card, one also needs to register an APDU demuxer (before running the network client) through `swicc_apduh_pro_register`, as well as APDU handlers that get called by the demuxer depending on command that was received.
A good example for using the framework is the [swSIM](https://github.com/tomasz-lisowski/swsim) project which implements a SIM card using swICC.
