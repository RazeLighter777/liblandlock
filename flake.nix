{
  description = "liblandlock";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      # Define the systems you want to support
      supportedSystems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ];
      # Function to create outputs for each system
      forAllSystems =
        f: nixpkgs.lib.genAttrs supportedSystems (system: f { pkgs = import nixpkgs { inherit system; }; });
    in
    {
      devShells = forAllSystems (
        { pkgs }:
        {
          default = pkgs.mkShell {
            # nativeBuildInputs are for tools you run (compiler, build systems)
            nativeBuildInputs = with pkgs; [
              gcc # The C compiler
              gnumake # A common build tool
              cmake # Another common build tool
              pkg-config # Helps manage library linking
            ];

            # buildInputs are for libraries your code links against (e.g., a specific library)
            buildInputs = with pkgs; [
              # Add any specific C libraries here, e.g.
              # someCspecificLibrary
            ];

            # Set environment variables or run commands when entering the shell
            shellHook = ''
              # You can add further customization here
            '';
          };
        }
      );
    };
}
