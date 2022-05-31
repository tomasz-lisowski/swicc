#include "uicc/common.h"
#include "uicc/io.h"

#pragma once
/**
 * FSM (finite state machine) for the SIM state machine described in
 * ISO 7816-3:2006 p.11 sec.6
 */

/**
 * @brief A FSM state handler is defined as this function it must handle the
 * incoming RX/TX data/contact state then respond by writing to the RX and TX
 * buffer (and setting their lengths) and setting contact states.
 * @param uicc_state
 * @note These functions always succeed. Failures/errors while handling lead to
 * just another transition.
 */
typedef void uicc_fsmh_ft(uicc_st *const uicc_state);

typedef enum uicc_fsm_state_e
{
    /**
     * SIM is not connected (mechanically) to any device or the device has not
     * begun using it by applying signals to the contacts.
     */
    UICC_FSM_STATE_OFF,

    /**
     * At this point the card must be mechanically connected and activated
     * according to a class of operating conditions (COC).
     *
     * Class A = 5V, B = 3V, C = 1.8V. More than one class can be supported but
     * the classes need to be consecutive (no specification on the tested
     * order?). ISO 7816-3:2006 p.6 sec.5.1.3.
     *
     * Contacts must be setup like so: RST=L, VCC=ON, I/O=H (reception mode),
     * CLK=ON. ISO 7816-3:2006 p.9 sec.6.2.1.
     */
    UICC_FSM_STATE_ACTIVATION,

    /**
     * Device is in the activation state.
     * 1. Card sets I/O state to H within 200 ticks of receiving the clock.
     * 2. RST will be set L for at least 400 ticks.
     * 3. then it is set back to H.
     * ISO 7816-3:2006 p.10 sec.6.2.2
     */
    UICC_FSM_STATE_RESET_COLD,

    /**
     * Continutation of the cold reset state.
     * 4. Once RST goes from L to H, the card must send the Answer-To-Reset
     * (ATR) within 400-40,000 ticks.
     * 5. If card does not send ATR, interface performs deactivation.
     * ISO 7816-3:2006 p.10 sec.6.2.2
     *
     * When the ATR sent by card contains the class indicator of the class
     * applied by the interface then this means the correct COC is recognized by
     * both sides. If there is a mismatch, interface will wait 10ms and perform
     * a change of COC and a cold reset after deactivation.
     * If the ATR contains no class indication, then the current class is used.
     * Once selected a COC, it should not be changed by an interface until after
     * a deactivation according to ISO 7816-3:2006 p.11 sec.6.2.4.
     */
    UICC_FSM_STATE_ATR_REQ,

    /**
     * Card is sending the ATR response.
     * From here the card can fall into 2 states:
     *
     * 1. After the COC is determined to be good and TA2 is absent in ATR, card
     * is in a specific mode and a specific transmission protocol (specific
     * values) is used. ISO 7816-3:2006 p.11 sec.6.3.1
     *
     * 2. After the COC is determined to be good and TA2 is present in ATR, card
     * is in negotiable mode and the interface can either begin a PPS exchange
     * (default values) or use the first offered transmission protocol
     * (default values). ISO 7816-3:2006 p.11 sec.6.3.1
     */
    UICC_FSM_STATE_ATR_RES,

    /**
     * Device must be in a cold reset or during the sending of the ATR response
     * (after TS and T0 are received and 4,464 ticks have elapsed by the
     * interface according to ISO 7816-3:2006 p.10 sec.6.2.3).
     * 1. Initiated by interface setting RST to L for at least 400 ticks.
     * 2. Card sets I/O to H (reception mode) within 200 ticks.
     * 3. Interface sets RST to H and after 400-40,000 ticks, card will send the
     * ATR.
     * 4. If card does not send ATR, interface performs deactivation.
     * ISO 7816-3:2006 p.10 sec.6.2.3
     */
    UICC_FSM_STATE_RESET_WARM,

    /**
     * After card sent an ATR with an absent TA2, the interface can send first
     * byte 0xFF (=PPSS which is invalid CLA (T=0) and NAD (T=1)) to indicate a
     * initiate a PPS exchange done using a default transmission protocol with
     * default values.
     * ISO 7816-3:2006 p.11-12 sec.6.3.1
     */
    UICC_FSM_STATE_PPS_REQ,

    /**
     * Based on the previous transitions and what has been received, one of 3
     * options for the transmission protocol have been chosen.
     * 1. Specific tranmission protocol with specific values.
     * 2. First offered (by card) tranmission protocol with default values.
     * 3. Negotiated transmission protocol with negotiated values.
     * ISO 7816-3:2006 p.11 sec.6.3.1
     *
     * Card is ready for receiving commands so it waits for the interface to
     * send one.
     */
    UICC_FSM_STATE_CMD_WAIT,

    /**
     * Send a procedure byte to the interface. This can occur after the header
     * has been received and handled or when data of a command is requested in
     * parts thus leading to a back and forth exchange of data and procedure
     * bytes that request more data.
     */
    UICC_FSM_STATE_CMD_PROCEDURE,

    /**
     * When data is expected from the interface, this state is reached so it is
     * not reached for messages without data or if the message failed early
     * after just the header.
     */
    UICC_FSM_STATE_CMD_DATA,
} uicc_fsm_state_et;

/**
 * @brief Given an IO state update, perform the correct state transition or
 * remain in the same one.
 * @param uicc_state
 */
void uicc_fsm(uicc_st *const uicc_state);
