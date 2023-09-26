#include <swicc/swicc.h>

/**
 * ATR defined according to ISO/IEC 7816-3:2006 clause.8 and ISO/IEC
 * 7816-4:2020 clause.12.2.2.
 */
uint8_t const swicc_atr[] = {
    0b00111011, /**
                 * LSB>MSB
                 * TS  =   L (implicit character start indicator)
                 *       + HHL (synchronization pattern)
                 *       + HHH or LLL (HHH selects 'direct' enc/decoding
                 *                     convention LSB>MSB with no bit inversion)
                 *       + LL
                 *       + H (implicit character parity bit of moments 2 to 10
                 *            i.e. after character start and including this
                 *            moment)
                 * ISO/IEC 7816-3:2006 clause.8.1
                 */

    /**
     * ISO/IEC 7816-3:2006 does a bad job explaining what the Y indicator is.
     * ETSI TS 102 221 V16.4.0 only gives some examples but those do a better
     * job.
     *
     * The 4 bits of Y indicate (from LSB to MSB) what bytes (TA, TB, TC, TD in
     * this order) will be present in the next set of interface bytes (TAi, TBi,
     * TCi, TDi).
     *
     * E.g. 0b0101 = TA, TC are present and TB, TD are not present.
     */

    0b11011111, /**
                 * LSB>MSB
                 * T00 =   4b K  = 15 (number of historical bytes)
                 *       + 4b Y1 = TA1, TC1, and TD1 are present
                 */
    0b10010110, /**
                 * LSB>MSB
                 * TA1 =   4b Di = 32
                 *       + 4b Fi = 512
                 */
    0b00000000, /**
                 * TC1 = N (extra guard time) = 0 (default)
                 */
    0b10010000, /**
                 * LSB>MSB
                 * TD1 =   4b T  = 0 (half-duplex character-based protocol)
                 *       + 4b Y2 = TA2 and TD2 are present
                 */
    0b00010000, /**
                 * LSB>MSB
                 * TA2 =   4b T       = 0 (half-duplex character-based protocol)
                 *       + 1b F and D = 1 (implicit values)
                 *       + 2b RFU     = 0
                 *       + 1b Mode    = 0 (capable to change)
                 */
    0b00111111, /**
                 * LSB>MSB
                 * TD2 =   4b T  = 15
                 *       + 4b Y3 = TA3 and TB3 are present
                 */
    0b00000111, /**
                 * LSB>MSB
                 * TA3 =   6b Y = 7 = A, B, and C (class indicator)
                 *       + 2b X = 0 = Clock stop not supported
                 */
    0b00000000, /**
                 * LSB>MSB
                 * TB3 =   7b SPU Purpose     = 0 (not used)
                 *       + 1b SPU Proprietary = 0 (standard use of SPU)
                 */

    /**
     * Historical bytes are sent as a COMPACT-TLV data object.
     *
     * COMPACT-TLV works by packing the tag and length into a single byte like
     * this: 0x73 which means tag = 7 and length = 3 (bytes in the value field).
     */
    0x80,       /**
                 * Category Indicator = 0x80 (status indicator may be present in
                 *                            a COMPACT-TLV data object)
                 */
    0x31,       /**
                 * Tag = 3 (card service data)
                 * Len = 1
                 */
    0b11100000, /**
                 * LSB>MSB
                 * CSD
                 *  =   1b Card MF = 0 (card with MF)
                 *    + 3b EF.DIR and EF.ATR/INFO access services
                 *      = 0 (by READ RECORD command)
                 *    + 2b DOs available = 2 (in EF.DIR but not in EF.ATR/INFO)
                 *    + 2b Application selection = 3 (by full DF name and
                 *                                    partial DF name)
                 */
    0x67,       /**
                 * Tag = 4 (pre-issuing data)
                 * Len = 7
                 */

    's', 'w', 'i', 'c', 'c', 0x00, 0x00,

    0x73,       /**
                 * Tag = 7 (card capabilities)
                 * Len = 3
                 */
    0b11111110, /**
                 * LSB>MSB
                 * Selection Methods
                 *  =   1b Record ID support     = 0 (no)
                 *    + 1b Record number support = 1 (yes)
                 *    + 1b Short EF ID support   = 1 (yes)
                 *    + 1b Implicit DF selection = 1 (yes)
                 *    + 1b DF by full DF name    = 1 (yes)
                 *    + 1b DF by partial DF name = 1 (yes)
                 *    + 1b DF by path            = 1 (yes)
                 *    + 1b DF by file ID         = 1 (yes)
                 */
    0b00100001, /**
                 * LSB>MSB
                 * Data Coding
                 *  =   4b Data unit size = 1 (2 quartets)
                 *    + 1b First byte of BER-TLV is 'FF' = 0 (invalid)
                 *    + 2b Write behavior = 1 (proprietary)
                 *    + 1b EFs of BER-TLV struct support = 0 (no)
                 */
    0b00000000, /**
                 * LSB>MSB
                 * Command chaining, length fields, and logical channels
                 *  =   1b t = 0
                 *    + 1b z = 0
                 *    + 1b y = 0
                 *    + 2b Logical channel assignment
                 *      = 0 (only basic channel available)
                 *    + 1b Extended length info in EF.ATR/INFO = 0 (no)
                 *    + 1b Extended Lc and Le fields = 0 (no)
                 *    + 1b Command chaining = 0 (no)
                 *
                 * @note Maximum number of logical channels is calculated using
                 * 't', 'z', 'y' with: 4y + 2z + t + 1 (when not all =1, else it
                 * means 8 or more)
                 */
    0x06,       /**
                 * Check byte (TCK)
                 *  = XOR of all bytes (with TCK=0)
                 */
};
