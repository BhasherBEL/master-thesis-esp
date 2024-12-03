final: prev: {
  esp-idf-esp32 = (prev.callPackage ./pkgs/esp-idf { }).override {
    toolsToInclude = [
      "xtensa-esp-elf"
      "esp32ulp-elf"
      "openocd-esp32"
      "xtensa-esp-elf-gdb"
    ];
  };
}
