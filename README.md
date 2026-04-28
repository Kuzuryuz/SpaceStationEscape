# Space Station Escape

Space Station Escape is a 3D puzzle escape game built with C++ and OpenGL. The player is trapped inside a damaged space station after a meteor impact and must repair critical systems before escaping.

The game focuses on exploring connected rooms, solving station repair puzzles, and using environmental clues to unlock the final control room.

## Itch.io Links

**TODO: add itch.io project link**

## Screenshots

**Menu screen**
<img width="1920" height="1080" alt="menu" src="https://github.com/user-attachments/assets/31454b1d-f97d-4b2e-9be0-1179b981f824" />

**Main room**
<img width="1920" height="1080" alt="main room" src="https://github.com/user-attachments/assets/efc52ac8-5a80-4dbe-80f0-4b28166bbd1c" />

**Oxygen room**
<img width="1920" height="1080" alt="oxygen room" src="https://github.com/user-attachments/assets/60284ef6-2d36-4466-90c7-463beed9a224" />

**Power puzzle**
<img width="1920" height="1080" alt="power puzzle" src="https://github.com/user-attachments/assets/6928f243-c005-4970-b57b-8af19716db2b" />

**Lab puzzle**
<img width="1920" height="1080" alt="lab puzzle" src="https://github.com/user-attachments/assets/130df2de-a281-4f80-885c-b1ae372109e8" />

**Control room code puzzle**
<img width="1920" height="1080" alt="control code puzzle" src="https://github.com/user-attachments/assets/3d5781b5-3824-4a99-a310-5fd8b55a706c" />

## Demo Video

**TODO: add demo vdo**

## Gameplay

After a meteor strike damages the station, the emergency AI guides the player through the repair sequence:

1. Fix the oxygen system by activating the valves in the correct order.
2. Restore main power by solving the wire-cutting puzzle.
3. Recover a clue from storage.
4. Decode the clue in the lab puzzle.
5. Unlock the control room and authorize the escape route.

The player wins by authorizing the escape route from the control terminal. The player can lose by failing critical repair puzzles.

## Controls

- `WASD` - Move
- `Mouse` - Move camera
- `Shift` - Run
- `E` - Interact
- `Enter` - Confirm puzzle input
- `Arrow keys` - Navigate or adjust puzzle values
- `Backspace` - Delete code input
- `Esc` - Close puzzle HUD or exit
- `R` - Restart after winning or losing

## Features

- Third-person 3D exploration
- Room-based space station layout
- Oxygen, power, lab, and code puzzles
- Collision-aware player and camera movement
- HUD panels for puzzle interactions
- Story subtitles and objective guidance
- Sound effects and background audio using miniaudio
- Animated player character and animated oxygen pipe sequence

## Build

This project uses CMake.

```powershell
cmake --preset x64-release
cmake --build out/build/x64-release --config Release
```

The executable is generated at:

```text
out/build/x64-release/SpaceStationEscape.exe
```

For a playable release package, place the executable next to the runtime asset folders:

```text
SpaceStationEscape-release/
  SpaceStationEscape.exe
  assets/
  shaders/
```

## Source Code

Main source folders:

- `src/` - C++ game code
- `src/audio/` - Audio playback system
- `src/graphics/` - Shader, texture, model, and animation rendering helpers
- `src/world/` - Space station layout, interactables, collisions, and scene placement
- `shaders/` - GLSL shader files
- `include/` - Header-only third-party libraries

## Attribution

TODO: Fill in exact asset/source links before submission.

- OpenGL - Graphics API
- GLFW - Window creation and input
- GLAD - OpenGL function loader
- GLM - Mathematics library
- Assimp - Model and animation loading
- miniaudio - Audio playback
- 3D character model and animations: **TODO: add source/author/license**
- Space station models and props: **TODO: add source/author/license**
- Puzzle posters/textures: **TODO: add source/author/license**
- Sound effects and background audio: **TODO: add source/author/license**
