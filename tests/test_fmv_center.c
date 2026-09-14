#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../mods/fmv_center.c"
static unsigned char ram[0x200000];
static CPUState cpu;
static int writes, started;
int psx_mod_game_started(void) { return started; }
uint16_t psx_mod_read_half(uint32_t a) {
    uint16_t v; memcpy(&v, ram + (a & 0x1fffff), 2); return v;
}
uint32_t psx_mod_read_word(uint32_t a) {
    uint32_t v; memcpy(&v, ram + (a & 0x1fffff), 4); return v;
}
void psx_mod_write_half(uint32_t a, uint16_t v) {
    memcpy(ram + (a & 0x1fffff), &v, 2); ++writes;
}
int psx_mod_register_function_entry_plugin(const char *id, uint32_t a,
                                          PSXModFunctionEntryCallback cb) {
    assert(strcmp(id, "red-sun.center-fmv") == 0 && a == 0x8007FAD0 && cb == center_fmv);
    return 1;
}
int psx_mod_register_activation_plugin(const char *id, PSXModActivationCallback cb) {
    assert(strcmp(id, "red-sun.center-fmv") == 0 && cb == activate_center_fmv);
    return 1;
}
static void word(uint32_t a, uint32_t v) { memcpy(ram + (a & 0x1fffff), &v, 4); }
static void setup(unsigned height, unsigned y) {
    memset(ram, 0, sizeof ram); memset(&cpu, 0, sizeof cpu); writes = 0; started = 1;
    cpu.gpr[4] = 0x8008D0B8; cpu.gpr[31] = 0x800B8E54;
    word(0x8007FAD0, 0x27BDFFE0); word(0x80080F24, 0xAC620000);
    word(0x800BFE94, 0x8016E154); word(0x8016E1A4, 0x800BFEB4);
    word(0x800BFEC8, 256u << 16); word(0x8016E194, 1);
    word(0x8008D0B8, (y << 16) | 240); word(0x8016E188, (y << 16) | 264);
    word(0x8008D0BC, (height << 16) | 24); word(0x8016E18C, (height << 16) | 24);
}
static void run(void) { center_fmv(&cpu, 0x8007FAD0); }
int main(void) {
    activate_center_fmv();
    const unsigned heights[] = {192, 220, 239, 240};
    for (unsigned p = 0; p < 2; ++p) for (unsigned i = 0; i < 4; ++i) {
        unsigned y = p * 256 + 32; setup(heights[i], y); run();
        assert(psx_mod_read_half(0x8008D0BA) == p * 256 + (240 - heights[i]) / 2);
        assert(psx_mod_read_word(0x8016E188) == (y << 16 | 264));
        assert(psx_mod_read_half(0x8008D0B8) == 240 && writes == 1);
        run(); assert(writes == 1); /* Idempotent when called again. */
    }
    setup(220, 288); word(0x8008D0B8, 288u << 16 | 456); word(0x8016E188, 32u << 16); run(); assert(writes == 1 && psx_mod_read_half(0x8008D0BA) == 266); /* Final slice after buffer advance. */
    setup(220, 32); word(0x8007FAD0, 0); run(); assert(writes == 0);
    setup(220, 32); started = 0; run(); assert(writes == 0);
    setup(220, 32); word(0x800BFE94, 0x80200000); run(); assert(writes == 0);
    setup(220, 32); cpu.gpr[4] = 0; run(); assert(writes == 0);
    setup(220, 32); word(0x8016E1A4, 0); run(); assert(writes == 0);
    setup(241, 32); run(); assert(writes == 0);
    setup(0, 32); run(); assert(writes == 0);
    setup(220, 17); run(); assert(writes == 0);
    setup(220, 32); word(0x8016E194, 0); run(); assert(writes == 0);
    setup(220, 32); cpu.gpr[31] = 0; run(); assert(writes == 0);
    setup(220, 32); word(0x8008D0BC, 220u << 16 | 320); run(); assert(writes == 0);
    puts("Movie slice centering: first-upload geometry, both buffers and guards passed");
}
