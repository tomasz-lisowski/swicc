#include "uicc.h"

/**
 * ATR defined according to ISO 7816-3:2006 p.15-20 sec.8
 */
uint8_t const uicc_atr[UICC_ATR_LEN] = {
    0b00111011, /**
                 * LSB>MSB
                 * TS =   L (implicit character start indicator)
                 *      + HHL (synchronization pattern)
                 *      + HHH or LLL (HHH selects 'direct' enc/decoding
                 *                    convention LSB>MSB with no bit inversion)
                 *      + LL
                 *      + H (implicit character parity bit of moments 2 to 10
                 *           i.e. after character start and including this
                 *           moment)
                 * ISO 7816-3:2006 p.15 sec.8.1
                 */

    /**
     * ISO 7816-3:2006 does a bad job explaining what the Y indicator is. ETSI
     * TS 102 221 V16.4.0 only gives some examples but those do a better job.
     *
     * The 4 bits of Y indicate (from LSB to MSB) what bytes (TA, TB, TC, TD in
     * this order) will be present in the next set of interface bytes (TAi, TBi,
     * TCi, TDi).
     *
     * E.g. 0b0101 = TA, TC are present and TB, TD are not present.
     */

    0b10010000, /**
                 * LSB>MSB
                 * T0 =   4b K  = 0 = number of 'historical bytes' (if any)
                 *      + 4b Y1 = TA1 and TD1 are present
                 */
    0b00010001, /**
                 * LSB>MSB
                 * TA1 =   4b Di = 1   (default)
                 *       + 4b Fi = 372 (default)
                 */
    0b10000000, /**
                 * LSB>MSB
                 * TD1 =   4b T  = 0 = Half-duplex character-based protocol
                 *       + 4b Y2 = TD2 is present
                 *
                 * (TODO: Look into TC2 which encodes WI i.e. the waiting time
                 *  integer)
                 */

    0b10010000, /**
                 * LSB>MSB
                 * TD2 =   4b T  = 0
                 *       + 4b Y3 = TA3 and TD3 are present
                 */

    0b00000000, /**
                 * LSB>MSB
                 * TA3 =   4b T = 0 = Half-duplex character-based protocol
                 *       + 1b Fi/Di Definition = 0 = as defined in TA1
                 *       + 2b Reserved = 0
                 *       + 1b Ability to change nego/spec modes = 0 = capable
                 */
    0b00010001, /**
                 * LSB>MSB
                 * TD3 =   4b T = 1 = Half-duplex block-based protocol
                 *       + 4b Y4 = TA4 is present
                 */

    0b00000000, /**
                 * LSB>MSB
                 * TA4 =   4b T = 1 = Half-duplex block-based protocol
                 *       + 1b Fi/Di Definition = 0 = as defined in TA1
                 *       + 2b Reserved = 0
                 *       + 1b Ability to change nego/spec modes = 0 = capable
                 */
};
