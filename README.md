# swICC Library

> Project **needs** to be cloned recursively. Downloading the ZIP is not enough.

Software ICC (or swICC) is a framework providing an easy and flexible way to develop most types of smart cards. It also allows any swICC-based card to be connected to the PC through the [PC/SC](https://en.wikipedia.org/wiki/PC/SC) interface (using the [swICC PC/SC reader](https://github.com/tomasz-lisowski/swicc-drv-ifd)) which is the de facto standard for connecting smart cards to PCs.

## Scope
- Framework for developing smart cards in software, with no hardware dependencies.
- Any swICC-based card can connect to the PC via PC/SC using the [swICC PC/SC reader](https://github.com/tomasz-lisowski/swicc-drv-ifd).
- Smart card file system can be defined using JSON, examples present in `./test/data/disk`. The FS can be saved to disk as a `.swicc` file, and loaded back into the card.
- Plenty debug utilities.
- Includes an easy-to-use BER-TLV implementation.

## Install
- You need `make`, `cmake`, and `gcc` to compile the project. No extra runtime dependencies.
1. `sudo apt-get install make cmake gcc`
2. `git clone --recurse-submodules git@github.com:tomasz-lisowski/swicc.git`
3. `cd swicc`
4. `make main-dbg` (for more info on building, take a look at `./doc/install.md`).
5. Link with `./build/libswicc.a` (e.g. `-Llib/swicc/build -lswicc`) and add `./include` to the include path (e.g. `-Ilib/swicc/include`).
6. In your project add `#include <swicc/swicc.h>` to include all headers.

## Usage
To create a minimal smart card do the following:
1. Make sure to follow the installation instructions first and make sure `./build/libswicc.a` exists.
2. `mkdir card`
3. `cd card`
4. Copy the following code into a `main.c` file inside the `./card` directory.
<details>
    <summary>Click to see source code.</summary>

```C
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <swicc/swicc.h>

swicc_net_client_st client_ctx = {0U};

static void sig_exit_handler(__attribute__((unused)) int signum)
{
    printf("Shutting down...\n");
    swicc_net_client_destroy(&client_ctx);
    exit(0);
}

int32_t main(int32_t const argc, char const *const argv[const argc])
{
    swicc_disk_st disk = {0U};
    swicc_ret_et ret = swicc_diskjs_disk_create(&disk, "../test/data/disk/006-in.json");
    if (ret == SWICC_RET_SUCCESS)
    {
        swicc_st swicc_ctx = {0U};
        ret = swicc_fs_disk_mount(&swicc_ctx, &disk);
        if (ret == SWICC_RET_SUCCESS)
        {
            ret = swicc_net_client_sig_register(sig_exit_handler);
            if (ret == SWICC_RET_SUCCESS)
            {
                ret =
                    swicc_net_client_create(&client_ctx, "127.0.0.1", "37324");
                if (ret == SWICC_RET_SUCCESS)
                {
                    ret = swicc_net_client(&swicc_ctx, &client_ctx);
                    if (ret != SWICC_RET_SUCCESS)
                    {
                        printf("Failed to run network client.\n");
                    }
                    swicc_net_client_destroy(&client_ctx);
                }
                else
                {
                    printf("Failed to create a client.\n");
                }
            }
            else
            {
                printf("Failed to register signal handler.\n");
            }
            swicc_terminate(&swicc_ctx);
        }
        else
        {
            printf("Failed to mount disk.\n");
            swicc_disk_unload(&disk);
        }
    }
    else
    {
        printf("Failed to create disk.\n");
    }

    return 0;
}
```
</details>

5. `gcc main.c -I../include -L../build -lswicc -o card.elf` to build the card.
6. To interact with the card over PC/SC, you will need to start a swICC card server, e.g., the [swICC PC/SC reader](https://github.com/tomasz-lisowski/swicc-drv-ifd).
7. `./card.elf` which will connect the card to the card reader.
8. `pcsc_scan` (part of the `pcsc-tools` package) will show some details of the card.
9. You can begin interacting with the card through PC/SC as you would with a real card.

To implement a custom card, one needs to register an APDU demuxer (before running the network client) through `swicc_apduh_pro_register`, as well as APDU handlers that get called by the demuxer depending on command that was received. A good example for using the framework in a more advanced way is the [swSIM](https://github.com/tomasz-lisowski/swsim) project which implements a SIM card using swICC.
