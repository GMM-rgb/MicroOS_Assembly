#ifdef RAW_HARDWARE
// On real hardware use inline assembly to reboot via the keyboard controller.
void bios_restart(void) {
    __asm__ volatile (
      "mov $0xfe, %%al\n"
      "out %%al, $0x64\n"
      :
      :
      : "al"
    );
}
#else
// In simulation mode do nothing special.
void bios_restart(void) { }
#endif
