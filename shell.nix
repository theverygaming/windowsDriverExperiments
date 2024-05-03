with import <nixpkgs> { };
let
in stdenv.mkDerivation {
  name = "windowsDriverExperiments";
  buildInputs =
    let
    in
    [
      pkgsCross.mingwW64.buildPackages.gcc
      pkgsCross.i686-embedded.buildPackages.gcc

      gnumake

      nasm
    ];
}
