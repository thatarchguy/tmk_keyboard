#include "keymap_common.h"

#define KEYMAP_BLE60( \
    K00, K01, K02, K03, K04, K05, K06, K07, K08, K09, K0A, K0B, K0C, K0D, \
    K10, K11, K12, K13, K14, K15, K16, K17, K18, K19, K1A, K1B, K1C, K1D, \
    K20, K21, K22, K23, K24, K25, K26, K27, K28, K29, K2A, K2B,      K2D, \
    K30,      K32, K33, K34, K35, K36, K37, K38, K39, K3A, K3B, K3C, K3D, \
    K40, K41, K42,           K45,                K49, K4A, K4B, K4C, K4D  \
) { \
    { KC_##K00, KC_##K01, KC_##K02, KC_##K03, KC_##K04, KC_##K05, KC_##K06, KC_##K07, KC_##K08, KC_##K09, KC_##K0A, KC_##K0B, KC_##K0C, KC_##K0D }, \
    { KC_##K10, KC_##K11, KC_##K12, KC_##K13, KC_##K14, KC_##K15, KC_##K16, KC_##K17, KC_##K18, KC_##K19, KC_##K1A, KC_##K1B, KC_##K1C, KC_##K1D }, \
    { KC_##K20, KC_##K21, KC_##K22, KC_##K23, KC_##K24, KC_##K25, KC_##K26, KC_##K27, KC_##K28, KC_##K29, KC_##K2A, KC_##K2B, KC_NO,    KC_##K2D }, \
    { KC_##K30, KC_NO,    KC_##K32, KC_##K33, KC_##K34, KC_##K35, KC_##K36, KC_##K37, KC_##K38, KC_##K39, KC_##K3A, KC_##K3B, KC_##K3C, KC_##K3D }, \
    { KC_##K40, KC_##K41, KC_##K42, KC_NO,    KC_NO,    KC_##K45, KC_NO,    KC_NO,    KC_NO,    KC_##K49, KC_##K4A, KC_##K4B, KC_##K4C, KC_##K4D }  \
}

const uint8_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* Layer:0 */
    KEYMAP_BLE60( 
        FN0,  1,    2,    3,    4,    5,    6,    7,    8,    9,    0,    MINS, EQL,  DEL,  \
        TAB,  Q,    W,    E,    R,    T,    Y,    U,    I,    O,    P,    LBRC, RBRC, BSLS, \
        CAPS, A,    S,    D,    F,    G,    H,    J,    K,    L,    SCLN, QUOT,       ENT,  \
        LSFT,       Z,    X,    C,    V,    B,    N,    M,    COMM, DOT,  UP,   SLSH, RSFT, \
        LCTRL,FN1,  LALT,             SPC,                    BSPC, LEFT, DOWN, RGHT, RCTRL \
        ),

    /* Layer:1 */
    KEYMAP_BLE60( 
        GRV,  F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,  F11,  F12,  DEL,  \
        FN7, FN8, UP,   FN9, TRNS, TRNS, CALC, TRNS, INS,  TRNS, PSCR, SLCK, PAUS, FN10, \
        TRNS, LEFT, DOWN, RGHT, TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, HOME, PGUP,       TRNS, \
        TRNS,       TRNS, APP,  FN2,  FN3,  FN4,  VOLD, VOLU, MUTE, END,  PGDN, TRNS, TRNS, \
        TRNS, TRNS, TRNS,             FN5,                    TRNS, TRNS, TRNS, TRNS, TRNS  \
        ),

    /* Layer:2 */
    KEYMAP_BLE60( 
        TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, P7,   P8,   P9,   TRNS, PMNS, TRNS, TRNS, \
        TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, P4,   P5,   P6,   TRNS, PENT, TRNS, TRNS, \
        TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, P1,   P2,   P3,   PPLS, PAST,       TRNS, \
        TRNS,       TRNS, TRNS, TRNS, TRNS, TRNS, TRNS, P0,   PCMM, PDOT, PSLS, TRNS, TRNS, \
        TRNS, TRNS, TRNS,             FN6,                    TRNS, TRNS, TRNS, TRNS, TRNS  \
        ),

};

/* id for user defined function */
enum function_id {
    SHIFT_ESC,
};

#define MODS_CTRL_MASK  (MOD_BIT(KC_LSHIFT)|MOD_BIT(KC_RSHIFT))
void action_function(keyrecord_t *record, uint8_t id, uint8_t opt)
{
    static uint8_t shift_esc_shift_mask;

    switch (id) {
        case SHIFT_ESC:
            shift_esc_shift_mask = get_mods()&MODS_CTRL_MASK;
            if (record->event.pressed) {
                if (shift_esc_shift_mask) {
                    add_key(KC_GRV);
                } else {
                    add_key(KC_ESC);
                }
            } else {
              if (shift_esc_shift_mask) {
                    del_key(KC_GRV);
                } else {
                    del_key(KC_ESC);
                }
            }
            send_keyboard_report();
            break;
    }
}

const uint16_t fn_actions[] PROGMEM = {

    [0] = ACTION_FUNCTION(SHIFT_ESC),
    [1] = ACTION_LAYER_TAP_KEY(1 , KC_NO),
    [2] = ACTION_BACKLIGHT_DECREASE(),
    [3] = ACTION_BACKLIGHT_TOGGLE(),
    [4] = ACTION_BACKLIGHT_INCREASE(),
    [5] = ACTION_LAYER_ON(2 , ON_RELEASE),
    [6] = ACTION_LAYER_OFF(2 , ON_RELEASE),
    [7] = ACTION_CHANGE_SEND_MODE(),
    [8] = ACTION_CHANGE_BLE_NAME(),
    [9] = ACTION_PAIRING_BLE(),
    [10] = ACTION_DISCONT_BLE()

};
