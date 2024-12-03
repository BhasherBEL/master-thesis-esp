{
  description = "ESP8266/ESP32 development tools";

  inputs = {
    nixpkgs.url = "nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    {
      overlays.default = import ./overlay.nix;
    }
    // flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ self.overlays.default ];
        };
      in
      {
        packages = {
          inherit (pkgs) esp-idf-esp32;
        };

        devShells = {
          default = pkgs.mkShell {
            name = "esp-idf-esp32-shell";

            buildInputs = with pkgs; [
              esp-idf-esp32
              platformio
              platformio-core.udev
            ];
          };
        };
      }
    );
}
