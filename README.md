# GekkoGameUE
This is an Unreal Engine 5 port "GekkoGame", the Pong example from [GekkoNet](https://github.com/HeatXD/GekkoNet).
This uses the [GekkoNetUE](https://github.com/koenjicode/GekkoNetUE) plugin for networked matches and [RedoUE](https://github.com/koenjicode/RedoUE) for replay management.

![](preview.png)

## Setup
- Install Unreal Engine 5.7
- Install Visual Studio and the related game dependencies.
- Clone the repo using `git clone --recursive https://github.com/koenjicode/GekkoGameUE.git`.
- Generate the project files (Right click on the .uproject file).
- Follow the build instructions for GekkoNetUE [here](https://github.com/koenjicode/GekkoNetUE/blob/main/README.md).
- Open the .sln or .uproject file.

## Controls
This game can be played online or locally with either a keyboard or a controller.
- Arrow Keys | W,A,S,D | D-Pad | Analog Stick = Move paddle up and down
- 1 = Increase local delay online
- 2 = Decrease local delay online
- 3 = Pull up network stats
- 4 = Exit Match
- 8 = Rewind replay.
- 9 = Fast forward replay.
- 0 = Start/Stop replay takeover
