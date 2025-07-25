{
  description = "devshell";

  outputs =
    { nixpkgs, ... }:
    let
      forAllSystems =
        f: nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed (s: f nixpkgs.legacyPackages.${s});
    in
    {
      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            stdenv.cc.cc
            arduino-cli
            platformio
          ];

          LD_LIBRARY_PATH = "${pkgs.stdenv.cc.cc.lib}/lib";
          ARDUINO_CONFIG_FILE = "./arduino-cli.yaml";
        };
      });
    };
}
