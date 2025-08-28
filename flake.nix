{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs { inherit system; };
      in
        {
          devShell = pkgs.mkShell {
            name = "sistemioperativi_hashing_dev";
            packages = with pkgs; [
              clang-tools
              gcc
              openssl
              meson
              ninja
            ];
          };
        }
    );
}
