{
  description = "Time Table Scheduler Dev Env(C, C++, Qt 6)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        clang
        clang-tools
        cmake
        gnumake
        pkg-config
        qt6.qtbase
        qt6.qtwayland
      ];

      shellHook = ''
        export CC=clang
        export CXX=clang++
        export QT_QPA_PLATFORM="wayland;xcb"
        export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}:${pkgs.qt6.qtwayland}/${pkgs.qt6.qtbase.qtPluginPrefix}"
        echo "Loaded Timetable Suite devShell (Clang + CMake + Qt 6)"
      '';
    };
  };
}
