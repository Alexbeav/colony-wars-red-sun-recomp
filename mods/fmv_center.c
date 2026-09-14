/* SLUS-00866: the movie callback uses (256 - 192)/2 even for 220-row films.
 * The decoded height first becomes available after the initial rectangles
 * are built. Center each verified movie slice before LoadImage consumes it.
 * Evidence: Notion wave4/colony-wars-red-sun/fmv-placement-20260913.
 */
#include "mod_plugins.h"
#include "cpu_state.h"

static int ram_range(uint32_t address, uint32_t size) {
    return address >= 0x80010000u && address <= 0x80200000u - size;
}

static void center_fmv(CPUState *cpu, uint32_t address) {
    if (address != 0x8007FAD0u || !psx_mod_game_started() ||
        psx_mod_read_word(address) != 0x27BDFFE0u ||
        cpu->gpr[31] != 0x800B8E54u)
        return;
    const uint32_t context = psx_mod_read_word(0x800BFE94u);
    const uint32_t rect = cpu->gpr[4];
    if (!ram_range(context, 0x60u) || !ram_range(rect, 8)) return;
    const uint16_t x = psx_mod_read_half(rect), y = psx_mod_read_half(rect + 2);
    const uint16_t height = psx_mod_read_half(rect + 6);
    const uint32_t buffers = psx_mod_read_word(context + 0x50u);
    if (!ram_range(buffers, 40) ||
        psx_mod_read_half(buffers + 2) != 0 ||
        psx_mod_read_half(buffers + 22) != 256 ||
        psx_mod_read_word(context + 0x40u) != 1 ||
        psx_mod_read_half(rect + 4) != 24 || x > 456 || x % 24 != 0 ||
        (y != 32 && y != 288) || height == 0 || height > 240 ||
        psx_mod_read_word(rect + 4) != psx_mod_read_word(context + 0x38u))
        return;
    psx_mod_write_half(rect + 2, (uint16_t)((y & 256u) + (240 - height) / 2));
}

static void activate_center_fmv(void) {
    (void)psx_mod_register_function_entry_plugin(
        "red-sun.center-fmv", 0x8007FAD0u, center_fmv);
}

PSX_MOD_CONSTRUCTOR(register_red_sun_fmv_center) {
    (void)psx_mod_register_activation_plugin("red-sun.center-fmv", activate_center_fmv);
}
