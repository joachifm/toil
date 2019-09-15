with (import <nixpkgs> {});

let
  hsDevEnv = haskellPackages.ghcWithPackages (hsPkgs: with hsPkgs; [
    random
  ]);
in

mkShell {
  name = "dev-shell";
  nativeBuildInputs = [
    git

    doxygen

    cscope
    ctags
    global

    clang-tools  # clang-tidy et al
    gdb
    valgrind

    hsDevEnv
  ];
  inherit hsDevEnv;
  shellHook = ''
    eval $(grep export $hsDevEnv/bin/ghc)
  '';
}
